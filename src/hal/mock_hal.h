/**
 * @file mock_hal.h
 * @brief Virtual Mock HAL for automated verification and testbenches
 */

#ifndef MOCK_HAL_H
#define MOCK_HAL_H

#include "hal_interfaces.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    hal_led_state_t rled;
    hal_led_state_t bled;
    hal_motor_state_t motor;
    bool water_valve_open;
    bool drain_pump_on;
    bool door_locked;
    uint32_t cycle_complete_count;
} mock_hal_state_t;

/**
 * @brief Reset mock hardware recorder.
 */
void mock_hal_reset(void);

/**
 * @brief Get read-only snapshot of virtual hardware state.
 */
const mock_hal_state_t* mock_hal_get_state(void);

/**
 * @brief Get the standard callback struct bound to this mock HAL.
 */
hal_output_callbacks_t mock_hal_get_callbacks(void);

#ifdef __cplusplus
}
#endif

#endif /* MOCK_HAL_H */
