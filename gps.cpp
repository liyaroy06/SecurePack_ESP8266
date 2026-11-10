#include "gps.h"
#include "config.h"
#include <SoftwareSerial.h>
#include <TinyGPS++.h>

static TinyGPSPlus gps;
static SoftwareSerial gpsSerial(GPS_RX_PIN, GPS_TX_PIN);

static GpsData currentGpsData = {false, 0.0, 0.0, 0, 0.0f, 0.0f, "", "", 0};
static unsigned long lastGpsProcessMs = 0;

void gpsInit() {
  gpsSerial.begin(GPS_BAUD_RATE);
  Serial.print(F("[GPS] NEO-6M Serial initialized on RX="));
  Serial.print(GPS_RX_PIN);
  Serial.print(F(", TX="));
  Serial.println(GPS_TX_PIN);
}

void gpsUpdate() {
  // Feed incoming GPS NMEA sentences non-blockingly
  while (gpsSerial.available() > 0) {
    char c = gpsSerial.read();
    gps.encode(c);
  }

  unsigned long now = millis();
  if (now - lastGpsProcessMs < 500) {
    return;
  }
  lastGpsProcessMs = now;

  // Check valid fix (valid coordinate and updated within 5 seconds)
  bool hasValidLocation = gps.location.isValid() && (gps.location.age() < 6000);
  currentGpsData.fixAvailable = hasValidLocation;

  if (hasValidLocation) {
    currentGpsData.latitude = gps.location.lat();
    currentGpsData.longitude = gps.location.lng();
    currentGpsData.lastFixAgeMs = gps.location.age();
  } else {
    currentGpsData.latitude = 0.0;
    currentGpsData.longitude = 0.0;
    currentGpsData.lastFixAgeMs = 0;
  }

  currentGpsData.satellites = gps.satellites.isValid() ? gps.satellites.value() : 0;
  currentGpsData.speedKmph = gps.speed.isValid() ? (float)gps.speed.kmph() : 0.0f;
  currentGpsData.altitudeMeters = gps.altitude.isValid() ? (float)gps.altitude.meters() : 0.0f;

  if (gps.date.isValid()) {
    snprintf(currentGpsData.dateStr, sizeof(currentGpsData.dateStr), "%02d/%02d/%04d",
             gps.date.day(), gps.date.month(), gps.date.year());
  } else {
    strncpy(currentGpsData.dateStr, "--/--/----", sizeof(currentGpsData.dateStr));
  }

  if (gps.time.isValid()) {
    snprintf(currentGpsData.timeStr, sizeof(currentGpsData.timeStr), "%02d:%02d:%02d",
             gps.time.hour(), gps.time.minute(), gps.time.second());
  } else {
    strncpy(currentGpsData.timeStr, "--:--:--", sizeof(currentGpsData.timeStr));
  }
}

bool gpsHasFix() {
  return currentGpsData.fixAvailable;
}

GpsData gpsGetData() {
  return currentGpsData;
}

String gpsGetTimeString() {
  if (gps.time.isValid()) {
    char buf[16];
    snprintf(buf, sizeof(buf), "%02d:%02d:%02d",
             gps.time.hour(), gps.time.minute(), gps.time.second());
    return String(buf);
  }
  return String("");
}

String gpsGetJson() {
  String json = "{";
  json += "\"fix\":" + String(currentGpsData.fixAvailable ? "true" : "false") + ",";
  
  if (currentGpsData.fixAvailable) {
    json += "\"status\":\"FIX_AVAILABLE\",";
    json += "\"lat\":" + String(currentGpsData.latitude, 6) + ",";
    json += "\"lng\":" + String(currentGpsData.longitude, 6) + ",";
    json += "\"satellites\":" + String(currentGpsData.satellites) + ",";
    json += "\"speed\":" + String(currentGpsData.speedKmph, 1) + ",";
    json += "\"altitude\":" + String(currentGpsData.altitudeMeters, 1) + ",";
    json += "\"date\":\"" + String(currentGpsData.dateStr) + "\",";
    json += "\"time\":\"" + String(currentGpsData.timeStr) + "\",";
    json += "\"mapUrl\":\"https://maps.google.com/?q=" + 
            String(currentGpsData.latitude, 6) + "," + 
            String(currentGpsData.longitude, 6) + "\"";
  } else {
    json += "\"status\":\"NO_FIX\",";
    json += "\"message\":\"Waiting for satellite signal...\",";
    json += "\"lat\":0.0,";
    json += "\"lng\":0.0,";
    json += "\"satellites\":" + String(currentGpsData.satellites) + ",";
    json += "\"speed\":0.0,";
    json += "\"altitude\":0.0,";
    json += "\"date\":\"--\",";
    json += "\"time\":\"--\",";
    json += "\"mapUrl\":\"\"";
  }
  
  json += "}";
  return json;
}
