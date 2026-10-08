# Assumptions Document

1. The project is developed and demonstrated in Wokwi.
2. Potentiometers represent simulated cell voltages.
3. The simulated cell-voltage input range is approximately 0–4.2 V.
4. The active cell count is selected at compile time with CELLS.
5. CELLS is constrained to 4–16.
6. A CD74HC4067 is used to multiplex the 16 simulated cell inputs into the ESP32 ADC.
7. The current potentiometer on GPIO25 represents discharge current.
8. The ADC conversion used by the simulation is (raw / 4095.0) * 4.2.
9. SoC is estimated using a linear 3.0–4.2 V mapping.
10. The MUX arrangement is for simulation and educational demonstration; it is not a safe direct interface to a real 16S battery.
11. A real battery implementation would require an appropriate battery-monitor/protection IC and high-voltage front end.
