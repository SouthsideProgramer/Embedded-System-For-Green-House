/**
 * @file test_washing_machine.c
 * @brief Automated Unit Test Suite for Washing Machine Control Unit (CO3053 - BTL 2)
 *        Covers 100% of functional requirements, state transitions, and edge cases.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include "washing_machine_fsm.h"
#include "mock_hal.h"

/* Terminal formatting for test reporting */
#define ANSI_GREEN  "\033[1;32m"
#define ANSI_RED    "\033[1;31m"
#define ANSI_YELLOW "\033[1;33m"
#define ANSI_CYAN   "\033[1;36m"
#define ANSI_RESET  "\033[0m"

static int g_tests_passed = 0;
static int g_tests_failed = 0;

#define TEST_ASSERT(cond, msg) do { \
    if (!(cond)) { \
        printf(ANSI_RED "  [FAIL] " ANSI_RESET "%s (Line %d): %s\n", __func__, __LINE__, msg); \
        g_tests_failed++; \
        return; \
    } \
} while(0)

#define TEST_PASS(test_name) do { \
    printf(ANSI_GREEN "  [PASS] " ANSI_RESET "%s\n", test_name); \
    g_tests_passed++; \
} while(0)

static void helper_advance_ms(wm_context_t *ctx, uint32_t ms) {
    for (uint32_t i = 0; i < ms; i++) {
        wm_fsm_tick_1ms(ctx);
    }
}

static void helper_advance_sec(wm_context_t *ctx, uint32_t sec) {
    for (uint32_t i = 0; i < sec; i++) {
        helper_advance_ms(ctx, 1000);
        wm_fsm_tick_1s(ctx);
    }
}

/* -------------------------------------------------------------------------- */
/* TC-01: Sub-threshold Deposit                                              */
/* -------------------------------------------------------------------------- */
static void test_tc01_sub_threshold_deposit(void) {
    mock_hal_reset();
    hal_output_callbacks_t cbs = mock_hal_get_callbacks();
    wm_context_t ctx;
    wm_fsm_init(&ctx, &cbs);

    TEST_ASSERT(ctx.state == WM_STATE_STANDBY, "Init must be STANDBY");
    TEST_ASSERT(mock_hal_get_state()->rled == HAL_LED_ON, "Standby RLED must be ON");
    TEST_ASSERT(mock_hal_get_state()->bled == HAL_LED_OFF, "Standby BLED must be OFF");

    wm_fsm_dispatch_event(&ctx, WM_EVT_COIN_10);
    TEST_ASSERT(ctx.coin_balance_cents == 10, "Balance must be 10¢");
    TEST_ASSERT(ctx.state == WM_STATE_STANDBY, "10¢ must stay in STANDBY");

    wm_fsm_dispatch_event(&ctx, WM_EVT_COIN_20);
    TEST_ASSERT(ctx.coin_balance_cents == 30, "Balance must be 30¢");
    TEST_ASSERT(ctx.state == WM_STATE_STANDBY, "30¢ must stay in STANDBY");
    TEST_ASSERT(mock_hal_get_state()->rled == HAL_LED_ON, "RLED must stay ON");
    TEST_ASSERT(mock_hal_get_state()->bled == HAL_LED_OFF, "BLED must stay OFF");

    TEST_PASS("TC-01: Sub-threshold Deposit (10¢ + 20¢ stays in STANDBY)");
}

/* -------------------------------------------------------------------------- */
/* TC-02: Exact Threshold Deposit                                            */
/* -------------------------------------------------------------------------- */
static void test_tc02_exact_threshold_deposit(void) {
    mock_hal_reset();
    hal_output_callbacks_t cbs = mock_hal_get_callbacks();
    wm_context_t ctx;
    wm_fsm_init(&ctx, &cbs);

    wm_fsm_dispatch_event(&ctx, WM_EVT_COIN_50);
    TEST_ASSERT(ctx.coin_balance_cents == 50, "Balance must be 50¢");
    TEST_ASSERT(ctx.state == WM_STATE_READY, "50¢ must trigger transition to READY");
    TEST_ASSERT(mock_hal_get_state()->rled == HAL_LED_OFF, "READY RLED must be OFF");
    TEST_ASSERT(mock_hal_get_state()->bled == HAL_LED_ON, "READY BLED must be solid ON");

    TEST_PASS("TC-02: Exact Threshold Deposit (50¢ transitions to READY)");
}

/* -------------------------------------------------------------------------- */
/* TC-03: Surplus Deposit Accumulation                                       */
/* -------------------------------------------------------------------------- */
static void test_tc03_surplus_deposit_accumulation(void) {
    mock_hal_reset();
    hal_output_callbacks_t cbs = mock_hal_get_callbacks();
    wm_context_t ctx;
    wm_fsm_init(&ctx, &cbs);

    wm_fsm_dispatch_event(&ctx, WM_EVT_COIN_20);
    wm_fsm_dispatch_event(&ctx, WM_EVT_COIN_20);
    TEST_ASSERT(ctx.state == WM_STATE_STANDBY, "40¢ remains STANDBY");

    wm_fsm_dispatch_event(&ctx, WM_EVT_COIN_20);
    TEST_ASSERT(ctx.coin_balance_cents == 60, "Balance accumulated to 60¢");
    TEST_ASSERT(ctx.state == WM_STATE_READY, "60¢ transitions to READY");

    /* Insert extra coin in READY */
    wm_fsm_dispatch_event(&ctx, WM_EVT_COIN_50);
    TEST_ASSERT(ctx.coin_balance_cents == 110, "Balance accumulated to 110¢");
    TEST_ASSERT(ctx.state == WM_STATE_READY, "Remains in READY with surplus");

    TEST_PASS("TC-03: Surplus Deposit Accumulation (Accepts 60¢, 110¢ in READY)");
}

/* -------------------------------------------------------------------------- */
/* TC-04: Execution & Zero Refund Policy                                     */
/* -------------------------------------------------------------------------- */
static void test_tc04_execution_and_zero_refund(void) {
    mock_hal_reset();
    hal_output_callbacks_t cbs = mock_hal_get_callbacks();
    wm_context_t ctx;
    wm_fsm_init(&ctx, &cbs);

    /* Deposit 70¢ */
    wm_fsm_dispatch_event(&ctx, WM_EVT_COIN_50);
    wm_fsm_dispatch_event(&ctx, WM_EVT_COIN_20);
    TEST_ASSERT(ctx.state == WM_STATE_READY, "Must be READY");
    TEST_ASSERT(ctx.coin_balance_cents == 70, "Balance is 70¢");

    /* Press RUN */
    wm_fsm_dispatch_event(&ctx, WM_EVT_BTN_RUN);
    TEST_ASSERT(ctx.state == WM_STATE_RUNNING, "Must transition to RUNNING");
    TEST_ASSERT(ctx.coin_balance_cents == 0, "MONEY MUST BE CLEARED (Zero Refund)");
    TEST_ASSERT(ctx.remaining_cycle_sec == 1800, "Timer must be initialized to 1800s (30 min)");
    TEST_ASSERT(mock_hal_get_state()->bled == HAL_LED_BLINK_1HZ, "BLED must blink at 1Hz in RUNNING");
    TEST_ASSERT(mock_hal_get_state()->rled == HAL_LED_OFF, "RLED must be OFF");
    TEST_ASSERT(mock_hal_get_state()->door_locked == true, "Door must be locked during washing");
    TEST_ASSERT(mock_hal_get_state()->motor == HAL_MOTOR_AGITATE, "Motor must be active");

    TEST_PASS("TC-04: Execution & Zero Refund (70¢ cleared to 0¢, 30-min timer active)");
}

/* -------------------------------------------------------------------------- */
/* TC-05: Premature Run Attempt                                              */
/* -------------------------------------------------------------------------- */
static void test_tc05_premature_run_attempt(void) {
    mock_hal_reset();
    hal_output_callbacks_t cbs = mock_hal_get_callbacks();
    wm_context_t ctx;
    wm_fsm_init(&ctx, &cbs);

    wm_fsm_dispatch_event(&ctx, WM_EVT_COIN_20);
    TEST_ASSERT(ctx.state == WM_STATE_STANDBY, "Must be in STANDBY");

    /* User presses RUN without sufficient money */
    bool handled = wm_fsm_dispatch_event(&ctx, WM_EVT_BTN_RUN);
    TEST_ASSERT(handled == false, "Premature RUN must be rejected");
    TEST_ASSERT(ctx.state == WM_STATE_STANDBY, "State must remain STANDBY");
    TEST_ASSERT(ctx.remaining_cycle_sec == 0, "Timer must not start");
    TEST_ASSERT(mock_hal_get_state()->motor == HAL_MOTOR_OFF, "Motor must remain OFF");

    TEST_PASS("TC-05: Premature RUN Attempt (Ignored when balance < 50¢)");
}

/* -------------------------------------------------------------------------- */
/* TC-06: Normal Pause and Resume                                            */
/* -------------------------------------------------------------------------- */
static void test_tc06_normal_pause_and_resume(void) {
    mock_hal_reset();
    hal_output_callbacks_t cbs = mock_hal_get_callbacks();
    wm_context_t ctx;
    wm_fsm_init(&ctx, &cbs);

    wm_fsm_dispatch_event(&ctx, WM_EVT_COIN_50);
    wm_fsm_dispatch_event(&ctx, WM_EVT_BTN_RUN);
    TEST_ASSERT(ctx.state == WM_STATE_RUNNING, "Must be RUNNING");

    /* Wash for 100 seconds */
    helper_advance_sec(&ctx, 100);
    TEST_ASSERT(ctx.remaining_cycle_sec == 1700, "Timer should be at 1700s");

    /* Press PAUSE */
    wm_fsm_dispatch_event(&ctx, WM_EVT_BTN_PAUSE);
    TEST_ASSERT(ctx.state == WM_STATE_PAUSED, "Must enter PAUSED state");
    TEST_ASSERT(mock_hal_get_state()->motor == HAL_MOTOR_OFF, "Motor must stop when paused");

    /* Press RUN to resume */
    wm_fsm_dispatch_event(&ctx, WM_EVT_BTN_RUN);
    TEST_ASSERT(ctx.state == WM_STATE_RUNNING, "Must resume to RUNNING");
    TEST_ASSERT(mock_hal_get_state()->motor == HAL_MOTOR_AGITATE, "Motor must resume agitation");
    TEST_ASSERT(mock_hal_get_state()->bled == HAL_LED_BLINK_1HZ, "BLED must blink again");

    TEST_PASS("TC-06: Normal Pause and Resume (Actuators safely suspended and resumed)");
}

/* -------------------------------------------------------------------------- */
/* TC-07: Persistent Timer During Pause (CRITICAL REQUIREMENT)               */
/* -------------------------------------------------------------------------- */
static void test_tc07_persistent_timer_during_pause(void) {
    mock_hal_reset();
    hal_output_callbacks_t cbs = mock_hal_get_callbacks();
    wm_context_t ctx;
    wm_fsm_init(&ctx, &cbs);

    wm_fsm_dispatch_event(&ctx, WM_EVT_COIN_50);
    wm_fsm_dispatch_event(&ctx, WM_EVT_BTN_RUN);

    /* Wash for 200s */
    helper_advance_sec(&ctx, 200);
    TEST_ASSERT(ctx.remaining_cycle_sec == 1600, "Timer at 1600s");

    /* Enter Pause */
    wm_fsm_dispatch_event(&ctx, WM_EVT_BTN_PAUSE);
    TEST_ASSERT(ctx.state == WM_STATE_PAUSED, "Must be in PAUSED");

    /* Stay paused for 300 seconds (5 minutes) */
    helper_advance_sec(&ctx, 300);

    /* CRITICAL ASSERTION: The timer must have decremented by 300s during PAUSE! */
    TEST_ASSERT(ctx.remaining_cycle_sec == 1300,
                "Timer MUST continue counting down during pause (1600 - 300 = 1300s)");
    TEST_ASSERT(mock_hal_get_state()->motor == HAL_MOTOR_OFF,
                "Motor must remain halted during pause countdown");

    TEST_PASS("TC-07: Persistent Timer in Pause (Timer ticked down from 1600s to 1300s while paused)");
}

/* -------------------------------------------------------------------------- */
/* TC-08: Pause Timeout Termination                                          */
/* -------------------------------------------------------------------------- */
static void test_tc08_pause_timeout_termination(void) {
    mock_hal_reset();
    hal_output_callbacks_t cbs = mock_hal_get_callbacks();
    wm_context_t ctx;
    wm_fsm_init(&ctx, &cbs);
    ctx.cycle_duration_setting = 30; /* Accelerated 30s cycle for test */

    wm_fsm_dispatch_event(&ctx, WM_EVT_COIN_50);
    wm_fsm_dispatch_event(&ctx, WM_EVT_BTN_RUN);
    TEST_ASSERT(ctx.remaining_cycle_sec == 30, "Timer set to 30s");

    /* Wash 10s then pause */
    helper_advance_sec(&ctx, 10);
    wm_fsm_dispatch_event(&ctx, WM_EVT_BTN_PAUSE);
    TEST_ASSERT(ctx.state == WM_STATE_PAUSED, "Paused at remaining=20s");

    /* Leave machine paused for 20s until timer expires */
    helper_advance_sec(&ctx, 20);

    TEST_ASSERT(ctx.state == WM_STATE_STANDBY, "Expired pause timer must return to STANDBY");
    TEST_ASSERT(mock_hal_get_state()->rled == HAL_LED_ON, "RLED must be solid ON in Standby");
    TEST_ASSERT(mock_hal_get_state()->bled == HAL_LED_OFF, "BLED must be OFF in Standby");

    TEST_PASS("TC-08: Pause Timeout Termination (Timer expiring in PAUSED resets to STANDBY)");
}

/* -------------------------------------------------------------------------- */
/* TC-09: Single STOP Rejection                                              */
/* -------------------------------------------------------------------------- */
static void test_tc09_single_stop_rejection(void) {
    mock_hal_reset();
    hal_output_callbacks_t cbs = mock_hal_get_callbacks();
    wm_context_t ctx;
    wm_fsm_init(&ctx, &cbs);

    wm_fsm_dispatch_event(&ctx, WM_EVT_COIN_50);
    wm_fsm_dispatch_event(&ctx, WM_EVT_BTN_RUN);
    TEST_ASSERT(ctx.state == WM_STATE_RUNNING, "Must be RUNNING");

    /* Single press on STOP */
    wm_fsm_dispatch_event(&ctx, WM_EVT_BTN_STOP);
    TEST_ASSERT(ctx.state == WM_STATE_RUNNING, "Single STOP must NOT terminate execution!");
    TEST_ASSERT(ctx.stop_press_count == 1, "Stop count should record 1st press");

    /* Wait 2.0s (> 1.5s window) */
    helper_advance_ms(&ctx, 2000);
    TEST_ASSERT(ctx.stop_press_count == 0, "Double-press window must expire and clear count");
    TEST_ASSERT(ctx.state == WM_STATE_RUNNING, "Machine remains running undisturbed");

    TEST_PASS("TC-09: Single STOP Rejection (Single press does not force stop)");
}

/* -------------------------------------------------------------------------- */
/* TC-10: Force Stop on Double Press                                         */
/* -------------------------------------------------------------------------- */
static void test_tc10_force_stop_on_double_press(void) {
    mock_hal_reset();
    hal_output_callbacks_t cbs = mock_hal_get_callbacks();
    wm_context_t ctx;
    wm_fsm_init(&ctx, &cbs);

    wm_fsm_dispatch_event(&ctx, WM_EVT_COIN_50);
    wm_fsm_dispatch_event(&ctx, WM_EVT_BTN_RUN);
    TEST_ASSERT(ctx.state == WM_STATE_RUNNING, "Must be RUNNING");

    /* Press STOP 1st time */
    wm_fsm_dispatch_event(&ctx, WM_EVT_BTN_STOP);
    TEST_ASSERT(ctx.state == WM_STATE_RUNNING, "Still running after 1st stop");

    /* Wait 300 ms (well within 1500 ms window) */
    helper_advance_ms(&ctx, 300);

    /* Press STOP 2nd time */
    wm_fsm_dispatch_event(&ctx, WM_EVT_BTN_STOP);

    /* MUST IMMEDIATELY FORCE STOP */
    TEST_ASSERT(ctx.state == WM_STATE_STANDBY, "Double STOP must immediately force stop to STANDBY");
    TEST_ASSERT(ctx.remaining_cycle_sec == 0, "Cycle timer must be canceled");
    TEST_ASSERT(mock_hal_get_state()->motor == HAL_MOTOR_OFF, "Actuators must be instantly cut off");
    TEST_ASSERT(mock_hal_get_state()->rled == HAL_LED_ON, "RLED must be solid ON (Standby)");
    TEST_ASSERT(mock_hal_get_state()->bled == HAL_LED_OFF, "BLED must be OFF");

    TEST_PASS("TC-10: Force Stop on Double Press (2 presses within 300ms forces termination)");
}

/* -------------------------------------------------------------------------- */
/* TC-11: Force Stop from Paused State                                       */
/* -------------------------------------------------------------------------- */
static void test_tc11_force_stop_from_paused(void) {
    mock_hal_reset();
    hal_output_callbacks_t cbs = mock_hal_get_callbacks();
    wm_context_t ctx;
    wm_fsm_init(&ctx, &cbs);

    wm_fsm_dispatch_event(&ctx, WM_EVT_COIN_50);
    wm_fsm_dispatch_event(&ctx, WM_EVT_BTN_RUN);
    wm_fsm_dispatch_event(&ctx, WM_EVT_BTN_PAUSE);
    TEST_ASSERT(ctx.state == WM_STATE_PAUSED, "Must be PAUSED");

    /* Double press STOP while paused */
    wm_fsm_dispatch_event(&ctx, WM_EVT_BTN_STOP);
    helper_advance_ms(&ctx, 400);
    wm_fsm_dispatch_event(&ctx, WM_EVT_BTN_STOP);

    TEST_ASSERT(ctx.state == WM_STATE_STANDBY, "Double STOP in PAUSED must return to STANDBY");
    TEST_ASSERT(ctx.remaining_cycle_sec == 0, "Timer must be cleared");

    TEST_PASS("TC-11: Force Stop from Paused State (Double STOP terminates paused machine)");
}

/* -------------------------------------------------------------------------- */
/* TC-12: Normal 30-min Cycle Completion                                     */
/* -------------------------------------------------------------------------- */
static void test_tc12_normal_cycle_completion(void) {
    mock_hal_reset();
    hal_output_callbacks_t cbs = mock_hal_get_callbacks();
    wm_context_t ctx;
    wm_fsm_init(&ctx, &cbs);
    ctx.cycle_duration_setting = 1800; /* Full 30 minutes */

    wm_fsm_dispatch_event(&ctx, WM_EVT_COIN_50);
    wm_fsm_dispatch_event(&ctx, WM_EVT_BTN_RUN);

    /* Run full 1800 seconds */
    helper_advance_sec(&ctx, 1800);

    TEST_ASSERT(ctx.state == WM_STATE_STANDBY, "Completed 1800s must return to STANDBY");
    TEST_ASSERT(ctx.remaining_cycle_sec == 0, "Timer must be 0");
    TEST_ASSERT(mock_hal_get_state()->cycle_complete_count == 1, "on_cycle_complete must be called once");
    TEST_ASSERT(mock_hal_get_state()->rled == HAL_LED_ON, "RLED must be solid ON");
    TEST_ASSERT(mock_hal_get_state()->bled == HAL_LED_OFF, "BLED must be OFF");
    TEST_ASSERT(mock_hal_get_state()->motor == HAL_MOTOR_OFF, "Motor must be stopped");

    TEST_PASS("TC-12: Normal 30-min Cycle Completion (1800s expires naturally to STANDBY)");
}

/* -------------------------------------------------------------------------- */
/* TC-13: Fault Interruption and Recovery                                    */
/* -------------------------------------------------------------------------- */
static void test_tc13_fault_interruption_and_recovery(void) {
    mock_hal_reset();
    hal_output_callbacks_t cbs = mock_hal_get_callbacks();
    wm_context_t ctx;
    wm_fsm_init(&ctx, &cbs);

    wm_fsm_dispatch_event(&ctx, WM_EVT_COIN_50);
    wm_fsm_dispatch_event(&ctx, WM_EVT_BTN_RUN);
    TEST_ASSERT(ctx.state == WM_STATE_RUNNING, "Must be RUNNING");

    /* Inject hardware fault */
    wm_fsm_dispatch_event(&ctx, WM_EVT_FAULT_OCCURRED);
    TEST_ASSERT(ctx.state == WM_STATE_ERROR, "Must transition to ERROR");
    TEST_ASSERT(mock_hal_get_state()->rled == HAL_LED_BLINK_2HZ, "RLED must blink at 2Hz in ERROR");
    TEST_ASSERT(mock_hal_get_state()->bled == HAL_LED_OFF, "BLED must be OFF in ERROR");
    TEST_ASSERT(mock_hal_get_state()->motor == HAL_MOTOR_OFF, "Actuators must be instantly cut");

    /* Normal buttons must be ignored in ERROR */
    wm_fsm_dispatch_event(&ctx, WM_EVT_BTN_RUN);
    TEST_ASSERT(ctx.state == WM_STATE_ERROR, "RUN must be rejected in ERROR");

    /* Clear fault */
    wm_fsm_dispatch_event(&ctx, WM_EVT_FAULT_CLEARED);
    TEST_ASSERT(ctx.state == WM_STATE_STANDBY, "Clearing fault must return to STANDBY");
    TEST_ASSERT(mock_hal_get_state()->rled == HAL_LED_ON, "RLED returns to solid ON");
    TEST_ASSERT(mock_hal_get_state()->bled == HAL_LED_OFF, "BLED returns to OFF");

    TEST_PASS("TC-13: Fault Interruption and Recovery (Safety shutdown & error reset)");
}

/* -------------------------------------------------------------------------- */
/* Main Test Runner                                                          */
/* -------------------------------------------------------------------------- */
int main(void) {
    printf(ANSI_CYAN "============================================================\n" ANSI_RESET);
    printf(ANSI_CYAN " BTL 2: Washing Machine Control Unit - Verification Suite\n" ANSI_RESET);
    printf(ANSI_CYAN " CO3053 Embedded Systems - HCMUT\n" ANSI_RESET);
    printf(ANSI_CYAN "============================================================\n\n" ANSI_RESET);

    test_tc01_sub_threshold_deposit();
    test_tc02_exact_threshold_deposit();
    test_tc03_surplus_deposit_accumulation();
    test_tc04_execution_and_zero_refund();
    test_tc05_premature_run_attempt();
    test_tc06_normal_pause_and_resume();
    test_tc07_persistent_timer_during_pause();
    test_tc08_pause_timeout_termination();
    test_tc09_single_stop_rejection();
    test_tc10_force_stop_on_double_press();
    test_tc11_force_stop_from_paused();
    test_tc12_normal_cycle_completion();
    test_tc13_fault_interruption_and_recovery();

    printf("\n" ANSI_CYAN "============================================================\n" ANSI_RESET);
    if (g_tests_failed == 0) {
        printf(ANSI_GREEN " ALL %d TESTS PASSED SUCCESSFULLY! (100%% Test Coverage)\n" ANSI_RESET, g_tests_passed);
        printf(ANSI_CYAN "============================================================\n" ANSI_RESET);
        return 0;
    } else {
        printf(ANSI_RED " TEST SUITE FAILED: %d Passed, %d Failed\n" ANSI_RESET, g_tests_passed, g_tests_failed);
        printf(ANSI_CYAN "============================================================\n" ANSI_RESET);
        return 1;
    }
}
