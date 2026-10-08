# BMS Project

## Overview
This project implements a scalable Battery Management System (BMS) engine on ESP32 for a simulated 4–16 cell battery pack in Wokwi.

## Objectives
- Support 4 to 16 cells using one compile-time `CELLS` setting.
- Identify the strongest and weakest cells.
- Calculate cell-voltage imbalance in mV.
- Monitor imbalance trend.
- Estimate State of Charge (SoC).
- Apply an adaptive imbalance threshold using SoC and discharge current.
- Demonstrate system and fault state machines.
- Provide local LED, relay and buzzer indications.

## Hardware / Simulation
- ESP32
- CD74HC4067 16:1 analog multiplexer
- 16 potentiometers representing simulated cell voltages
- Current potentiometer on GPIO25
- Relay GPIO26
- Green LED GPIO27
- Yellow LED GPIO14
- Red LED GPIO12
- Buzzer GPIO13
- I2C LCD: SDA GPIO21, SCL GPIO22

## Scalability
Set:
```cpp
#define CELLS 4
```
Valid values are 4 through 16. The same code processes only the first `CELLS` MUX channels.

## Core Calculations
### Cell voltage
`Vcell = (ADC / 4095) × 4.2`

### Imbalance
`Imbalance(mV) = (Vstrongest − Vweakest) × 1000`

### SoC
Linear mapping from 3.0 V to 4.2 V:
- 3.0 V → 0%
- 4.2 V → 100%

### Adaptive threshold
- SoC > 80%: 30 mV
- SoC 20–80%: 50 mV
- SoC < 20%: 20 mV
- Discharge current > 1 A: add 15 mV

## State Machines
System state:
- NORMAL
- FAULT_DETECTED
- RECOVERY

Fault severity:
- FAULT_NORMAL
- FAULT_DEGRADED
- FAULT_FAILSAFE
- FAULT_SHUTDOWN

## Wokwi
Connect P1–P16 to MUX channels C0–C15. MUX SIG goes to GPIO34 and S0–S3 go to GPIO16–19. EN is tied to GND.

## Repository Structure
- `src/` — source code
- `docs/` — mandatory documentation
- `hardware/` — wiring and pin documentation

## Limitations
This is a Wokwi educational simulation. A real 16-cell battery requires an appropriate battery-monitor/protection IC and high-voltage measurement front end.
