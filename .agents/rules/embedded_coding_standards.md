# Rule: Embedded Coding & Architectural Standards

## 1. Core Principles
All code developed for BTL 2 must conform to professional embedded systems engineering practices:
- **MISRA-C / Embedded Clean Code Compliant**: Explicit types (`stdint.h`), bounded loops, zero dynamic allocation (`malloc`/`free` prohibited).
- **Strict Separation of Concerns**: Logic $\leftrightarrow$ HAL $\leftrightarrow$ Simulator.
- **Portability**: The core FSM engine must compile on GCC, Clang, Keil ARM, and MSVC without any target-specific compiler extensions.

---

## 2. Directory & Module Structure
```text
src/
├── include/
│   ├── washing_machine_fsm.h       # Public FSM API, state enums, event types
│   ├── hal_gpio.h                  # Abstract button, LED, and sensor interface
│   ├── hal_timer.h                 # Abstract system tick and timer interface
│   └── washing_machine_config.h    # Configurable timings (30 min, debounce, double-click)
├── fsm/
│   └── washing_machine_fsm.c       # Deterministic FSM implementation
├── hal/
│   ├── hal_gpio.c                  # GPIO implementation (or virtual mock)
│   └── hal_timer.c                 # Timer implementation (1 ms tick system)
└── main.c                          # Super-loop / event dispatcher demonstration
```

---

## 3. Coding Guidelines & Invariants

1. **State & Event Definitions (Strongly Typed Enums)**:
   ```c
   typedef enum {
       STATE_STANDBY = 0,
       STATE_READY,
       STATE_RUNNING,
       STATE_PAUSED,
       STATE_ERROR
   } wm_state_t;

   typedef enum {
       EVT_NONE = 0,
       EVT_COIN_10,
       EVT_COIN_20,
       EVT_COIN_50,
       EVT_BTN_RUN,
       EVT_BTN_PAUSE,
       EVT_BTN_STOP,
       EVT_TIMER_TICK_1S,
       EVT_ERROR_RAISED,
       EVT_ERROR_RESET
   } wm_event_t;
   ```

2. **FSM Context Struct**:
   State data must be encapsulated in a single context struct (no loose static global variables polluting global scope):
   ```c
   typedef struct {
       wm_state_t current_state;
       uint32_t coin_balance_cents;
       uint32_t remaining_cycle_sec;
       uint32_t stop_press_count;
       uint32_t stop_window_timer_ms;
       uint32_t error_code;
       bool is_blinking_rled;
       bool is_blinking_bled;
   } wm_context_t;
   ```

3. **Non-Blocking Button Debounce & Double-Click Logic**:
   - Buttons must be sampled with a 20 ms–50 ms software debounce filter.
   - Double-click detection must use an asynchronous timeout counter decremented in the periodic system tick interrupt.

4. **LED Blinker Engine**:
   - An independent tick handler toggles blinking LEDs at their specified frequency (e.g. 1 Hz for BLED washing, 2 Hz for RLED error) without stalling FSM state transitions.
