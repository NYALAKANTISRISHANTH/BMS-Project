# Wokwi Setup

1. Add an ESP32 board.
2. Add one CD74HC4067 16-channel analog multiplexer.
3. Add 16 potentiometers for the simulated cells.
4. Connect P1-P16 to C0-C15.
5. Connect MUX SIG to GPIO35 and S0,S1,S2,S3 to GPIO26,GPIO25,GPIO33,GPIO32.
6. Tie MUX EN low.
7. Keep the current potentiometer on GPIO34.
8. Keep relay, LEDs, buzzer and LCD on their documented pins.
9. Start with CELLS 4.
10. Test by changing CELLS to 8, 12 and 16 and verify that only the selected number of channels is processed.

For a real battery, do not connect battery taps directly to the ESP32 or this simulation MUX.
