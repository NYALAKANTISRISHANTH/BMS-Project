#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#define BLYNK_TEMPLATE_ID "TMPL35EGB5nJ2"
#define BLYNK_TEMPLATE_NAME "Modular BMS"
#define BLYNK_AUTH_TOKEN "x-ERfvlMPMHlRXuFyGql6jJD3jA-_HHR"


char wifiName[] = "Wokwi-GUEST";
char wifiPass[] = "";

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>

#define MIN_CELLS 4
#define MAX_CELLS 16
#define CELLS 8

#if CELLS < MIN_CELLS || CELLS > MAX_CELLS
#error "CELLS must be between 4 and 16"
#endif

const int MUX_SIG = 35;
const int MUX_S0 = 26;
const int MUX_S1 = 25;
const int MUX_S2 = 33;
const int MUX_S3 = 32;
const int currentPin = 34;
const int relayPin = 15;
const int greenLedPin = 17;
const int yellowLedPin = 16;
const int redLedPin = 4;
const int buzzerPin = 13;

const float MIN_CELL_VOLTAGE = 3.00;
const float MAX_CELL_VOLTAGE = 4.20;
const float RELAY_ON_MV = 60.0;
const float RELAY_OFF_MV = 40.0;
const float SEVERE_IMBALANCE_MV = 500.0;
const float MAX_CURRENT_AMPS = 5.0;

const unsigned long SENSOR_SAMPLE_MS = 100;
const unsigned long RELAY_DEBOUNCE_MS = 300;
const unsigned long RECOVERY_VERIFY_MS = 5000;
const unsigned long WIFI_RETRY_MS = 5000;
const unsigned long BLYNK_RETRY_MS = 5000;
const unsigned long LCD_UPDATE_MS = 250;
const float VOLTAGE_JUMP_THRESHOLD = 1.0;
const float CURRENT_JUMP_THRESHOLD = 3.0;
const int FROZEN_SAMPLE_LIMIT = 20;
const float FROZEN_EPSILON = 0.003;
const float RELAY_MISMATCH_TOLERANCE_MS = 1000.0;
const float EVENT_IMBALANCE_DELTA_MV = 10.0;
const float EVENT_SOC_DELTA = 2.0;
const float EVENT_CURRENT_DELTA_A = 0.25;


enum SystemState { SYSTEM_NORMAL, SYSTEM_FAULT, SYSTEM_RECOVERY };
enum FaultState { FAULT_NORMAL, FAULT_DEGRADED, FAULT_FAILSAFE, FAULT_SHUTDOWN };

struct TelemetryEvent {
  unsigned long timestamp;
  float imbalance;
  float soc;
  float current;
  bool relay;
  int systemState;
  int faultState;
  String type;
};

struct FaultLog {
  unsigned long timestamp;
  int oldSystemState;
  int newSystemState;
  int oldFaultState;
  int newFaultState;
  String reason;
};

enum FaultId {
  FAULT_ID_NONE,
  FAULT_ID_CELL_VOLTAGE,
  FAULT_ID_ADC_RANGE,
  FAULT_ID_SENSOR_FROZEN,
  FAULT_ID_VOLTAGE_JUMP,
  FAULT_ID_RELAY_MISMATCH,
  FAULT_ID_WIFI_LOST,
  FAULT_ID_SEVERE_IMBALANCE
};

struct SensorMonitor {
  float previousValue = -1.0f;
  float lastDistinctValue = -1.0f;
  int frozenCount = 0;
  bool initialized = false;
};


struct Analytics {
  float imbalanceHistory[100];
  int imbalanceIndex = 0;
  float socSum = 0;
  unsigned long socSamples = 0;
  float averageSoC = 0;
  float minimumSoC = 101.0;
  float maximumSoC = 0;
  unsigned long uptime = 0;
  unsigned long downtime = 0;
  unsigned long faultCount = 0;
  float riskScore = 0;
  String healthStatus = "EXCELLENT";
} analytics;

LiquidCrystal_I2C lcd(0x27, 16, 2);

float cellVoltage[CELLS];
float rawCellVoltage[CELLS];
int rawCellADC[CELLS];
int strongestCell = 0;
int weakestCell = 0;
float imbalance_mV = 0.0;
float previousImbalance_mV = 0.0;
float averageVoltage = 0.0;
float soc = 0.0;
float currentAmps = 0.0;
const char* imbalanceTrend = "STABLE";
float adaptiveThreshold = 50.0;

SystemState systemState = SYSTEM_NORMAL;
FaultState faultState = FAULT_NORMAL;

bool cellError = false;
bool adcError = false;
bool relayError = false;
bool severeCellError = false;
bool relayIsOn = false;

unsigned long recoveryStartTime = 0;
const unsigned long RECOVERY_TIME = 5000;

const int HISTORY_SIZE = 10;
float voltageHistory[CELLS][HISTORY_SIZE];
int historyIndex = 0;

int displayPage = 0;
unsigned long lcdTimer = 0;

const int QUEUE_SIZE = 50;
TelemetryEvent eventQueue[QUEUE_SIZE];
int queueHead = 0;
int queueTail = 0;
int queueCount = 0;
unsigned long telemetryTimer = 0;

bool wifiConnected = false;
int wifiRSSI = 0;
unsigned long wifiTimer = 0;

const int MAX_LOGS = 20;
FaultLog faultLogs[MAX_LOGS];
int faultLogCount = 0;

unsigned long lastMainPrint = 0;

unsigned long lastSensorSample = 0;
unsigned long lastRelayChange = 0;
unsigned long relayCommandSince = 0;
unsigned long lastWifiAttempt = 0;
unsigned long lastBlynkAttempt = 0;
unsigned long lastLCDUpdate = 0;
unsigned long lastMeaningfulEvent = 0;
unsigned long lastFaultEvent = 0;
bool blynkConnected = false;
bool relayCommand = false;
bool relayMismatch = false;
bool voltageJumpError = false;
bool frozenSensorError = false;
bool wifiWasConnected = false;
bool recoveryVerified = false;
FaultId activeFaultId = FAULT_ID_NONE;
SensorMonitor cellMonitors[CELLS];
SensorMonitor currentMonitor;
float previousEventImbalance = -1.0f;
float previousEventSoC = -1.0f;
float previousEventCurrent = -1.0f;
float lastLCDLines[2] = {-9999.0f, -9999.0f}; // retained for compatible fixed-size state
String lcdLine0 = "";
String lcdLine1 = "";
String lastRenderedLine0 = "";
String lastRenderedLine1 = "";


void selectMuxChannel(int channel);
void readSensorsAndStats();
bool detectErrors();
void updateStateMachines(bool errorDetected);
void updateOutputs();
void updateLCD();
void addTelemetryEvent(String eventType);
void processTelemetryQueue();
void updateWiFiAndAnalytics();
void serviceWiFiAndBlynk() {
  unsigned long now = millis();
  wifiConnected = (WiFi.status() == WL_CONNECTED);
  if (wifiConnected) {
    wifiRSSI = WiFi.RSSI();
    if (!wifiWasConnected) {
      Serial.print("[WIFI] Reconnected, IP: ");
      Serial.println(WiFi.localIP());
      addTelemetryEvent("WIFI_RECONNECTED");
    }
    wifiWasConnected = true;

    // Retry Blynk when Wi-Fi is available. Configure credentials once in setup().
    if (!Blynk.connected() && now - lastBlynkAttempt >= BLYNK_RETRY_MS) {
      lastBlynkAttempt = now;
      Serial.println("[BLYNK] Attempting cloud connection...");
      blynkConnected = Blynk.connect(5000); // bounded 1-second attempt
      if (blynkConnected) {
        Serial.println("[BLYNK] Connected successfully.");
        addTelemetryEvent("BLYNK_CONNECTED");
      } else {
        Serial.println("[BLYNK] Connection failed; will retry in 5 seconds.");
      }
    }

    // Keep the Blynk client serviced while connected.
    if (Blynk.connected()) {
      Blynk.run();
      blynkConnected = Blynk.connected();
      if (!blynkConnected) {
        Serial.println("[BLYNK] Disconnected; queued events retained.");
      }
    } else {
      blynkConnected = false;
    }
  } else {
    if (wifiWasConnected) {
      Serial.println("[WIFI] Lost; queueing telemetry offline.");
      addTelemetryEvent("WIFI_LOST");
    }
    wifiWasConnected = false;
    blynkConnected = false;
    wifiRSSI = 0;
    if (now - lastWifiAttempt >= WIFI_RETRY_MS) {
      lastWifiAttempt = now;
      WiFi.disconnect(false, false);
      WiFi.begin(wifiName, wifiPass, 6);
      Serial.println("[WIFI] Reconnection attempt started.");
    }
  }
}

void monitorSensorAnomalies() {
  frozenSensorError = false;
  voltageJumpError = false;
  activeFaultId = FAULT_ID_NONE;
  bool anotherCellChanged = false;

  // Static potentiometers are normal in Wokwi. Only suspect a frozen
  // channel when another cell changes substantially during the same window.
  for (int i = 0; i < CELLS; i++) {
    SensorMonitor &m = cellMonitors[i];
    if (m.initialized &&
        fabsf(rawCellVoltage[i] - m.previousValue) > 0.05f) {
      anotherCellChanged = true;
    }
  }

  for (int i = 0; i < CELLS; i++) {
    SensorMonitor &m = cellMonitors[i];
    float value = rawCellVoltage[i];
    if (m.initialized) {
      if (fabsf(value - m.previousValue) <= FROZEN_EPSILON) {
        if (m.frozenCount < FROZEN_SAMPLE_LIMIT) m.frozenCount++;
      } else {
        m.frozenCount = 0;
        m.lastDistinctValue = value;
      }

      if (fabsf(value - m.previousValue) > VOLTAGE_JUMP_THRESHOLD) {
        voltageJumpError = true;
        activeFaultId = FAULT_ID_VOLTAGE_JUMP;
      }

      if (anotherCellChanged && m.frozenCount >= FROZEN_SAMPLE_LIMIT) {
        frozenSensorError = true;
        activeFaultId = FAULT_ID_SENSOR_FROZEN;
      }
    } else {
      m.initialized = true;
      m.lastDistinctValue = value;
      m.frozenCount = 0;
    }
    m.previousValue = value;
  }

  if (activeFaultId == FAULT_ID_NONE) {
    for (int i = 0; i < CELLS; i++) {
      if (rawCellVoltage[i] < MIN_CELL_VOLTAGE ||
          rawCellVoltage[i] > MAX_CELL_VOLTAGE) {
        activeFaultId = FAULT_ID_CELL_VOLTAGE;
        break;
      }
    }
  }
}

void updateFaultSeverity() {
  if (severeCellError) activeFaultId = (imbalance_mV > SEVERE_IMBALANCE_MV)
      ? FAULT_ID_SEVERE_IMBALANCE : FAULT_ID_CELL_VOLTAGE;
  else if (relayMismatch) activeFaultId = FAULT_ID_RELAY_MISMATCH;
  else if (adcError && activeFaultId == FAULT_ID_NONE) activeFaultId = FAULT_ID_ADC_RANGE;
  else if (!cellError && !adcError && !relayError) activeFaultId = FAULT_ID_NONE;
}

void updateRelayNonBlocking() {
  unsigned long now = millis();
  bool requestedState = relayIsOn;

  // Adaptive threshold is used for ON; hysteresis band is retained for OFF.
  float onThreshold = max(RELAY_ON_MV, adaptiveThreshold);
  if (imbalance_mV > onThreshold) requestedState = true;
  else if (imbalance_mV < RELAY_OFF_MV) requestedState = false;

  // Do not energize relay for severe cell faults or a latched shutdown.
  if (faultState == FAULT_FAILSAFE || faultState == FAULT_SHUTDOWN) {
    requestedState = false;
  }

  if (requestedState != relayIsOn && now - lastRelayChange >= RELAY_DEBOUNCE_MS) {
    relayIsOn = requestedState;
    lastRelayChange = now;
    relayCommandSince = now;
    digitalWrite(relayPin, relayIsOn ? HIGH : LOW);
    addTelemetryEvent(relayIsOn ? "RELAY_ON" : "RELAY_OFF");
  }

  // Relay mismatch detection is only meaningful when an actual feedback input exists.
  // This Wokwi wiring has no relay feedback pin, so do not fabricate a mismatch.
  relayMismatch = false;
}

void renderLCDLine(byte row, const String &value) {
  String clipped = value;
  if (clipped.length() > 16) clipped = clipped.substring(0, 16);
  while (clipped.length() < 16) clipped += ' ';
  String &lastLine = (row == 0) ? lastRenderedLine0 : lastRenderedLine1;
  if (clipped != lastLine) {
    lcd.setCursor(0, row);
    lcd.print(clipped);
    lastLine = clipped;
  }
}

void serviceLCD() {
  unsigned long now = millis();
  if (now - lastLCDUpdate < LCD_UPDATE_MS) return;
  lastLCDUpdate = now;

  // Critical faults override the normal rotating pages immediately.
  if (faultState == FAULT_FAILSAFE || faultState == FAULT_SHUTDOWN) {
    renderLCDLine(0, faultState == FAULT_FAILSAFE ? "FAILSAFE FAULT" : "SYSTEM SHUTDOWN");
    String detail = severeCellError ? "SEVERE CELL/IMB" :
                    (cellError ? "CELL VOLTAGE ERR" :
                    (adcError ? "SENSOR/ADC ERR" : "CHECK SYSTEM"));
    renderLCDLine(1, detail);
    return;
  }

  static unsigned long pageTimer = 0;
  static int page = 0;
  if (now - pageTimer >= 3000) {
    pageTimer = now;
    page = (page + 1) % 4;
  }

  switch (page) {
    case 0:
      renderLCDLine(0, "BMS CELLS " + String(CELLS));
      renderLCDLine(1, "SOC " + String(soc, 1) + "%");
      break;
    case 1:
      renderLCDLine(0, "MAX C" + String(strongestCell + 1) + ":" +
                       String(cellVoltage[strongestCell], 2) + "V");
      renderLCDLine(1, "MIN C" + String(weakestCell + 1) + ":" +
                       String(cellVoltage[weakestCell], 2) + "V");
      break;
    case 2:
      renderLCDLine(0, "IMB " + String(imbalance_mV, 0) + "mV");
      renderLCDLine(1, String("TREND ") + imbalanceTrend);
      break;
    case 3:
      renderLCDLine(0, String("SYS ") +
          (systemState == SYSTEM_NORMAL ? "NORMAL" :
          (systemState == SYSTEM_FAULT ? "FAULT" : "RECOVERY")));
      renderLCDLine(1, String("RLY ") + (relayIsOn ? "ON" : "OFF") +
                       " Q:" + String(queueCount));
      break;
  }
}

void serviceMeaningfulTelemetry() {
  bool changed = false;
  if (previousEventImbalance < 0 ||
      fabsf(imbalance_mV - previousEventImbalance) >= EVENT_IMBALANCE_DELTA_MV) changed = true;
  if (previousEventSoC < 0 ||
      fabsf(soc - previousEventSoC) >= EVENT_SOC_DELTA) changed = true;
  if (previousEventCurrent < 0 ||
      fabsf(currentAmps - previousEventCurrent) >= EVENT_CURRENT_DELTA_A) changed = true;

  if (changed && millis() - lastMeaningfulEvent >= 1000) {
    addTelemetryEvent("PARAMETER_CHANGE");
    previousEventImbalance = imbalance_mV;
    previousEventSoC = soc;
    previousEventCurrent = currentAmps;
    lastMeaningfulEvent = millis();
  }
}

void recordFaultEvent(FaultId id, const String &reason) {
  lastFaultEvent = millis();
  Serial.printf("[FAULT LOG] t=%lu id=%d reason=%s\n",
                lastFaultEvent, (int)id, reason.c_str());
}

void logStateChange(int oldSys, int newSys, int oldFlt, int newFlt, String reason);
void printStatus();

void serviceWiFiAndBlynk();
void monitorSensorAnomalies();
void updateFaultSeverity();
void updateRelayNonBlocking();
void renderLCDLine(byte row, const String &value);
void serviceLCD();
void recordFaultEvent(FaultId id, const String &reason);
void serviceMeaningfulTelemetry();


void setup() {
  Serial.begin(115200);
  delay(100);

  pinMode(MUX_SIG, INPUT);
  pinMode(MUX_S0, OUTPUT);
  pinMode(MUX_S1, OUTPUT);
  pinMode(MUX_S2, OUTPUT);
  pinMode(MUX_S3, OUTPUT);

  pinMode(relayPin, OUTPUT);
  pinMode(greenLedPin, OUTPUT);
  pinMode(yellowLedPin, OUTPUT);
  pinMode(redLedPin, OUTPUT);
  pinMode(buzzerPin, OUTPUT);

  digitalWrite(relayPin, LOW);
  digitalWrite(greenLedPin, LOW);
  digitalWrite(yellowLedPin, LOW);
  digitalWrite(redLedPin, LOW);
  digitalWrite(buzzerPin, LOW);

  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("MODULAR BMS");
  lcd.setCursor(0, 1);
  lcd.print("Starting...");

  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  Blynk.config(BLYNK_AUTH_TOKEN);  // Configure Blynk once; connect after Wi-Fi is ready.
  WiFi.begin(wifiName, wifiPass, 6);
  lastWifiAttempt = millis();
  Serial.println("[WIFI] Initial connection started (non-blocking).");

  selectMuxChannel(0);
  for (int i = 0; i < CELLS; i++) {
    cellMonitors[i].initialized = false;
  }

  readSensorsAndStats();
  updateOutputs();
  Serial.println("\n========================================");
  Serial.printf(" MODULAR BMS - %d ACTIVE CELLS\n", CELLS);
  Serial.println(" Non-blocking monitoring started");
  Serial.println("========================================");
}


void loop() {
  const unsigned long now = millis();

  // Sensor reads are periodic and do not use long blocking waits.
  if (now - lastSensorSample >= SENSOR_SAMPLE_MS) {
    lastSensorSample = now;
    readSensorsAndStats();
    monitorSensorAnomalies();
    bool errorDetected = detectErrors();
    updateStateMachines(errorDetected);
    updateFaultSeverity();
    updateRelayNonBlocking();
    updateOutputs();
    serviceMeaningfulTelemetry();
  }

  serviceWiFiAndBlynk();
  updateWiFiAndAnalytics();
  serviceLCD();

  if (now - telemetryTimer >= 3000) {
    telemetryTimer = now;
    addTelemetryEvent("PERIODIC");
  }
  processTelemetryQueue();

  if (now - lastMainPrint >= 1000) {
    lastMainPrint = now;
    printStatus();
  }
}


void selectMuxChannel(int channel) {
  digitalWrite(MUX_S0, channel & 0x01);
  digitalWrite(MUX_S1, (channel >> 1) & 0x01);
  digitalWrite(MUX_S2, (channel >> 2) & 0x01);
  digitalWrite(MUX_S3, (channel >> 3) & 0x01);
}

void readSensorsAndStats() {
  float totalVolt = 0.0f;
  strongestCell = 0;
  weakestCell = 0;

  for (int i = 0; i < CELLS; i++) {
    selectMuxChannel(i);
    delayMicroseconds(50);
    // Discard one conversion after mux switching to improve settling.
    (void)analogRead(MUX_SIG);
    rawCellADC[i] = analogRead(MUX_SIG);
    rawCellVoltage[i] = (rawCellADC[i] / 4095.0f) * 4.2f;
    cellVoltage[i] = rawCellVoltage[i];

    if (i > 0) {
      if (cellVoltage[i] > cellVoltage[strongestCell]) strongestCell = i;
      if (cellVoltage[i] < cellVoltage[weakestCell]) weakestCell = i;
    }
    totalVolt += cellVoltage[i];
    voltageHistory[i][historyIndex] = cellVoltage[i];
  }

  historyIndex = (historyIndex + 1) % HISTORY_SIZE;

  int rawCurrent = analogRead(currentPin);
  currentAmps = constrain((rawCurrent / 4095.0f) * MAX_CURRENT_AMPS,
                          0.0f, MAX_CURRENT_AMPS);

  averageVoltage = totalVolt / CELLS;
  imbalance_mV = max(0.0f,
      (cellVoltage[strongestCell] - cellVoltage[weakestCell]) * 1000.0f);

  if (imbalance_mV > previousImbalance_mV + 0.5f) {
    imbalanceTrend = "INCREASING";
  } else if (imbalance_mV < previousImbalance_mV - 0.5f) {
    imbalanceTrend = "DECREASING";
  } else {
    imbalanceTrend = "STABLE";
  }
  previousImbalance_mV = imbalance_mV;

  soc = constrain(((averageVoltage - MIN_CELL_VOLTAGE) /
      (MAX_CELL_VOLTAGE - MIN_CELL_VOLTAGE)) * 100.0f, 0.0f, 100.0f);

  // Adaptive imbalance limit per Task 1: SoC bands plus discharge-rate margin.
  if (soc > 80.0f) adaptiveThreshold = 30.0f;
  else if (soc < 20.0f) adaptiveThreshold = 20.0f;
  else adaptiveThreshold = 50.0f;
  if (currentAmps > 1.0f) adaptiveThreshold += 15.0f;
}


bool detectErrors() {
  cellError = false;
  adcError = false;
  severeCellError = false;

  for (int i = 0; i < CELLS; i++) {
    if (rawCellADC[i] < 0 || rawCellADC[i] > 4095) adcError = true;
    if (rawCellVoltage[i] < MIN_CELL_VOLTAGE ||
        rawCellVoltage[i] > MAX_CELL_VOLTAGE) cellError = true;
    if (rawCellVoltage[i] < 2.50f) severeCellError = true;
  }

  if (imbalance_mV > SEVERE_IMBALANCE_MV) severeCellError = true;
  if (frozenSensorError || voltageJumpError) adcError = true;
  if (relayMismatch) relayError = true;
  return cellError || adcError || severeCellError || relayError;
}


void updateStateMachines(bool errorDetected) {
  SystemState oldSystem = systemState;
  FaultState oldFault = faultState;

  // Task 4: explicit deterministic fault-state transitions.
  switch (faultState) {
    case FAULT_NORMAL:
      if (severeCellError) faultState = FAULT_FAILSAFE;
      else if (errorDetected) faultState = FAULT_DEGRADED;
      break;

    case FAULT_DEGRADED:
      if (severeCellError) faultState = FAULT_FAILSAFE;
      else if (!errorDetected) {
        faultState = FAULT_NORMAL;
      }
      break;

    case FAULT_FAILSAFE:
      // Never jump straight to NORMAL; verify all inputs stable first.
      if (!errorDetected && !severeCellError) {
        if (!recoveryStartTime) recoveryStartTime = millis();
        if (millis() - recoveryStartTime >= RECOVERY_VERIFY_MS) {
          faultState = FAULT_DEGRADED;
          recoveryStartTime = 0;
        }
      } else {
        recoveryStartTime = 0;
      }
      break;

    case FAULT_SHUTDOWN:
      // Latched shutdown requires an explicit operator reset/restart.
      break;
  }

  switch (systemState) {
    case SYSTEM_NORMAL:
      if (errorDetected) {
        systemState = SYSTEM_FAULT;
        recoveryStartTime = 0;
      }
      break;
    case SYSTEM_FAULT:
      if (!errorDetected) {
        systemState = SYSTEM_RECOVERY;
        recoveryStartTime = millis();
      }
      break;
    case SYSTEM_RECOVERY:
      if (errorDetected) {
        systemState = SYSTEM_FAULT;
        recoveryStartTime = 0;
      } else if (millis() - recoveryStartTime >= RECOVERY_TIME) {
        systemState = SYSTEM_NORMAL;
      }
      break;
  }

  if (oldSystem != systemState || oldFault != faultState) {
    analytics.faultCount += (oldFault != faultState) ? 1UL : 0UL;
    String reason = "state transition";
    if (severeCellError) reason = "severe cell/imbalance fault";
    else if (frozenSensorError) reason = "frozen sensor reading";
    else if (voltageJumpError) reason = "unrealistic voltage jump";
    else if (relayMismatch) reason = "relay command/status mismatch";
    else if (cellError) reason = "cell voltage out of range";
    logStateChange((int)oldSystem, (int)systemState,
                   (int)oldFault, (int)faultState, reason);
    recordFaultEvent(activeFaultId, reason);
  }
}


void updateOutputs() {
  digitalWrite(relayPin, relayIsOn ? HIGH : LOW);

  digitalWrite(greenLedPin, faultState == FAULT_NORMAL ? HIGH : LOW);
  digitalWrite(yellowLedPin, faultState == FAULT_DEGRADED ? HIGH : LOW);
  digitalWrite(redLedPin, faultState >= FAULT_FAILSAFE ? HIGH : LOW);

  if (faultState == FAULT_NORMAL) {
    digitalWrite(buzzerPin, LOW);
  } else if (faultState == FAULT_DEGRADED) {
    digitalWrite(buzzerPin, ((millis() / 1000UL) % 2UL == 0UL) ? HIGH : LOW);
  } else {
    digitalWrite(buzzerPin, ((millis() / 250UL) % 2UL == 0UL) ? HIGH : LOW);
  }
}


void updateLCD() {
  serviceLCD();
}


void addTelemetryEvent(String eventType) {
  if (queueCount >= QUEUE_SIZE) {
    Serial.println("[QUEUE] FULL - EVENT DROPPED");
    return;
  }

  eventQueue[queueTail] = {millis(), imbalance_mV, soc, currentAmps, relayIsOn, (int)systemState, (int)faultState, eventType};
  queueTail = (queueTail + 1) % QUEUE_SIZE;
  queueCount++;
}

void processTelemetryQueue() {
  if (queueCount <= 0 || !wifiConnected || !blynkConnected) return;

  TelemetryEvent ev = eventQueue[queueHead];
  Blynk.virtualWrite(V0, ev.imbalance);
  Blynk.virtualWrite(V1, cellVoltage[strongestCell]);
  Blynk.virtualWrite(V2, cellVoltage[weakestCell]);
  Blynk.virtualWrite(V3, ev.relay ? 1 : 0);
  Blynk.virtualWrite(V4, ev.faultState);
  Blynk.virtualWrite(V5, wifiRSSI);
  Blynk.virtualWrite(V6, queueCount - 1);
  Blynk.virtualWrite(V7, ev.systemState);
  Blynk.virtualWrite(V8, ev.soc);
  Blynk.virtualWrite(V9, analytics.riskScore);

  queueHead = (queueHead + 1) % QUEUE_SIZE;
  queueCount--;
}


void updateWiFiAndAnalytics() {
  // Keep analytics updated from live measurements.
  analytics.imbalanceHistory[analytics.imbalanceIndex] = imbalance_mV;
  analytics.imbalanceIndex = (analytics.imbalanceIndex + 1) % 100;

  analytics.socSum += soc;
  analytics.socSamples++;
  analytics.averageSoC = analytics.socSum / analytics.socSamples;
  analytics.minimumSoC = min(analytics.minimumSoC, soc);
  analytics.maximumSoC = max(analytics.maximumSoC, soc);

  if (faultState == FAULT_NORMAL) analytics.uptime++;
  else analytics.downtime++;

  float imbalanceRisk = constrain((imbalance_mV / 1000.0f) * 100.0f, 0.0f, 100.0f);
  float trendRisk = (String(imbalanceTrend) == "INCREASING") ? 20.0f : 0.0f;
  float faultRisk = constrain(analytics.faultCount * 10.0f, 0.0f, 100.0f);
  float socRisk = (soc < 20.0f || soc > 90.0f) ? 30.0f : 0.0f;
  float stateRisk = faultState == FAULT_DEGRADED ? 20.0f :
                    (faultState == FAULT_FAILSAFE ? 60.0f :
                    (faultState == FAULT_SHUTDOWN ? 100.0f : 0.0f));
  analytics.riskScore = constrain(
      (imbalanceRisk + trendRisk + faultRisk + socRisk + stateRisk) / 5.0f,
      0.0f, 100.0f);

  if (analytics.riskScore < 20.0f) analytics.healthStatus = "EXCELLENT";
  else if (analytics.riskScore < 40.0f) analytics.healthStatus = "GOOD";
  else if (analytics.riskScore < 60.0f) analytics.healthStatus = "FAIR";
  else if (analytics.riskScore < 80.0f) analytics.healthStatus = "POOR";
  else analytics.healthStatus = "CRITICAL";
}


void logStateChange(int oldSys, int newSys, int oldFlt, int newFlt, String reason) {
  if (faultLogCount >= MAX_LOGS) {
    for (int i = 0; i < MAX_LOGS - 1; i++) {
      faultLogs[i] = faultLogs[i + 1];
    }
    faultLogCount = MAX_LOGS - 1;
  }

  faultLogs[faultLogCount++] = {millis(), oldSys, newSys, oldFlt, newFlt, reason};
  Serial.printf("[STATE] %s\n", reason.c_str());
}

void printStatus() {
  Serial.println("\n----------------------------------------");
  Serial.printf("Active Cells: %d / %d\n", CELLS, MAX_CELLS);
  Serial.print("Cells: ");
  for (int i = 0; i < CELLS; i++) {
    Serial.printf("C%d=%.2fV", i + 1, cellVoltage[i]);
    if (i < CELLS - 1) Serial.print(" | ");
  }
  Serial.println();
  Serial.printf("Strongest: C%d = %.3f V\n", strongestCell + 1, cellVoltage[strongestCell]);
  Serial.printf("Weakest: C%d = %.3f V\n", weakestCell + 1, cellVoltage[weakestCell]);
  Serial.printf("Imbalance: %.1f mV | Trend: %s\n", imbalance_mV, imbalanceTrend);
  Serial.printf("SoC: %.1f%% | Current: %.2f A\n", soc, currentAmps);
  Serial.printf("Adaptive Threshold: %.1f mV\n", adaptiveThreshold);
  Serial.printf("System State: %d | Fault State: %d\n", (int)systemState, (int)faultState);
  Serial.printf("Errors -> Cell: %s | ADC: %s | Severe: %s\n", cellError ? "YES" : "NO", adcError ? "YES" : "NO", severeCellError ? "YES" : "NO");
  Serial.printf("Relay: %s | Queue Depth: %d\n", relayIsOn ? "ON" : "OFF", queueCount);
  Serial.printf("WiFi: %s | Blynk: %s | RSSI: %s\n",
                wifiConnected ? "CONNECTED" : "OFFLINE",
                blynkConnected ? "CONNECTED" : "OFFLINE",
                wifiConnected ? (String(wifiRSSI) + " dBm").c_str() : "N/A");
  Serial.printf("Risk: %.1f | Health: %s\n", analytics.riskScore, analytics.healthStatus.c_str());
  Serial.printf("Avg SoC: %.1f%% | Min: %.1f%% | Max: %.1f%%\n", analytics.averageSoC, analytics.minimumSoC, analytics.maximumSoC);
  Serial.printf("Fault Count: %lu\n", analytics.faultCount);
  Serial.println("----------------------------------------");
}
