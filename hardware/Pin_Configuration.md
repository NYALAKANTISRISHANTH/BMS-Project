# Pin Configuration

| Function | ESP32 Pin |
|---|---:|
| MUX SIG/COM | GPIO35 |
| MUX S0 | GPIO26 |
| MUX S1 | GPIO25 |
| MUX S2 | GPIO33 |
| MUX S3 | GPIO32 |
| Current sensor/potentiometer | GPIO34 |
| Relay | GPIO15 |
| Green LED | GPIO17 |
| Yellow LED | GPIO16 |
| Red LED | GPIO4 |
| Buzzer | GPIO13 |
| LCD SDA | GPIO21 |
| LCD SCL | GPIO22 |

## CD74HC4067
- VCC -> 3.3 V
- GND -> GND
- EN -> GND
- SIG/COM -> GPIO35
- S0-S3 -> GPIO16-GPIO19

## Cell Potentiometers
P1 -> C0, P2 -> C1, ..., P16 -> C15.

Each potentiometer has its outer terminals connected to 3.3 V and GND; its wiper connects to the corresponding MUX channel.
