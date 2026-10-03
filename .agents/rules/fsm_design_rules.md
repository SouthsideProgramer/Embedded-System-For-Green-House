# Rule: FSM Design & Mathematical Formalism for Washing Machine Control Unit

## 1. Formal Definition of the FSM
The Control Unit is modeled as a 6-tuple Moore-Mealy hybrid state machine:
$$M = (S, \Sigma, \Gamma, \delta, \lambda, s_0)$$
where:
- **$S$ (State Set)**:
  - $s_0 = \text{STANDBY}$: Machine available, waiting for coin insertion.
  - $s_1 = \text{READY}$: Sufficient deposit ($\ge 50$¢), waiting for `RUN`.
  - $s_2 = \text{RUNNING}$: Actuators operating, 30-min timer ticking down, BLED blinking.
  - $s_3 = \text{PAUSED}$: Actuators paused, 30-min timer continues ticking down, BLED solid or off.
  - $s_4 = \text{ERROR}$: System fault detected, actuators off, RLED blinking.
- **$\Sigma$ (Input Alphabet / Events)**:
  - `EVT_COIN_10`, `EVT_COIN_20`, `EVT_COIN_50`
  - `EVT_BTN_RUN`
  - `EVT_BTN_PAUSE`
  - `EVT_BTN_STOP_SINGLE`, `EVT_BTN_STOP_DOUBLE`
  - `EVT_TIMER_30MIN_EXPIRED`
  - `EVT_ERROR_OCCURRED`, `EVT_ERROR_CLEARED`
- **$\Gamma$ (Output Actions)**:
  - LED commands: `SET_RLED(ON|OFF|BLINK)`, `SET_BLED(ON|OFF|BLINK)`
  - Actuator commands: `SET_MOTOR(STOP|AGITATE|SPIN)`, `SET_VALVE(OPEN|CLOSE)`, `SET_PUMP(ON|OFF)`
  - Timer commands: `START_30MIN_TIMER()`, `CLEAR_MONEY()`
- **$\delta: S \times \Sigma \rightarrow S$**: Deterministic state transition function.
- **$\lambda: S \rightarrow \Gamma$**: Moore output function for steady visual indicators + Mealy transition triggers for clearing money and starting timers.

---

## 2. Mandatory State Transition Rules

1. **Self-Loop on Underfunded Deposits**:
   - In `STANDBY`, receiving `EVT_COIN_10` or `EVT_COIN_20` when `balance < 50` updates the internal counter without changing state:
     $$\delta(\text{STANDBY}, \text{coin}) = \begin{cases} \text{READY} & \text{if } \text{balance} + \text{coin} \ge 50 \\ \text{STANDBY} & \text{otherwise} \end{cases}$$

2. **Surplus Absorption in Ready State**:
   - In `READY`, if more coins are inserted, balance accumulates, but no refunds occur.
   - When $\delta(\text{READY}, \text{EVT\_BTN\_RUN}) \rightarrow \text{RUNNING}$ triggers:
     - `balance := 0` (immediate clear).
     - `remaining_seconds := 1800` (30 minutes).

3. **Continuous Clock Invariance during Pause**:
   - The countdown timer is a hardware or software interrupt-driven clock decoupled from actuator state:
     $$\frac{d}{dt}(\text{Timer}) = -1 \quad \forall t \in [\text{RUNNING}, \text{PAUSED}]$$
   - Transitioning between `RUNNING` and `PAUSED` MUST NOT restart, pause, or reload `Timer`.

4. **Double-Stop Detection Rule**:
   - The first `EVT_BTN_STOP` enters a pending detection state $T_{\text{stop\_window}} = 1.5\,\text{s}$.
   - If a 2nd press arrives before $T_{\text{stop\_window}}$ expires:
     $$\delta(\text{RUNNING} \mid \text{PAUSED}, \text{EVT\_BTN\_STOP\_DOUBLE}) \rightarrow \text{STANDBY}$$
     - All actuators immediately de-energize.
     - 30-minute timer canceled.
     - Returns to `STANDBY` (`RLED = ON`).
   - If $T_{\text{stop\_window}}$ expires without 2nd press: Discard pending flag; stay in current state.

5. **Exhaustive Default Handling**:
   - Any unspecified event in any state must be ignored explicitly (no unhandled fall-throughs).
   - In C implementations, every `switch(state)` MUST contain `default: break;` or assert failure to comply with safety-critical guidelines.
