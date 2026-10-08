# Pin Configuration

| Function | ESP32 Pin |
|---|---:|
| MUX SIG/COM | GPIO34 |
| MUX S0 | GPIO16 |
| MUX S1 | GPIO17 |
| MUX S2 | GPIO18 |
| MUX S3 | GPIO19 |
| Current sensor/potentiometer | GPIO25 |
| Relay | GPIO26 |
| Green LED | GPIO27 |
| Yellow LED | GPIO14 |
| Red LED | GPIO12 |
| Buzzer | GPIO13 |
| LCD SDA | GPIO21 |
| LCD SCL | GPIO22 |

## CD74HC4067
- VCC -> 3.3 V
- GND -> GND
- EN -> GND
- SIG/COM -> GPIO34
- S0-S3 -> GPIO16-GPIO19

## Cell Potentiometers
P1 -> C0, P2 -> C1, ..., P16 -> C15.

Each potentiometer has its outer terminals connected to 3.3 V and GND; its wiper connects to the corresponding MUX channel.
