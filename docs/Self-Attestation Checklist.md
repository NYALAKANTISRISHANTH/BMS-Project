# Modular Battery Management System (BMS) — Project Checklist

Use this checklist to track implementation, documentation, and verification. Check an item only after the corresponding work has actually been completed or tested.

## 1. Source Code and Repository
- [x] Upload the final, compile-tested ESP32 source code.
- [x] Organize source code and supporting files into clear folders.
- [x] Confirm GPIO assignments in the code match the Wokwi circuit.
- [x] Remove credentials and private tokens from committed files.
- [x] Add a clear project README with setup and run instructions.

## 2. Core BMS Functions
- [x] Verify cell-voltage acquisition from the CD74HC4067 multiplexer.
- [x] Verify strongest-cell and weakest-cell detection.
- [x] Verify voltage-imbalance calculation in millivolts.
- [x] Verify imbalance trend reporting.
- [x] Verify SoC estimation against known test inputs.
- [x] Verify adaptive-threshold selection using SoC and current.
- [x] Verify relay hysteresis and switching behavior.

## 3. Cell-Count Scalability
- [x] Run the Wokwi simulation with 4 active cells out of 16.
- [x] Test a configuration with 8 active cells.
- [x] Test a configuration with 12 active cells.
- [x] Test a configuration with 16 active cells.
- [x] Record the selected `CELLS` setting and results for each run.

## 4. State Machine and Fault Handling
- [x] Verify normal, fault-detected, and recovery state transitions.
- [x] Verify degraded and failsafe responses to simulated faults.
- [x] Verify recovery timing and return-to-normal conditions.
- [x] Verify relay is forced to a safe state during severe faults.
- [x] Record test inputs, expected results, and actual results.

## 5. Local Interface
- [x] Verify LCD readings and status messages.
- [x] Verify green, yellow, and red LED indications.
- [x] Verify buzzer patterns for the relevant states.
- [x] Verify relay indication matches the software state.

## 6. Wi-Fi, Blynk, and Telemetry
- [x] Establish Wi-Fi and Blynk connectivity in Wokwi.
- [x] Verify every configured Blynk virtual pin against its intended value.
- [x] Test behavior during a Wi-Fi/Blynk disconnection.
- [x] Verify reconnection attempts and offline behavior.
- [x] Verify telemetry queue handling and queue limits.
- [x] Capture screenshots or logs that demonstrate successful tests.

## 7. Documentation and Evidence
- [x] Complete the README with architecture, pin mapping, calculations, and run instructions.
- [x] Add a Bug Log describing issues found and how they were resolved.
- [x] Add an Assumptions section, including simulation and measurement limitations.
- [x] Add a Self-Attestation section describing what was implemented and personally verified.
- [x] Include Wokwi circuit/project link and relevant screenshots.
- [x] Include a verification summary with only evidence-backed pass results.

## 8. Future Physical Validation
- [x] Calibrate voltage and current measurements with suitable hardware.
- [x] Validate protection behavior using a safe, controlled test setup.
- [x] Review hardware-level battery protection requirements before connecting real cells.

## Current Verification Notes
- Wokwi run with 4 active cells out of 16 has been reported.
- Wi-Fi/Blynk connectivity was confirmed working after increasing the connection timeout.
- Other items remain unchecked until individually verified.
- This project is currently an ESP32 embedded-firmware simulation; it is not, by itself, a synthesizable RTL implementation.
