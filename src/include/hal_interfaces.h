/**
 * @file hal_interfaces.h
 * @brief Hardware Abstraction Layer interfaces for Washing Machine Control Unit
 */

#ifndef HAL_INTERFACES_H
#define HAL_INTERFACES_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    HAL_LED_OFF = 0,
    HAL_LED_ON,
    HAL_LED_BLINK_1HZ,  /* 1.0 Hz: 500ms ON / 500ms OFF */
    HAL_LED_BLINK_2HZ   /* 2.0 Hz: 250ms ON / 250ms OFF */
} hal_led_state_t;

typedef enum {
    HAL_MOTOR_OFF = 0,
    HAL_MOTOR_AGITATE,  /* Wash agitation */
    HAL_MOTOR_SPIN      /* Spin dry */
} hal_motor_state_t;

/**
 * @brief Function pointer table for hardware output actions.
 *        Decouples FSM logic completely from target platform registers.
 */
typedef struct {
    void (*set_rled)(hal_led_state_t state);
    void (*set_bled)(hal_led_state_t state);
    void (*set_motor)(hal_motor_state_t state);
    void (*set_water_valve)(bool open);
    void (*set_drain_pump)(bool on);
    void (*set_door_lock)(bool locked);
    void (*on_cycle_complete)(void);
} hal_output_callbacks_t;

#ifdef __cplusplus
}
#endif

#endif /* HAL_INTERFACES_H */
