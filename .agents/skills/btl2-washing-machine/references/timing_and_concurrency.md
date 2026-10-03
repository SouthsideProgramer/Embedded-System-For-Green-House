# Reference: Timing, Debounce & Concurrency Specifications

## 1. System Timebase & Tick Architecture
- System clock tick: $1\,\text{ms}$ periodic interrupt (SysTick or software timer).
- All delays, timeouts, debounces, and blinks derive from this single timebase.

```text
               +--------------------------------------+
               |    Hardware Timer / SysTick (1 ms)   |
               +--------------------------------------+
                                   |
           +-----------------------+-----------------------+
           |                       |                       |
           v                       v                       v
   Button Debounce Engine   LED Blinker Engine      FSM Cycle Timer
   (20 ms filter +          (1 Hz BLED / 2 Hz RLED) (1 sec decrements,
    1.5s double-click window)                        30 min total)
```

---

## 2. Button Debounce & Double-Press Algorithm

### Debounce State Machine (per button)
- Sampling period: $10\,\text{ms}$.
- Stable state confirmation threshold: 3 consecutive identical samples ($30\,\text{ms}$).
- Emits clean edge events: `PRESS_DOWN`, `RELEASE_UP`.

### Double-Press Detection Logic for `STOP` Button
```c
void on_stop_button_pressed(wm_context_t *ctx, uint32_t current_time_ms) {
    if (ctx->stop_press_count == 0) {
        ctx->stop_press_count = 1;
        ctx->last_stop_press_time_ms = current_time_ms;
    } else if (ctx->stop_press_count == 1) {
        if ((current_time_ms - ctx->last_stop_press_time_ms) <= DOUBLE_PRESS_WINDOW_MS) {
            // Valid double-press detected!
            ctx->stop_press_count = 0;
            wm_fsm_dispatch_event(ctx, EVT_BTN_STOP_DOUBLE);
        } else {
            // First press expired; treat this as a new first press
            ctx->last_stop_press_time_ms = current_time_ms;
        }
    }
}

// Background tick checker:
void check_stop_button_timeout(wm_context_t *ctx, uint32_t current_time_ms) {
    if (ctx->stop_press_count == 1) {
        if ((current_time_ms - ctx->last_stop_press_time_ms) > DOUBLE_PRESS_WINDOW_MS) {
            ctx->stop_press_count = 0; // Discard single press
        }
    }
}
```

---

## 3. LED Blinker Timing Parameters
- **`BLED` (Running Indicator)**:
  - Frequency: $1.0\,\text{Hz}$ ($T = 1000\,\text{ms}$)
  - Duty Cycle: $50\%$ (500 ms ON, 500 ms OFF)
- **`RLED` (Error Indicator)**:
  - Frequency: $2.0\,\text{Hz}$ ($T = 500\,\text{ms}$)
  - Duty Cycle: $50\%$ (250 ms ON, 250 ms OFF)

---

## 4. 30-Minute Cycle Timer Integrity
- Total duration: $30\,\text{minutes} = 1{,}800\,\text{seconds}$.
- Scalable simulation factor: For fast testing/simulation, support a simulation scaling macro:
  ```c
  #ifdef SIMULATION_MODE
  #define CYCLE_DURATION_SEC   30   // 30 seconds for testbench
  #else
  #define CYCLE_DURATION_SEC   1800 // 30 minutes for real operation
  #endif
  ```
