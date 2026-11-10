#include <Arduino.h>
#include "config.h"
#include "events.h"
#include "alarm.h"
#include "battery.h"
#include "sensors.h"
#include "gps.h"
#include "security.h"
#include "wifi_server.h"

void setup() {
  Serial.begin(115200);
  delay(250);

  Serial.println();
  Serial.println(F("========================================================"));
  Serial.println(F("        SECUREPACK — SMART ANTI-THEFT BAG SYSTEM        "));
  Serial.println(F("           Firmware Version 1.0.0 (ESP8266MOD)          "));
  Serial.println(F("========================================================"));

  // 1. Initialize Event History Buffer & link GPS time provider
  eventsInit();
  eventsSetTimeProvider(gpsGetTimeString);

  // 2. Initialize Audible & Visual Indicators (Buzzer & LEDs)
  alarmInit();

  // 3. Initialize Battery Sensing / Simulation
  batteryInit();

  // 4. Initialize Sensors (MPU6050 & Magnetic Reed Switch)
  sensorsInit();

  // 5. Initialize NEO-6M GPS Module
  gpsInit();

  // 6. Initialize Security Logic & PIN Authentication
  securityInit();

  // 7. Initialize Wi-Fi Access Point & HTTP REST Server
  wifiServerInit();

  Serial.println(F("--------------------------------------------------------"));
  Serial.println(F("[SYSTEM] Initialization complete. System is ready."));
  Serial.print(F("[SYSTEM] Connect to Wi-Fi SSID : "));
  Serial.println(F(WIFI_AP_SSID));
  Serial.print(F("[SYSTEM] Web Dashboard / API   : http://"));
  Serial.println(wifiGetIpAddress());
  Serial.print(F("[SYSTEM] Default Security PIN  : "));
  Serial.println(F(DEFAULT_SECURITY_PIN));
  Serial.println(F("========================================================"));

  // Play startup confirmation chirp
  alarmPlayConfirmChirp();
}

void loop() {
  // Update all system modules non-blockingly
  alarmUpdate();
  sensorsUpdate();
  gpsUpdate();
  batteryUpdate();
  securityUpdate();
  wifiServerUpdate();

  // Yield to ESP8266 Wi-Fi and system background tasks
  yield();
}
