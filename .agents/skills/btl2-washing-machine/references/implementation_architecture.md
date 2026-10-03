# Reference: Layered Implementation & HAL Architecture

## 1. Architectural Layers

```text
+-------------------------------------------------------------------+
|               Top-Level Application / Simulator / UI              |
|        (CLI Interactive Simulator, Automated Unit Tests, GUI)     |
+-------------------------------------------------------------------+
                                  |
                                  v
+-------------------------------------------------------------------+
|                  Washing Machine FSM Core Engine                  |
|    - State Transitions (STANDBY, READY, RUNNING, PAUSED, ERROR)   |
|    - 30-min Countdown Clock Management                            |
|    - Coin Accounting & Clearance Logic                            |
|    - Double-Stop Verification Logic                               |
+-------------------------------------------------------------------+
                                  |
                                  v
+-------------------------------------------------------------------+
|                     Hardware Abstraction Layer                    |
|   hal_led.h / hal_btn.h / hal_coin.h / hal_timer.h / hal_act.h    |
+-------------------------------------------------------------------+
                                  |
         +------------------------+------------------------+
         |                                                 |
         v                                                 v
+-----------------------------------+   +-----------------------------------+
|     Target Hardware Platform      |   |       Virtual Simulator Host      |
| (STM32 HAL / Arduino / ESP32 /    |   | (POSIX / Windows / Python C-FFI / |
|  Bare-metal ARM Cortex-M)         |   |  Interactive Terminal Mock)       |
+-----------------------------------+   +-----------------------------------+
```

---

## 2. HAL Interface Contracts

### LED Driver Interface (`hal_led.h`)
```c
typedef enum {
    LED_RED = 0,
    LED_BLUE
} hal_led_id_t;

typedef enum {
    LED_MODE_OFF = 0,
    LED_MODE_ON,
    LED_MODE_BLINK_SLOW, // 1 Hz (500ms ON / 500ms OFF)
    LED_MODE_BLINK_FAST  // 2 Hz (250ms ON / 250ms OFF)
} hal_led_mode_t;

void hal_led_init(void);
void hal_led_set_mode(hal_led_id_t led, hal_led_mode_t mode);
void hal_led_tick_1ms(void);
```

### Button Driver Interface (`hal_btn.h`)
```c
typedef enum {
    BTN_STOP = 0,
    BTN_RUN,
    BTN_PAUSE
} hal_btn_id_t;

typedef enum {
    BTN_EVT_NONE = 0,
    BTN_EVT_CLICKED,
    BTN_EVT_DOUBLE_CLICKED
} hal_btn_evt_t;

void hal_btn_init(void);
hal_btn_evt_t hal_btn_poll(hal_btn_id_t btn);
void hal_btn_tick_1ms(void);
```

### Coin Acceptor Interface (`hal_coin.h`)
```c
typedef enum {
    COIN_NONE = 0,
    COIN_VAL_10 = 10,
    COIN_VAL_20 = 20,
    COIN_VAL_50 = 50
} hal_coin_t;

void hal_coin_init(void);
hal_coin_t hal_coin_poll(void);
```

---

## 3. Advantages of this Architecture
1. **100% Platform Independence**: The assignment does not mandate a specific board; this architecture runs identically on an STM32 MCU, an Arduino, a Proteus virtual circuit, or a command-line simulator on Windows/Linux.
2. **Instant Testability**: Can run hundreds of automated tests in under a second without physical hardware.
3. **Academic Excellence**: Demonstrates professional software engineering principles directly answering lecture requirements for CO3053.
