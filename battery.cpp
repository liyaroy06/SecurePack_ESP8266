#include "battery.h"
#include "config.h"

static float currentVoltage = 4.05f;
static int currentPercentage = 85;
static BatteryState currentState = BATTERY_STATE_HIGH;
static unsigned long lastBatteryReadMs = 0;
static const unsigned long BATTERY_READ_INTERVAL_MS = 2000;

#define ADC_FILTER_SAMPLES 8
static int sampleIndex = 0;
static int rawSamples[ADC_FILTER_SAMPLES] = {0};
static bool samplesFilled = false;

void batteryInit() {
  pinMode(BATTERY_ADC_PIN, INPUT);
  
  // Prime samples
  int initialRead = analogRead(BATTERY_ADC_PIN);
  for (int i = 0; i < ADC_FILTER_SAMPLES; i++) {
    rawSamples[i] = initialRead;
  }
  samplesFilled = true;
  batteryUpdate();
}

void batteryUpdate() {
  unsigned long now = millis();
  if (now - lastBatteryReadMs < BATTERY_READ_INTERVAL_MS && lastBatteryReadMs != 0) {
    return;
  }
  lastBatteryReadMs = now;

  if (BATTERY_SIMULATION_MODE) {
    // Clear simulation mode for demonstration when no external divider is attached
    currentVoltage = 4.02f;
    currentPercentage = 85;
    currentState = BATTERY_STATE_HIGH;
    return;
  }

  // Real ADC reading with moving average filter
  int raw = analogRead(BATTERY_ADC_PIN);
  rawSamples[sampleIndex] = raw;
  sampleIndex = (sampleIndex + 1) % ADC_FILTER_SAMPLES;

  long sum = 0;
  for (int i = 0; i < ADC_FILTER_SAMPLES; i++) {
    sum += rawSamples[i];
  }
  float avgRaw = (float)sum / ADC_FILTER_SAMPLES;

  // Convert raw ADC (0 - 1023) to battery voltage
  // NodeMCU onboard divider allows up to 3.3V input.
  // With typical external scaling resistor factor:
  currentVoltage = avgRaw * BATTERY_ADC_FACTOR;
  if (currentVoltage > 4.35f) currentVoltage = 4.35f;
  if (currentVoltage < 0.0f)  currentVoltage = 0.0f;

  // Calculate percentage: 3.30V (0%) to 4.20V (100%)
  if (currentVoltage >= BATTERY_MAX_VOLTAGE) {
    currentPercentage = 100;
  } else if (currentVoltage <= BATTERY_MIN_VOLTAGE) {
    currentPercentage = 0;
  } else {
    currentPercentage = (int)(((currentVoltage - BATTERY_MIN_VOLTAGE) / 
                               (BATTERY_MAX_VOLTAGE - BATTERY_MIN_VOLTAGE)) * 100.0f);
  }

  // Determine state
  if (currentPercentage < 15) {
    currentState = BATTERY_STATE_CRITICAL;
  } else if (currentPercentage < 40) {
    currentState = BATTERY_STATE_LOW;
  } else if (currentPercentage < 75) {
    currentState = BATTERY_STATE_MEDIUM;
  } else {
    currentState = BATTERY_STATE_HIGH;
  }
}

int batteryGetPercentage() {
  return currentPercentage;
}

float batteryGetVoltage() {
  return currentVoltage;
}

BatteryState batteryGetState() {
  return currentState;
}

const char* batteryGetStateString() {
  switch (currentState) {
    case BATTERY_STATE_CRITICAL: return "CRITICAL";
    case BATTERY_STATE_LOW:      return "LOW";
    case BATTERY_STATE_MEDIUM:   return "MEDIUM";
    case BATTERY_STATE_HIGH:
    default:                     return "HIGH";
  }
}

bool batteryIsSimulated() {
  return BATTERY_SIMULATION_MODE;
}

String batteryGetJson() {
  String json = "{";
  json += "\"percentage\":" + String(currentPercentage) + ",";
  json += "\"voltage\":" + String(currentVoltage, 2) + ",";
  json += "\"state\":\"" + String(batteryGetStateString()) + "\",";
  json += "\"simulated\":" + String(BATTERY_SIMULATION_MODE ? "true" : "false");
  json += "}";
  return json;
}
