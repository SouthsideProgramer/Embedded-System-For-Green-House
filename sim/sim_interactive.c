/**
 * @file sim_interactive.c
 * @brief Interactive CLI Simulator for Washing Machine Control Unit (CO3053 - BTL 2)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "washing_machine_fsm.h"
#include "mock_hal.h"

#define ANSI_GREEN  "\033[1;32m"
#define ANSI_RED    "\033[1;31m"
#define ANSI_YELLOW "\033[1;33m"
#define ANSI_BLUE   "\033[1;34m"
#define ANSI_CYAN   "\033[1;36m"
#define ANSI_RESET  "\033[0m"

static void print_dashboard(const wm_context_t *ctx, const mock_hal_state_t *hal) {
    uint32_t min = ctx->remaining_cycle_sec / 60;
    uint32_t sec = ctx->remaining_cycle_sec % 60;

    printf("\n" ANSI_CYAN "======================================================================\n" ANSI_RESET);
    printf("   COIN-OPERATED WASHING MACHINE CONTROL UNIT (CO3053 - HCMUT)\n");
    printf(ANSI_CYAN "======================================================================\n" ANSI_RESET);

    /* State row */
    printf(" [STATE]     : ");
    switch (ctx->state) {
        case WM_STATE_STANDBY: printf(ANSI_GREEN "[ STANDBY ]" ANSI_RESET " (Available to serve)\n"); break;
        case WM_STATE_READY:   printf(ANSI_BLUE  "[  READY  ]" ANSI_RESET " (Deposit >= 50¢; Press RUN)\n"); break;
        case WM_STATE_RUNNING: printf(ANSI_YELLOW"[ RUNNING ]" ANSI_RESET " (Washing active)\n"); break;
        case WM_STATE_PAUSED:  printf(ANSI_YELLOW"[ PAUSED  ]" ANSI_RESET " (Suspended; Timer counting down!)\n"); break;
        case WM_STATE_ERROR:   printf(ANSI_RED   "[  ERROR  ]" ANSI_RESET " (System Fault Active!)\n"); break;
        default: break;
    }

    /* Financials & Timer */
    printf(" [BALANCE]   : " ANSI_GREEN "$%u.%02u" ANSI_RESET " (Required threshold: $0.50)\n",
           ctx->coin_balance_cents / 100, ctx->coin_balance_cents % 100);
    printf(" [CYCLE TIME]: " ANSI_CYAN "%02u:%02u" ANSI_RESET " remaining\n", min, sec);

    /* LEDs */
    printf(" [RLED (Red)]: ");
    if (hal->rled == HAL_LED_ON) printf(ANSI_RED "[ SOLID ON ]" ANSI_RESET " (Standby mode)\n");
    else if (hal->rled == HAL_LED_BLINK_2HZ) printf(ANSI_RED "[ BLINKING (2Hz) ]" ANSI_RESET " (FAULT DETECTED)\n");
    else printf("[ OFF ]\n");

    printf(" [BLED (Blue): ");
    if (hal->bled == HAL_LED_ON) printf(ANSI_BLUE "[ SOLID ON ]" ANSI_RESET " (Ready to execute)\n");
    else if (hal->bled == HAL_LED_BLINK_1HZ) printf(ANSI_BLUE "[ BLINKING (1Hz) ]" ANSI_RESET " (Washing in progress)\n");
    else printf("[ OFF ]\n");

    /* Actuators */
    printf(" [MOTOR]     : %s\n", (hal->motor == HAL_MOTOR_AGITATE) ? ANSI_GREEN "AGITATING (Active Wash)" ANSI_RESET :
                                  (hal->motor == HAL_MOTOR_SPIN) ? ANSI_GREEN "SPINNING" ANSI_RESET : "[ STOPPED ]");
    printf(" [DOOR LOCK] : %s\n", hal->door_locked ? ANSI_GREEN "LOCKED" ANSI_RESET : "UNLOCKED");
    printf(ANSI_CYAN "======================================================================\n" ANSI_RESET);
    printf(" COMMANDS:\n");
    printf("  [1] Insert 10¢    [2] Insert 20¢    [3] Insert 50¢\n");
    printf("  [r] Press RUN     [p] Press PAUSE   [s] Press STOP (Single click)\n");
    printf("  [ss] Double STOP (Force stop)       [t <sec>] Advance time by seconds\n");
    printf("  [e] Trigger FAULT [c] Clear FAULT   [q] Quit simulator\n");
    printf(ANSI_CYAN "----------------------------------------------------------------------\n" ANSI_RESET);
    printf(" Enter command > ");
    fflush(stdout);
}

int main(void) {
    mock_hal_reset();
    hal_output_callbacks_t cbs = mock_hal_get_callbacks();
    wm_context_t ctx;
    wm_fsm_init(&ctx, &cbs);

    char line[128];
    while (1) {
        print_dashboard(&ctx, mock_hal_get_state());
        if (!fgets(line, sizeof(line), stdin)) {
            break;
        }

        /* Strip trailing newline */
        line[strcspn(line, "\r\n")] = '\0';

        if (strcmp(line, "q") == 0 || strcmp(line, "quit") == 0) {
            printf("\nExiting simulator. Goodbye!\n");
            break;
        } else if (strcmp(line, "1") == 0) {
            wm_fsm_dispatch_event(&ctx, WM_EVT_COIN_10);
        } else if (strcmp(line, "2") == 0) {
            wm_fsm_dispatch_event(&ctx, WM_EVT_COIN_20);
        } else if (strcmp(line, "3") == 0) {
            wm_fsm_dispatch_event(&ctx, WM_EVT_COIN_50);
        } else if (strcmp(line, "r") == 0 || strcmp(line, "run") == 0) {
            wm_fsm_dispatch_event(&ctx, WM_EVT_BTN_RUN);
        } else if (strcmp(line, "p") == 0 || strcmp(line, "pause") == 0) {
            wm_fsm_dispatch_event(&ctx, WM_EVT_BTN_PAUSE);
        } else if (strcmp(line, "s") == 0 || strcmp(line, "stop") == 0) {
            wm_fsm_dispatch_event(&ctx, WM_EVT_BTN_STOP);
        } else if (strcmp(line, "ss") == 0) {
            /* Simulate double press within window */
            wm_fsm_dispatch_event(&ctx, WM_EVT_BTN_STOP);
            for (int i = 0; i < 200; i++) { wm_fsm_tick_1ms(&ctx); }
            wm_fsm_dispatch_event(&ctx, WM_EVT_BTN_STOP);
        } else if (line[0] == 't') {
            int sec = 1;
            sscanf(line, "t %d", &sec);
            if (sec <= 0) sec = 1;
            for (int i = 0; i < sec; i++) {
                for (int ms = 0; ms < 1000; ms++) {
                    wm_fsm_tick_1ms(&ctx);
                }
                wm_fsm_tick_1s(&ctx);
            }
        } else if (strcmp(line, "e") == 0 || strcmp(line, "error") == 0) {
            wm_fsm_dispatch_event(&ctx, WM_EVT_FAULT_OCCURRED);
        } else if (strcmp(line, "c") == 0 || strcmp(line, "clear") == 0) {
            wm_fsm_dispatch_event(&ctx, WM_EVT_FAULT_CLEARED);
        } else if (strlen(line) > 0) {
            printf(ANSI_YELLOW "Unknown command: '%s'\n" ANSI_RESET, line);
        }
    }

    return 0;
}
