/**
 * @file mock_hal.c
 * @brief Implementation of virtual mock hardware for test verification
 */

#include "mock_hal.h"
#include <string.h>

static mock_hal_state_t g_mock_state;

static void mock_set_rled(hal_led_state_t state) {
    g_mock_state.rled = state;
}

static void mock_set_bled(hal_led_state_t state) {
    g_mock_state.bled = state;
}

static void mock_set_motor(hal_motor_state_t state) {
    g_mock_state.motor = state;
}

static void mock_set_water_valve(bool open) {
    g_mock_state.water_valve_open = open;
}

static void mock_set_drain_pump(bool on) {
    g_mock_state.drain_pump_on = on;
}

static void mock_set_door_lock(bool locked) {
    g_mock_state.door_locked = locked;
}

static void mock_on_cycle_complete(void) {
    g_mock_state.cycle_complete_count++;
}

void mock_hal_reset(void) {
    memset(&g_mock_state, 0, sizeof(g_mock_state));
}

const mock_hal_state_t* mock_hal_get_state(void) {
    return &g_mock_state;
}

hal_output_callbacks_t mock_hal_get_callbacks(void) {
    hal_output_callbacks_t cbs = {
        .set_rled = mock_set_rled,
        .set_bled = mock_set_bled,
        .set_motor = mock_set_motor,
        .set_water_valve = mock_set_water_valve,
        .set_drain_pump = mock_set_drain_pump,
        .set_door_lock = mock_set_door_lock,
        .on_cycle_complete = mock_on_cycle_complete
    };
    return cbs;
}
