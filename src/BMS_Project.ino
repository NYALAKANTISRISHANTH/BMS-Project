// Scalable ESP32 BMS core - 4 to 16 cells
// Simulation architecture: CD74HC4067 + potentiometers.
// Set CELLS to any value from 4 to 16.

#include <Arduino.h>

#define MIN_CELLS 4
#define MAX_CELLS 16
#define CELLS 4

#if CELLS < MIN_CELLS || CELLS > MAX_CELLS
#error "CELLS must be between 4 and 16"
#endif

#define MUX_SIG 34
#define MUX_S0 16
#define MUX_S1 17
#define MUX_S2 18
#define MUX_S3 19
#define CURRENT_PIN 25
#define RELAY_PIN 26
#define GREEN_LED 27
#define YELLOW_LED 14
#define RED_LED 12
#define BUZZER 13

float cellVoltage[CELLS];
int rawCellADC[CELLS];
int strongestCell = 0;
int weakestCell = 0;
float imbalance_mV = 0.0;
float previousImbalance_mV = 0.0;
float soc = 0.0;
float dischargeCurrent = 0.0;
float adaptiveThreshold_mV = 50.0;

enum SystemState { NORMAL, FAULT_DETECTED, RECOVERY };
enum FaultState { FAULT_NORMAL, FAULT_DEGRADED, FAULT_FAILSAFE, FAULT_SHUTDOWN };
SystemState systemState = NORMAL;
FaultState faultState = FAULT_NORMAL;

void selectMuxChannel(uint8_t ch) {
  digitalWrite(MUX_S0, ch & 0x01);
  digitalWrite(MUX_S1, (ch >> 1) & 0x01);
  digitalWrite(MUX_S2, (ch >> 2) & 0x01);
  digitalWrite(MUX_S3, (ch >> 3) & 0x01);
  delayMicroseconds(5);
}

float readCell(uint8_t index) {
  selectMuxChannel(index);
  int raw = analogRead(MUX_SIG);
  rawCellADC[index] = raw;
  return (raw / 4095.0) * 4.2;
}

void calculateStatistics() {
  strongestCell = 0;
  weakestCell = 0;

  for (int i = 1; i < CELLS; i++) {
    if (cellVoltage[i] > cellVoltage[strongestCell]) strongestCell = i;
    if (cellVoltage[i] < cellVoltage[weakestCell]) weakestCell = i;
  }

  imbalance_mV = (cellVoltage[strongestCell] - cellVoltage[weakestCell]) * 1000.0;

  float average = 0.0;
  for (int i = 0; i < CELLS; i++) average += cellVoltage[i];
  average /= CELLS;

  soc = ((average - 3.0) / 1.2) * 100.0;
  soc = constrain(soc, 0.0, 100.0);

  if (soc > 80.0) adaptiveThreshold_mV = 30.0;
  else if (soc < 20.0) adaptiveThreshold_mV = 20.0;
  else adaptiveThreshold_mV = 50.0;

  if (dischargeCurrent > 1.0) adaptiveThreshold_mV += 15.0;
}

bool detectErrors() {
  bool error = false;
  bool severe = false;

  for (int i = 0; i < CELLS; i++) {
    if (cellVoltage[i] < 3.0 || cellVoltage[i] > 4.2) error = true;
    if (cellVoltage[i] < 2.5) severe = true;
  }

  if (imbalance_mV > 500.0) severe = true;

  if (severe) faultState = FAULT_FAILSAFE;
  else if (error) faultState = FAULT_DEGRADED;
  else faultState = FAULT_NORMAL;

  return error;
}

void updateStateMachine(bool error) {
  switch (systemState) {
    case NORMAL:
      if (error) systemState = FAULT_DETECTED;
      break;
    case FAULT_DETECTED:
      if (!error) systemState = RECOVERY;
      break;
    case RECOVERY:
      if (error) systemState = FAULT_DETECTED;
      else systemState = NORMAL;
      break;
  }
}

void updateOutputs() {
  bool fault = (systemState != NORMAL) || (faultState != FAULT_NORMAL);

  digitalWrite(GREEN_LED, !fault);
  digitalWrite(YELLOW_LED, fault && faultState == FAULT_DEGRADED);
  digitalWrite(RED_LED, faultState >= FAULT_FAILSAFE);
  digitalWrite(BUZZER, faultState >= FAULT_FAILSAFE);

  // Relay hysteresis: disconnect on high imbalance, reconnect below lower threshold.
  if (imbalance_mV > 60.0) digitalWrite(RELAY_PIN, LOW);
  else if (imbalance_mV < 40.0) digitalWrite(RELAY_PIN, HIGH);
}

void printStatus() {
  Serial.println("---- BMS STATUS ----");
  for (int i = 0; i < CELLS; i++) {
    Serial.print("Cell ");
    Serial.print(i + 1);
    Serial.print(": ");
    Serial.print(cellVoltage[i], 3);
    Serial.println(" V");
  }
  Serial.print("Strongest: Cell "); Serial.println(strongestCell + 1);
  Serial.print("Weakest: Cell "); Serial.println(weakestCell + 1);
  Serial.print("Imbalance: "); Serial.print(imbalance_mV, 1); Serial.println(" mV");
  Serial.print("SoC: "); Serial.print(soc, 1); Serial.println("%");
  Serial.print("Discharge current: "); Serial.print(dischargeCurrent, 2); Serial.println(" A");
  Serial.print("Adaptive threshold: "); Serial.print(adaptiveThreshold_mV, 1); Serial.println(" mV");
}

void setup() {
  Serial.begin(115200);
  pinMode(MUX_S0, OUTPUT); pinMode(MUX_S1, OUTPUT);
  pinMode(MUX_S2, OUTPUT); pinMode(MUX_S3, OUTPUT);
  pinMode(CURRENT_PIN, INPUT);
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(GREEN_LED, OUTPUT); pinMode(YELLOW_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT); pinMode(BUZZER, OUTPUT);

  digitalWrite(RELAY_PIN, HIGH);
  digitalWrite(GREEN_LED, HIGH);
  digitalWrite(YELLOW_LED, LOW);
  digitalWrite(RED_LED, LOW);
  digitalWrite(BUZZER, LOW);
}

void loop() {
  for (int i = 0; i < CELLS; i++) cellVoltage[i] = readCell(i);

  int rawCurrent = analogRead(CURRENT_PIN);
  dischargeCurrent = (rawCurrent / 4095.0) * 5.0;

  calculateStatistics();
  bool error = detectErrors();
  updateStateMachine(error);
  updateOutputs();
  printStatus();

  previousImbalance_mV = imbalance_mV;
  delay(1000);
}
