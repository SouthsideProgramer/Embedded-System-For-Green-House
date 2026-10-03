/**
 * @file washing_machine_fsm.h
 * @brief Public interface of the Washing Machine Finite State Machine (FSM)
 *        CO3053 - Embedded Systems - Assignment 2 (BTL 2)
 */

#ifndef WASHING_MACHINE_FSM_H
#define WASHING_MACHINE_FSM_H

#include <stdint.h>
#include <stdbool.h>
#include "washing_machine_config.h"
#include "hal_interfaces.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief States of the Washing Machine Control Unit
 */
typedef enum {
    WM_STATE_STANDBY = 0,   /**< Available to serve; RLED=ON, BLED=OFF */
    WM_STATE_READY,         /**< Deposit >= 50¢; RLED=OFF, BLED=ON */
    WM_STATE_RUNNING,       /**< Actuators working; BLED=BLINK(1Hz), Timer ticking */
    WM_STATE_PAUSED,        /**< Actuators stopped; Timer continues ticking down */
    WM_STATE_ERROR          /**< Hardware/safety error; RLED=BLINK(2Hz), Actuators stopped */
} wm_state_t;

/**
 * @brief External events injected into the FSM
 */
typedef enum {
    WM_EVT_NONE = 0,
    WM_EVT_COIN_10,             /**< 10-cent coin deposited */
    WM_EVT_COIN_20,             /**< 20-cent coin deposited */
    WM_EVT_COIN_50,             /**< 50-cent coin deposited */
    WM_EVT_BTN_RUN,             /**< RUN button clicked */
    WM_EVT_BTN_PAUSE,           /**< PAUSE button clicked */
    WM_EVT_BTN_STOP,            /**< STOP button clicked (single pulse) */
    WM_EVT_TIMER_TICK_1S,       /**< 1-second system clock tick */
    WM_EVT_TIMER_TICK_1MS,      /**< 1-millisecond system tick for debounce/double-press */
    WM_EVT_FAULT_OCCURRED,      /**< Safety fault triggered (e.g. lid open during spin) */
    WM_EVT_FAULT_CLEARED        /**< Fault cleared / reset button pressed */
} wm_event_t;

/**
 * @brief Complete state context of the Washing Machine Control Unit
 */
typedef struct {
    wm_state_t state;                   /**< Current FSM state */
    uint32_t coin_balance_cents;        /**< Accumulated coin deposit */
    uint32_t remaining_cycle_sec;       /**< Countdown timer in seconds (starts at 1800s) */
    uint32_t stop_press_count;          /**< Tracks consecutive presses of the STOP button */
    uint32_t stop_window_timer_ms;      /**< Countdown timer for the 1.5s double-click window */
    uint32_t active_error_flags;        /**< Bitmask of active system faults */
    hal_output_callbacks_t callbacks;   /**< HAL output driver callbacks */
    uint32_t cycle_duration_setting;    /**< Configurable total cycle time (default 1800s) */
} wm_context_t;

/**
 * @brief Initialize the washing machine FSM context to default STANDBY state.
 * @param ctx Pointer to FSM context structure.
 * @param callbacks Pointer to HAL output callback table (can be NULL for headless).
 */
void wm_fsm_init(wm_context_t *ctx, const hal_output_callbacks_t *callbacks);

/**
 * @brief Dispatch an event to the FSM.
 * @param ctx Pointer to FSM context structure.
 * @param event The event to process.
 * @return true if state changed or an action was executed; false if ignored.
 */
bool wm_fsm_dispatch_event(wm_context_t *ctx, wm_event_t event);

/**
 * @brief Periodic 1 ms tick handler for double-press window and debouncing.
 * @param ctx Pointer to FSM context structure.
 */
void wm_fsm_tick_1ms(wm_context_t *ctx);

/**
 * @brief Periodic 1 second tick handler for cycle countdown.
 * @param ctx Pointer to FSM context structure.
 */
void wm_fsm_tick_1s(wm_context_t *ctx);

/**
 * @brief Get current FSM state (safe getter).
 */
wm_state_t wm_fsm_get_state(const wm_context_t *ctx);

/**
 * @brief Get accumulated coin balance in cents.
 */
uint32_t wm_fsm_get_balance(const wm_context_t *ctx);

/**
 * @brief Get remaining cycle time in seconds.
 */
uint32_t wm_fsm_get_remaining_seconds(const wm_context_t *ctx);

/**
 * @brief Convert state enum to human-readable string.
 */
const char* wm_state_to_str(wm_state_t state);

/**
 * @brief Convert event enum to human-readable string.
 */
const char* wm_event_to_str(wm_event_t event);

#ifdef __cplusplus
}
#endif

#endif /* WASHING_MACHINE_FSM_H */
