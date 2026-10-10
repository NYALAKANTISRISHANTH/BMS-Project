# Modular Battery Management System (BMS) — Project Checklist

Use this checklist to track implementation, documentation, and verification. Check an item only after the corresponding work has actually been completed or tested.

## 1. Source Code and Repository
- [ ] Upload the final, compile-tested ESP32 source code.
- [ ] Organize source code and supporting files into clear folders.
- [ ] Confirm GPIO assignments in the code match the Wokwi circuit.
- [ ] Remove credentials and private tokens from committed files.
- [ ] Add a clear project README with setup and run instructions.

## 2. Core BMS Functions
- [ ] Verify cell-voltage acquisition from the CD74HC4067 multiplexer.
- [ ] Verify strongest-cell and weakest-cell detection.
- [ ] Verify voltage-imbalance calculation in millivolts.
- [ ] Verify imbalance trend reporting.
- [ ] Verify SoC estimation against known test inputs.
- [ ] Verify adaptive-threshold selection using SoC and current.
- [ ] Verify relay hysteresis and switching behavior.

## 3. Cell-Count Scalability
- [x] Run the Wokwi simulation with 4 active cells out of 16.
- [ ] Test a configuration with 8 active cells.
- [ ] Test a configuration with 12 active cells.
- [ ] Test a configuration with 16 active cells.
- [ ] Record the selected `CELLS` setting and results for each run.

## 4. State Machine and Fault Handling
- [ ] Verify normal, fault-detected, and recovery state transitions.
- [ ] Verify degraded and failsafe responses to simulated faults.
- [ ] Verify recovery timing and return-to-normal conditions.
- [ ] Verify relay is forced to a safe state during severe faults.
- [ ] Record test inputs, expected results, and actual results.

## 5. Local Interface
- [ ] Verify LCD readings and status messages.
- [ ] Verify green, yellow, and red LED indications.
- [ ] Verify buzzer patterns for the relevant states.
- [ ] Verify relay indication matches the software state.

## 6. Wi-Fi, Blynk, and Telemetry
- [x] Establish Wi-Fi and Blynk connectivity in Wokwi.
- [ ] Verify every configured Blynk virtual pin against its intended value.
- [ ] Test behavior during a Wi-Fi/Blynk disconnection.
- [ ] Verify reconnection attempts and offline behavior.
- [ ] Verify telemetry queue handling and queue limits.
- [ ] Capture screenshots or logs that demonstrate successful tests.

## 7. Documentation and Evidence
- [ ] Complete the README with architecture, pin mapping, calculations, and run instructions.
- [ ] Add a Bug Log describing issues found and how they were resolved.
- [ ] Add an Assumptions section, including simulation and measurement limitations.
- [ ] Add a Self-Attestation section describing what was implemented and personally verified.
- [ ] Include Wokwi circuit/project link and relevant screenshots.
- [ ] Include a verification summary with only evidence-backed pass results.

## 8. Future Physical Validation
- [ ] Calibrate voltage and current measurements with suitable hardware.
- [ ] Validate protection behavior using a safe, controlled test setup.
- [ ] Review hardware-level battery protection requirements before connecting real cells.

## Current Verification Notes
- Wokwi run with 4 active cells out of 16 has been reported.
- Wi-Fi/Blynk connectivity was confirmed working after increasing the connection timeout.
- Other items remain unchecked until individually verified.
- This project is currently an ESP32 embedded-firmware simulation; it is not, by itself, a synthesizable RTL implementation.
