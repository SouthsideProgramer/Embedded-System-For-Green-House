# Reference: Washing Machine FSM State Transition Table

## 1. Summary of States & Invariant Outputs

| State Name | RLED | BLED | Actuator Motor | Water Valve | Drain Pump | 30-min Timer | Condition Description |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| **`STANDBY`** | **ON** | **OFF** | OFF | Closed | OFF | Stopped (0) | Initial power-on state; available to serve; waiting for deposit. |
| **`READY`** | **OFF** | **ON** | OFF | Closed | OFF | Stopped (0) | Deposit accumulated $\ge 50$¢; waiting for user to press `RUN`. |
| **`RUNNING`** | **OFF** | **BLINK (1Hz)** | Active | Cycled | Cycled | **Counting Down** | Washing cycle active; timer decrementing every second. |
| **`PAUSED`** | **OFF** | **ON / OFF** | OFF | Closed | OFF | **Counting Down** | Suspended by user; motor stopped; timer **still counting down**! |
| **`ERROR`** | **BLINK (2Hz)** | **OFF** | OFF | Closed | OFF | Suspended | Critical fault detected (lid open, motor stall, sensor timeout). |

---

## 2. Complete State Transition Matrix

| Current State | Triggering Event / Condition | Guard Condition | Next State | Actions Executed on Transition |
| :--- | :--- | :--- | :--- | :--- |
| **`STANDBY`** | `COIN_10` / `COIN_20` / `COIN_50` | `balance + coin < 50` | `STANDBY` | `balance += coin` |
| **`STANDBY`** | `COIN_10` / `COIN_20` / `COIN_50` | `balance + coin >= 50` | `READY` | `balance += coin`, `SET_RLED(OFF)`, `SET_BLED(ON)` |
| **`STANDBY`** | `BTN_RUN` / `BTN_PAUSE` / `BTN_STOP` | None | `STANDBY` | None (Ignored) |
| **`STANDBY`** | `FAULT_TRIGGER` | None | `ERROR` | `SET_RLED(BLINK)`, `SET_BLED(OFF)` |
| **`READY`** | `COIN_10` / `COIN_20` / `COIN_50` | None | `READY` | `balance += coin` (surplus accepted, no refund) |
| **`READY`** | `BTN_RUN` | None | **`RUNNING`** | `balance = 0` (money cleared), `timer = 1800s`, `SET_BLED(BLINK)`, `START_CYCLE()` |
| **`READY`** | `BTN_STOP_DOUBLE` | None | `STANDBY` | `balance = 0`, `SET_BLED(OFF)`, `SET_RLED(ON)` (cancel before run) |
| **`READY`** | `FAULT_TRIGGER` | None | `ERROR` | `SET_BLED(OFF)`, `SET_RLED(BLINK)` |
| **`RUNNING`** | `TIMER_TICK` (1s) | `timer > 1` | `RUNNING` | `timer--` |
| **`RUNNING`** | `TIMER_TICK` (1s) | `timer == 1` | **`STANDBY`** | `timer = 0`, `STOP_ALL_ACTUATORS()`, `SET_BLED(OFF)`, `SET_RLED(ON)` |
| **`RUNNING`** | `BTN_PAUSE` | None | **`PAUSED`** | `STOP_ACTUATORS()`, `SET_BLED(ON)` (Timer continues running) |
| **`RUNNING`** | `BTN_STOP` (1st press) | None | `RUNNING` | `stop_count = 1`, `start_double_press_timer(1.5s)` |
| **`RUNNING`** | `BTN_STOP` (2nd press) | `stop_count == 1 && window active` | **`STANDBY`** | `STOP_ALL_ACTUATORS()`, `timer = 0`, `SET_BLED(OFF)`, `SET_RLED(ON)` |
| **`RUNNING`** | `FAULT_TRIGGER` | None | `ERROR` | `STOP_ALL_ACTUATORS()`, `SET_BLED(OFF)`, `SET_RLED(BLINK)` |
| **`PAUSED`** | `TIMER_TICK` (1s) | `timer > 1` | `PAUSED` | `timer--` (Timer keeps ticking!) |
| **`PAUSED`** | `TIMER_TICK` (1s) | `timer == 1` | **`STANDBY`** | `timer = 0`, `SET_BLED(OFF)`, `SET_RLED(ON)` (Timeout during pause) |
| **`PAUSED`** | `BTN_RUN` | None | **`RUNNING`** | `RESUME_ACTUATORS()`, `SET_BLED(BLINK)` |
| **`PAUSED`** | `BTN_STOP` (2nd press) | `stop_count == 1 && window active` | **`STANDBY`** | `timer = 0`, `SET_BLED(OFF)`, `SET_RLED(ON)` |
| **`PAUSED`** | `FAULT_TRIGGER` | None | `ERROR` | `SET_BLED(OFF)`, `SET_RLED(BLINK)` |
| **`ERROR`** | `ERROR_RESET_BTN` | Fault cleared | `STANDBY` | `SET_RLED(ON)`, `SET_BLED(OFF)` |
