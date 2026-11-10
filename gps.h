#ifndef GPS_MODULE_H
#define GPS_MODULE_H

#include <Arduino.h>

struct GpsData {
  bool fixAvailable;
  double latitude;
  double longitude;
  uint32_t satellites;
  float speedKmph;
  float altitudeMeters;
  char dateStr[16];     // e.g. "05/10/2026"
  char timeStr[16];     // e.g. "14:30:15"
  unsigned long lastFixAgeMs;
};

void gpsInit();
void gpsUpdate();

bool gpsHasFix();
GpsData gpsGetData();
String gpsGetJson();
String gpsGetTimeString();

#endif // GPS_MODULE_H
