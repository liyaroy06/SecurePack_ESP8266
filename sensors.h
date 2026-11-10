#ifndef SENSORS_H
#define SENSORS_H

#include <Arduino.h>

enum MotionSensitivity {
  SENSITIVITY_LOW,
  SENSITIVITY_MEDIUM,
  SENSITIVITY_HIGH
};

struct SensorReadings {
  float accelX;
  float accelY;
  float accelZ;
  float gyroX;
  float gyroY;
  float gyroZ;
  float tempC;
  float accelMagnitude;
  float deltaAccel;
  bool mpuConnected;
  bool zipperOpen;
  bool motionDetected;
};

void sensorsInit();
void sensorsUpdate();

// Sensitivity management
void sensorsSetSensitivity(MotionSensitivity level);
MotionSensitivity sensorsGetSensitivity();
const char* sensorsGetSensitivityString();

// Query states
bool sensorsIsMotionDetected();
bool sensorsIsZipperOpen();
bool sensorsZipperJustOpened();
bool sensorsMotionJustTriggered();
// Baseline & Calibration on Arm
void sensorsResetBaseline();
bool sensorsMagnetMoved();
void sensorsClearTriggerFlags();

// Raw data query
SensorReadings sensorsGetReadings();
String sensorsGetJson();

#endif // SENSORS_H
