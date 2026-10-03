/**
 * @file washing_machine_config.h
 * @brief System configuration constants for Washing Machine Control Unit (CO3053 - BTL 2)
 */

#ifndef WASHING_MACHINE_CONFIG_H
#define WASHING_MACHINE_CONFIG_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Full cycle duration in seconds (30 minutes = 1800s as per specification).
 */
#define WM_CYCLE_DURATION_SEC          (1800U)

/**
 * @brief Scaled cycle duration used during accelerated unit testing (30s).
 */
#define WM_CYCLE_DURATION_TEST_SEC     (30U)

/**
 * @brief Minimum accumulated coin balance required to reach READY state (50 cents).
 */
#define WM_COIN_THRESHOLD_CENTS        (50U)

/**
 * @brief Time window to register a double-press on the STOP button (1500 ms = 1.5s).
 */
#define WM_DOUBLE_PRESS_WINDOW_MS      (1500U)

/**
 * @brief Software debounce filter duration for digital button inputs (30 ms).
 */
#define WM_DEBOUNCE_TIME_MS            (30U)

/**
 * @brief LED blinking period in RUNNING state (1.0 Hz -> 500ms ON / 500ms OFF).
 */
#define WM_BLED_BLINK_PERIOD_MS        (1000U)

/**
 * @brief LED blinking period in ERROR state (2.0 Hz -> 250ms ON / 250ms OFF).
 */
#define WM_RLED_BLINK_PERIOD_MS        (500U)

#ifdef __cplusplus
}
#endif

#endif /* WASHING_MACHINE_CONFIG_H */
