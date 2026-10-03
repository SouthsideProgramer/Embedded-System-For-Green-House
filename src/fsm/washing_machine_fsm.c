/**
 * @file washing_machine_fsm.c
 * @brief Implementation of the Washing Machine Finite State Machine (FSM)
 *        CO3053 - Embedded Systems - Assignment 2 (BTL 2)
 */

#include "washing_machine_fsm.h"
#include <stddef.h>

/* Helper macro for safe callback execution */
#define CALL_HAL(cb, func, ...) do { if ((cb)->func) { (cb)->func(__VA_ARGS__); } } while(0)
#define CALL_HAL_VOID(cb, func)  do { if ((cb)->func) { (cb)->func(); } } while(0)

static void enter_standby(wm_context_t *ctx) {
    ctx->state = WM_STATE_STANDBY;
    ctx->coin_balance_cents = 0;
    ctx->remaining_cycle_sec = 0;
    ctx->stop_press_count = 0;
    ctx->stop_window_timer_ms = 0;

    CALL_HAL(&ctx->callbacks, set_motor, HAL_MOTOR_OFF);
    CALL_HAL(&ctx->callbacks, set_water_valve, false);
    CALL_HAL(&ctx->callbacks, set_drain_pump, false);
    CALL_HAL(&ctx->callbacks, set_door_lock, false);
    CALL_HAL(&ctx->callbacks, set_bled, HAL_LED_OFF);
    CALL_HAL(&ctx->callbacks, set_rled, HAL_LED_ON);
}

static void enter_ready(wm_context_t *ctx) {
    ctx->state = WM_STATE_READY;
    CALL_HAL(&ctx->callbacks, set_rled, HAL_LED_OFF);
    CALL_HAL(&ctx->callbacks, set_bled, HAL_LED_ON);
}

static void enter_running(wm_context_t *ctx, bool is_resuming) {
    ctx->state = WM_STATE_RUNNING;
    ctx->stop_press_count = 0;
    ctx->stop_window_timer_ms = 0;

    if (!is_resuming) {
        /* Unconditional clearance of money without returning redundancies */
        ctx->coin_balance_cents = 0;
        /* Activate 30-minute countdown timer */
        ctx->remaining_cycle_sec = (ctx->cycle_duration_setting > 0) ?
                                    ctx->cycle_duration_setting : WM_CYCLE_DURATION_SEC;
    }

    CALL_HAL(&ctx->callbacks, set_rled, HAL_LED_OFF);
    CALL_HAL(&ctx->callbacks, set_bled, HAL_LED_BLINK_1HZ);
    CALL_HAL(&ctx->callbacks, set_door_lock, true);
    CALL_HAL(&ctx->callbacks, set_motor, HAL_MOTOR_AGITATE);
}

static void enter_paused(wm_context_t *ctx) {
    ctx->state = WM_STATE_PAUSED;
    ctx->stop_press_count = 0;
    ctx->stop_window_timer_ms = 0;

    /* Actuators suspended, but remaining_cycle_sec continues counting down! */
    CALL_HAL(&ctx->callbacks, set_motor, HAL_MOTOR_OFF);
    CALL_HAL(&ctx->callbacks, set_water_valve, false);
    CALL_HAL(&ctx->callbacks, set_drain_pump, false);
    CALL_HAL(&ctx->callbacks, set_bled, HAL_LED_ON);
}

static void enter_error(wm_context_t *ctx) {
    ctx->state = WM_STATE_ERROR;

    CALL_HAL(&ctx->callbacks, set_motor, HAL_MOTOR_OFF);
    CALL_HAL(&ctx->callbacks, set_water_valve, false);
    CALL_HAL(&ctx->callbacks, set_drain_pump, false);
    CALL_HAL(&ctx->callbacks, set_door_lock, false);
    CALL_HAL(&ctx->callbacks, set_bled, HAL_LED_OFF);
    CALL_HAL(&ctx->callbacks, set_rled, HAL_LED_BLINK_2HZ);
}

void wm_fsm_init(wm_context_t *ctx, const hal_output_callbacks_t *callbacks) {
    if (!ctx) {
        return;
    }

    ctx->coin_balance_cents = 0;
    ctx->remaining_cycle_sec = 0;
    ctx->stop_press_count = 0;
    ctx->stop_window_timer_ms = 0;
    ctx->active_error_flags = 0;
    ctx->cycle_duration_setting = WM_CYCLE_DURATION_SEC;

    if (callbacks) {
        ctx->callbacks = *callbacks;
    } else {
        /* Initialize empty callbacks */
        ctx->callbacks.set_rled = NULL;
        ctx->callbacks.set_bled = NULL;
        ctx->callbacks.set_motor = NULL;
        ctx->callbacks.set_water_valve = NULL;
        ctx->callbacks.set_drain_pump = NULL;
        ctx->callbacks.set_door_lock = NULL;
        ctx->callbacks.on_cycle_complete = NULL;
    }

    enter_standby(ctx);
}

static bool handle_coin_deposit(wm_context_t *ctx, uint32_t amount) {
    if (ctx->state == WM_STATE_STANDBY) {
        ctx->coin_balance_cents += amount;
        if (ctx->coin_balance_cents >= WM_COIN_THRESHOLD_CENTS) {
            enter_ready(ctx);
        }
        return true;
    } else if (ctx->state == WM_STATE_READY) {
        /* Surplus money is accepted, but will be cleared upon RUN without refund */
        ctx->coin_balance_cents += amount;
        return true;
    }
    /* Ignored in RUNNING, PAUSED, ERROR */
    return false;
}

static bool handle_stop_button(wm_context_t *ctx) {
    if (ctx->state == WM_STATE_STANDBY) {
        return false;
    }

    if (ctx->stop_press_count == 0) {
        /* First press: start double-click window */
        ctx->stop_press_count = 1;
        ctx->stop_window_timer_ms = WM_DOUBLE_PRESS_WINDOW_MS;
        return true;
    } else {
        /* Second press within window: Force Stop confirmed! */
        ctx->stop_press_count = 0;
        ctx->stop_window_timer_ms = 0;
        ctx->coin_balance_cents = 0;
        enter_standby(ctx);
        return true;
    }
}

bool wm_fsm_dispatch_event(wm_context_t *ctx, wm_event_t event) {
    if (!ctx) {
        return false;
    }

    /* Global emergency error transition */
    if (event == WM_EVT_FAULT_OCCURRED) {
        if (ctx->state != WM_STATE_ERROR) {
            enter_error(ctx);
            return true;
        }
        return false;
    }

    /* System 1ms tick handling */
    if (event == WM_EVT_TIMER_TICK_1MS) {
        wm_fsm_tick_1ms(ctx);
        return true;
    }

    /* Dispatch based on current state */
    switch (ctx->state) {
        case WM_STATE_STANDBY:
            switch (event) {
                case WM_EVT_COIN_10:
                    return handle_coin_deposit(ctx, 10);
                case WM_EVT_COIN_20:
                    return handle_coin_deposit(ctx, 20);
                case WM_EVT_COIN_50:
                    return handle_coin_deposit(ctx, 50);
                default:
                    /* All button presses ignored in Standby */
                    return false;
            }

        case WM_STATE_READY:
            switch (event) {
                case WM_EVT_COIN_10:
                    return handle_coin_deposit(ctx, 10);
                case WM_EVT_COIN_20:
                    return handle_coin_deposit(ctx, 20);
                case WM_EVT_COIN_50:
                    return handle_coin_deposit(ctx, 50);
                case WM_EVT_BTN_RUN:
                    enter_running(ctx, false);
                    return true;
                case WM_EVT_BTN_STOP:
                    return handle_stop_button(ctx);
                default:
                    return false;
            }

        case WM_STATE_RUNNING:
            switch (event) {
                case WM_EVT_BTN_PAUSE:
                    enter_paused(ctx);
                    return true;
                case WM_EVT_BTN_STOP:
                    return handle_stop_button(ctx);
                case WM_EVT_TIMER_TICK_1S:
                    if (ctx->remaining_cycle_sec > 1) {
                        ctx->remaining_cycle_sec--;
                        return true;
                    } else if (ctx->remaining_cycle_sec == 1) {
                        ctx->remaining_cycle_sec = 0;
                        CALL_HAL_VOID(&ctx->callbacks, on_cycle_complete);
                        enter_standby(ctx);
                        return true;
                    }
                    return false;
                default:
                    return false;
            }

        case WM_STATE_PAUSED:
            switch (event) {
                case WM_EVT_BTN_RUN:
                    enter_running(ctx, true);
                    return true;
                case WM_EVT_BTN_STOP:
                    return handle_stop_button(ctx);
                case WM_EVT_TIMER_TICK_1S:
                    /* CRITICAL REQUIREMENT: Timer continues counting down in PAUSED state! */
                    if (ctx->remaining_cycle_sec > 1) {
                        ctx->remaining_cycle_sec--;
                        return true;
                    } else if (ctx->remaining_cycle_sec == 1) {
                        ctx->remaining_cycle_sec = 0;
                        CALL_HAL_VOID(&ctx->callbacks, on_cycle_complete);
                        enter_standby(ctx);
                        return true;
                    }
                    return false;
                default:
                    return false;
            }

        case WM_STATE_ERROR:
            if (event == WM_EVT_FAULT_CLEARED) {
                ctx->coin_balance_cents = 0;
                enter_standby(ctx);
                return true;
            }
            return false;

        default:
            return false;
    }
}

void wm_fsm_tick_1ms(wm_context_t *ctx) {
    if (!ctx) {
        return;
    }

    /* Decrement double-press sliding window */
    if (ctx->stop_window_timer_ms > 0) {
        ctx->stop_window_timer_ms--;
        if (ctx->stop_window_timer_ms == 0) {
            /* Double-press window expired: discard single press */
            ctx->stop_press_count = 0;
        }
    }
}

void wm_fsm_tick_1s(wm_context_t *ctx) {
    wm_fsm_dispatch_event(ctx, WM_EVT_TIMER_TICK_1S);
}

const char* wm_state_to_str(wm_state_t state) {
    switch (state) {
        case WM_STATE_STANDBY: return "STANDBY";
        case WM_STATE_READY:   return "READY";
        case WM_STATE_RUNNING: return "RUNNING";
        case WM_STATE_PAUSED:  return "PAUSED";
        case WM_STATE_ERROR:   return "ERROR";
        default:               return "UNKNOWN";
    }
}

const char* wm_event_to_str(wm_event_t event) {
    switch (event) {
        case WM_EVT_NONE:           return "EVT_NONE";
        case WM_EVT_COIN_10:        return "COIN_10c";
        case WM_EVT_COIN_20:        return "COIN_20c";
        case WM_EVT_COIN_50:        return "COIN_50c";
        case WM_EVT_BTN_RUN:        return "BTN_RUN";
        case WM_EVT_BTN_PAUSE:      return "BTN_PAUSE";
        case WM_EVT_BTN_STOP:       return "BTN_STOP";
        case WM_EVT_TIMER_TICK_1S:  return "TICK_1S";
        case WM_EVT_TIMER_TICK_1MS: return "TICK_1MS";
        case WM_EVT_FAULT_OCCURRED: return "FAULT_OCCURRED";
        case WM_EVT_FAULT_CLEARED:  return "FAULT_CLEARED";
        default:                    return "UNKNOWN_EVENT";
    }
}

wm_state_t wm_fsm_get_state(const wm_context_t *ctx) {
    return ctx ? ctx->state : WM_STATE_STANDBY;
}

uint32_t wm_fsm_get_balance(const wm_context_t *ctx) {
    return ctx ? ctx->coin_balance_cents : 0;
}

uint32_t wm_fsm_get_remaining_seconds(const wm_context_t *ctx) {
    return ctx ? ctx->remaining_cycle_sec : 0;
}

