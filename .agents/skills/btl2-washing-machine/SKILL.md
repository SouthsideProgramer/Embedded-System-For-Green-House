---
name: btl2-washing-machine
description: >-
  Use this skill whenever working on BTL 2 (CO3053 - Embedded Systems): Designing, implementing,
  simulating, verifying, or writing documentation for the Coin-Operated Washing Machine Control Unit.
---

# BTL 2: Washing Machine Control Unit Skill

This skill provides step-by-step procedures, mathematical models, state transition tables, and reference architectures for executing BTL 2.

## Quick References
- [Complete State Transition Table](./references/state_transition_table.md)
- [Timing, Debounce & Concurrency Specifications](./references/timing_and_concurrency.md)
- [Layered Implementation & HAL Architecture](./references/implementation_architecture.md)

---

## Standard Workflow

### Phase 1: Formal System Modeling & FSM Verification
1. Consult [State Transition Table](./references/state_transition_table.md) to inspect the 5 core states:
   `STANDBY`, `READY`, `RUNNING`, `PAUSED`, and `ERROR`.
2. Generate the State Diagram in Mermaid or TikZ for the assignment report.
3. Validate that all 4 critical problem constraints are met:
   - Coin balance threshold: $\ge 50$ cents.
   - Money clearance upon `RUN` with zero refunds.
   - Timer continues counting down during `PAUSED`.
   - Force termination only on `STOP` pressed twice within the double-click window ($T_{\text{double}} \le 1.5\,\text{s}$).

### Phase 2: Core FSM Engine Implementation
1. Define clean data structures in `washing_machine_fsm.h` (context, states, events).
2. Implement state transition logic using standard state-table or switch-case pattern in `washing_machine_fsm.c`.
3. Ensure zero blocking delays (`delay()` is strictly forbidden).

### Phase 3: Hardware Abstraction & Timing Infrastructure
1. Implement a 1 ms tick timer system (`hal_timer.c`).
2. Implement button debouncing and asynchronous edge detection with double-click window tracking.
3. Implement non-blocking LED blink generators for:
   - `BLED` at 1.0 Hz (500 ms ON, 500 ms OFF) when in `RUNNING`.
   - `RLED` at 2.0 Hz (250 ms ON, 250 ms OFF) when in `ERROR`.

### Phase 4: Automated Verification & Testbench
1. Execute unit test runner covering all 13 test scenarios in `test_and_verification_rules.md`.
2. Verify boundary cases:
   - Deposition of 10¢ + 20¢ + 20¢ = 50¢ (exact boundary).
   - Deposition of 50¢ + 50¢ = 100¢ (surplus boundary).
   - `PAUSE` state timing out automatically when 30 minutes expire.
   - Single press of `STOP` followed by 2.0s delay (must NOT stop).

### Phase 5: Documentation & Report Generation
1. Write the formal academic report in LaTeX matching the HCMUT standard template.
2. Include:
   - Executive Overview & Problem Definition.
   - Mathematical FSM Formalism & Statechart Diagram.
   - Software Architecture & Modular Design.
   - Verification Test Matrix & Simulation Results.
   - Edge Case & Safety Analysis.
