# Rule: Verification, Unit Testing & Edge-Case Coverage for BTL 2

## 1. Testing Philosophy
To guarantee 100% correctness and satisfy even the most rigorous grading rubric, every functional requirement and edge case must have an automated test scenario in the test harness.

---

## 2. Mandatory Test Suite (Test Matrix)

| Test ID | Test Scenario | Initial Condition | Actions / Inputs | Expected Output & Final State |
| :--- | :--- | :--- | :--- | :--- |
| **TC-01** | Sub-threshold Deposit | `STANDBY` ($0¢) | Insert 10¢, then 20¢ | Balance = 30¢, State remains `STANDBY`, `RLED=ON`, `BLED=OFF`. |
| **TC-02** | Exact Threshold Deposit | `STANDBY` ($0¢) | Insert 50¢ | Balance = 50¢, State transitions to `READY`, `RLED=OFF`, `BLED=ON`. |
| **TC-03** | Surplus Deposit Accumulation | `STANDBY` ($0¢) | Insert 20¢, 20¢, 20¢ | Balance = 60¢, State transitions to `READY`, `BLED=ON`. |
| **TC-04** | Execution & Money Clearance | `READY` ($60¢) | Press `RUN` | Balance resets to **0¢** immediately (no refund), Timer = 1800s, State $\rightarrow$ `RUNNING`, `BLED=BLINKING`. |
| **TC-05** | Premature RUN Attempt | `STANDBY` ($30¢) | Press `RUN` | Ignored. State remains `STANDBY`, timer does not start. |
| **TC-06** | Normal Pause & Resume | `RUNNING` (Timer=1700s) | Press `PAUSE` | State $\rightarrow$ `PAUSED`, Motor halts. Timer continues to 1690s. Press `RUN` $\rightarrow$ State $\rightarrow$ `RUNNING`. |
| **TC-07** | **Persistent Timer in Pause (Critical)** | `PAUSED` (Timer=10s) | Wait 10 seconds without pressing RUN | Timer reaches 0 $\rightarrow$ Cycle terminates, State $\rightarrow$ `STANDBY`, `RLED=ON`. |
| **TC-08** | Single Stop Ignores | `RUNNING` (Timer=1500s) | Press `STOP` once, wait 2.0s | Machine stays in `RUNNING`. Cycle continues normally. |
| **TC-09** | **Force Stop on Double Press** | `RUNNING` (Timer=1500s) | Press `STOP`, wait 300ms, press `STOP` again | State immediately transitions to `STANDBY`, all actuators halted, timer killed, `RLED=ON`. |
| **TC-10** | Force Stop from Paused State | `PAUSED` (Timer=1200s) | Press `STOP` twice within 1.0s | State immediately transitions to `STANDBY`, `RLED=ON`. |
| **TC-11** | Normal 30-min Completion | `RUNNING` (Timer=1s) | 1s tick expires | Timer reaches 0 $\rightarrow$ Transition to `STANDBY`, `RLED=ON`, `BLED=OFF`. |
| **TC-12** | Fault Handling | `RUNNING` or `PAUSED` | Fault trigger (e.g. Lid open) | State $\rightarrow$ `ERROR`, `RLED=BLINKING`, `BLED=OFF`, actuators cut. |
| **TC-13** | Error Reset | `ERROR` | Error cleared / Reset signal | State returns to `STANDBY`, `RLED=ON`. |

---

## 3. Automation Standard
- Unit tests must be written in a lightweight C unit testing framework (or a Python simulation harness) that executes in CI or local command line with exit code 0 on all tests passing.
- Provide clear colored terminal output (`[PASS]`, `[FAIL]`) for all test cases.
