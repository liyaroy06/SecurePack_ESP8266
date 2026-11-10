#ifndef BATTERY_H
#define BATTERY_H

#include <Arduino.h>

enum BatteryState {
  BATTERY_STATE_CRITICAL, // < 15%
  BATTERY_STATE_LOW,      // 15% - 40%
  BATTERY_STATE_MEDIUM,   // 40% - 75%
  BATTERY_STATE_HIGH      // > 75%
};

void batteryInit();
void batteryUpdate();
int batteryGetPercentage();
float batteryGetVoltage();
const char* batteryGetStateString();
BatteryState batteryGetState();
bool batteryIsSimulated();
String batteryGetJson();

#endif // BATTERY_H
