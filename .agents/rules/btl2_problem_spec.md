# Rule: BTL 2 Washing Machine Problem Specification

## 1. Specification Overview
Design and implement the embedded control unit of a coin-operated commercial washing machine.

## 2. Input Interface
- **Buttons (Digital Inputs)**:
  - `BTN_STOP`: Force cancellation / emergency stop (requires two presses).
  - `BTN_RUN`: Start cycle from Ready state; Resume cycle from Paused state.
  - `BTN_PAUSE`: Temporarily suspend washing actuation while preserving cycle timer countdown.
- **Coin Acceptor (Event Inputs)**:
  - `COIN_10`: 10-cent denomination detected.
  - `COIN_20`: 20-cent denomination detected.
  - `COIN_50`: 50-cent denomination detected.
  - Invalid coin denominations must be rejected by hardware/software.
- **Fault Sensors (Internal / Simulation Inputs)**:
  - `SENSOR_DOOR_LATCH`: Indicates whether lid/door is safely locked.
  - `SENSOR_WATER_TIMEOUT`: Inlet water failure sensor.
  - `SENSOR_MOTOR_OVERCURRENT`: Motor fault sensor.

## 3. Output Interface
- **LED Indicators (Visual Status)**:
  - **Red LED (`RLED`)**:
    - `ON (Solid)`: Machine in Standby mode (available to serve, waiting for customer/deposit).
    - `BLINKING (e.g. 2 Hz)`: Error condition present.
    - `OFF`: When in Ready or Running modes without error.
  - **Blue LED (`BLED`)**:
    - `ON (Solid)`: Ready to execute ($\ge 50$ cents deposited).
    - `BLINKING (e.g. 1 Hz)`: Running (active washing/spinning in progress).
    - `OFF`: When in Standby or Error modes.
- **Actuators (Controlled Loads)**:
  - `ACT_MOTOR`: Main wash/spin drum motor (`OFF`, `WASH_AGITATION`, `SPIN_DRY`).
  - `ACT_WATER_INLET`: Water fill solenoid valve.
  - `ACT_DRAIN_PUMP`: Wastewater discharge pump.
  - `ACT_DOOR_LOCK`: Safety interlock solenoid.

## 4. Operational Logic & Critical Constraints
1. **Coin Deposit & Clearing**:
   - Machine will not allow execution until accumulated balance is $\ge 50$ cents.
   - When balance reaches $\ge 50$ cents, machine immediately transitions to `READY` state (`BLED = ON`).
   - If user deposits surplus coins (e.g., 60¢, 70¢, 100¢), the system accepts them, but when `RUN` is pressed, **the entire balance is cleared to 0 without returning any redundancy**.
2. **Cycle Timing & Pause Behavior**:
   - Pressing `RUN` while in `READY` state activates the **30-minute washing cycle countdown timer**.
   - Pressing `PAUSE` while in `RUNNING` suspends motor/actuator operation, but **the 30-minute countdown timer MUST CONTINUE COUNTING DOWN**.
   - If `RUN` is pressed again while in `PAUSED`, machine resumes washing until the timer expires.
   - If user remains in `PAUSED` until the 30-minute timer expires, the machine terminates automatically and returns to `STANDBY`.
3. **Termination Conditions**:
   - Normal completion: 30 minutes expire $\rightarrow$ All actuators shut off $\rightarrow$ Return to `STANDBY` (`RLED = ON`, `BLED = OFF`).
   - Force stop: `STOP` button is pressed **twice** $\rightarrow$ Immediate termination $\rightarrow$ Return to `STANDBY`.
   - Single press of `STOP`: Must be buffered / timed; if a second press does not occur within $T_{\text{double\_press}}$ (default 1.5 s), the single press is discarded.
4. **Fault & Error Handling**:
   - Any critical safety violation (e.g. lid opened during spin, motor stall) transitions system to `ERROR` (`RLED = BLINKING`, `BLED = OFF`, all actuators de-energized).
