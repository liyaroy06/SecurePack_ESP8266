#include "sensors.h"
#include "config.h"
#include "security.h"
#include <Wire.h>

static bool mpuInitialized = false;
static uint8_t mpuI2cAddress = 0x68;

static MotionSensitivity currentSensitivity = SENSITIVITY_MEDIUM;
static SensorReadings lastReadings = {0};

static float prevMagnitude = 9.8f;
static float lastAccelX = 0.0f;
static float lastAccelY = 0.0f;
static float lastAccelZ = 9.8f;
static float baselineAccelX = 0.0f;
static float baselineAccelY = 0.0f;
static float baselineAccelZ = 9.8f;

static int motionDebounceCount = 0;
static unsigned long motionHoldUntilMs = 0;
static bool motionFlag = false;
static bool motionJustTriggeredFlag = false;

static int debouncedReedState = -1;
static int lastRawReedReading = -1;
static unsigned long lastReedDebounceTime = 0;
static int armedReedBaseline = -1;
static bool magnetMovedFlag = false;
static bool zipperJustOpenedFlag = false;
static bool zipperJustClosedFlag = false;
static unsigned long armingGracePeriodUntilMs = 0;

static unsigned long lastSensorUpdateMs = 0;
static const unsigned long SENSOR_UPDATE_INTERVAL_MS = 40; 

static bool scanAndInitMpu() {
    int pinPairs[][2] = {
        {D2, D1},
        {D1, D2}
    };
    const char* pinNames[] = {"SDA=D2, SCL=D1", "SDA=D1, SCL=D2"};

    for (int p = 0; p < 2; p++) {
        int sda = pinPairs[p][0];
        int scl = pinPairs[p][1];
        Serial.print(F("[I2C] Scanning "));
        Serial.print(pinNames[p]);
        Serial.println(F("..."));

        Wire.begin(sda, scl);
        Wire.setClock(100000);
        delay(40);

        for (uint8_t addr = 1; addr < 127; addr++) {
            Wire.beginTransmission(addr);
            byte error = Wire.endTransmission();
            if (error == 0) {
                Serial.print(F("[I2C] Device found at 0x"));
                if (addr < 16) Serial.print('0');
                Serial.print(addr, HEX);
                Serial.print(F(" using "));
                Serial.println(pinNames[p]);

                if (addr == 0x68 || addr == 0x69) {
                    mpuI2cAddress = addr;
                    Wire.beginTransmission(addr);
                    Wire.write(0x6B); // Power Management 1 register
                    Wire.write(0x00); // Wake up sensor
                    byte wakeErr = Wire.endTransmission();
                    if (wakeErr == 0) {
                        Serial.println(F("[MPU6050] Power wake-up byte (0x6B=0x00) sent successfully."));
                        return true;
                    } else {
                        Serial.print(F("[MPU6050] Wake write failed, err="));
                        Serial.println(wakeErr);
                    }
                }
            }
        }
    }
    return false;
}

static bool readMpuSensors(float &ax, float &ay, float &az, float &gx, float &gy, float &gz) {
    if (!mpuInitialized) return false;

    Wire.beginTransmission(mpuI2cAddress);
    Wire.write(0x3B); // Starting register: ACCEL_XOUT_H
    if (Wire.endTransmission(false) != 0) return false;

    if (Wire.requestFrom((uint8_t)mpuI2cAddress, (size_t)14, true) != 14) return false;

    int16_t rawAx = (Wire.read() << 8) | Wire.read();
    int16_t rawAy = (Wire.read() << 8) | Wire.read();
    int16_t rawAz = (Wire.read() << 8) | Wire.read();
    int16_t rawTemp = (Wire.read() << 8) | Wire.read();
    int16_t rawGx = (Wire.read() << 8) | Wire.read();
    int16_t rawGy = (Wire.read() << 8) | Wire.read();
    int16_t rawGz = (Wire.read() << 8) | Wire.read();

    ax = (rawAx / 16384.0f) * 9.80665f;
    ay = (rawAy / 16384.0f) * 9.80665f;
    az = (rawAz / 16384.0f) * 9.80665f;

    lastReadings.tempC = (rawTemp / 340.0f) + 36.53f;

    gx = (rawGx / 131.0f) * (PI / 180.0f);
    gy = (rawGy / 131.0f) * (PI / 180.0f);
    gz = (rawGz / 131.0f) * (PI / 180.0f);

    return true;
}

void sensorsInit() {
    pinMode(REED_PIN, INPUT_PULLUP);
    debouncedReedState = digitalRead(REED_PIN);
    lastRawReedReading = debouncedReedState;
    armedReedBaseline = debouncedReedState;

    if (scanAndInitMpu()) {
        mpuInitialized = true;
        Serial.print(F("[MPU6050] Init status: SUCCESS (Awakened at address 0x"));
        Serial.print(mpuI2cAddress, HEX);
        Serial.println(F(")"));
    } else {
        mpuInitialized = false;
        Serial.println(F("[MPU6050] Init status: FAILED (Sensor did not respond at 0x68/0x69)"));
    }

    if (mpuInitialized) {
        float ax, ay, az, gx, gy, gz;
        if (readMpuSensors(ax, ay, az, gx, gy, gz)) {
            lastAccelX = baselineAccelX = ax;
            lastAccelY = baselineAccelY = ay;
            lastAccelZ = baselineAccelZ = az;
            prevMagnitude = sqrt(sq(ax) + sq(ay) + sq(az));
            Serial.print(F("[MPU6050] Initial accelerometer: X="));
            Serial.print(ax, 2);
            Serial.print(F(" Y="));
            Serial.print(ay, 2);
            Serial.print(F(" Z="));
            Serial.println(az, 2);
        }
    }

    lastReadings.mpuConnected = mpuInitialized;
}

void sensorsResetBaseline() {
    debouncedReedState = digitalRead(REED_PIN);
    lastRawReedReading = debouncedReedState;
    armedReedBaseline = debouncedReedState;
    
    magnetMovedFlag = false;
    zipperJustOpenedFlag = false;
    motionFlag = false;
    motionJustTriggeredFlag = false;
    motionDebounceCount = 0;
    motionHoldUntilMs = 0;

    if (mpuInitialized) {
        float ax, ay, az, gx, gy, gz;
        if (readMpuSensors(ax, ay, az, gx, gy, gz)) {
            baselineAccelX = lastAccelX = ax;
            baselineAccelY = lastAccelY = ay;
            baselineAccelZ = lastAccelZ = az;
            prevMagnitude = sqrt(sq(ax) + sq(ay) + sq(az));
        }
    }

    lastReadings.zipperOpen = (debouncedReedState == HIGH);
    lastReadings.motionDetected = false;
    armingGracePeriodUntilMs = millis() + 1000;
}

bool sensorsMagnetMoved() { return magnetMovedFlag; }
void sensorsSetSensitivity(MotionSensitivity level) { currentSensitivity = level; }
MotionSensitivity sensorsGetSensitivity() { return currentSensitivity; }

const char* sensorsGetSensitivityString() {
    switch (currentSensitivity) {
        case SENSITIVITY_LOW:    return "LOW";
        case SENSITIVITY_HIGH:   return "HIGH";
        case SENSITIVITY_MEDIUM:
        default:                 return "MEDIUM";
    }
}

void sensorsUpdate() {
    unsigned long now = millis();
    if (now - lastSensorUpdateMs < SENSOR_UPDATE_INTERVAL_MS) return;
    lastSensorUpdateMs = now;

    int rawReading = digitalRead(REED_PIN);
    if (rawReading != lastRawReedReading) {
        lastReedDebounceTime = now;
        lastRawReedReading = rawReading;
    }

    if ((now - lastReedDebounceTime) > REED_DEBOUNCE_MS) {
        if (rawReading != debouncedReedState) {
            int prevState = debouncedReedState;
            debouncedReedState = rawReading;
            
            if (prevState == LOW && debouncedReedState == HIGH) {
                zipperJustOpenedFlag = true;
                magnetMovedFlag = true;
            } else if (prevState == HIGH && debouncedReedState == LOW) {
                zipperJustClosedFlag = true;
                magnetMovedFlag = false;
            }
        }
    }

    lastReadings.zipperOpen = (debouncedReedState == HIGH);

    if (!mpuInitialized) {
        static unsigned long lastMpuRetryMs = 0;
        if (now - lastMpuRetryMs >= 3000) {
            lastMpuRetryMs = now;
            if (scanAndInitMpu()) {
                mpuInitialized = true;
                lastReadings.mpuConnected = true;
                Serial.println(F("[MPU6050] Reconnected and awakened successfully on retry!"));
            }
        }
    }

    if (mpuInitialized) {
        float ax, ay, az, gx, gy, gz;
        if (readMpuSensors(ax, ay, az, gx, gy, gz)) {
            lastReadings.accelX = ax;
            lastReadings.accelY = ay;
            lastReadings.accelZ = az;
            lastReadings.gyroX = gx;
            lastReadings.gyroY = gy;
            lastReadings.gyroZ = gz;

            float currentMagnitude = sqrt(sq(ax) + sq(ay) + sq(az));
            
            // Dynamic acceleration change between consecutive updates
            float dynamicDelta = sqrt(sq(ax - lastAccelX) +
                                      sq(ay - lastAccelY) +
                                      sq(az - lastAccelZ));

            // Baseline displacement from resting position
            float baselineDelta = sqrt(sq(ax - baselineAccelX) +
                                       sq(ay - baselineAccelY) +
                                       sq(az - baselineAccelZ));

            // Deviation from 1G gravity
            float gravityDelta = fabs(currentMagnitude - 9.807f);

            // Gyroscope rotation rate in rad/s
            float gyroRate = sqrt(sq(gx) + sq(gy) + sq(gz));

            lastAccelX = ax;
            lastAccelY = ay;
            lastAccelZ = az;
            prevMagnitude = currentMagnitude;

            lastReadings.accelMagnitude = currentMagnitude;
            lastReadings.deltaAccel = dynamicDelta;

            float accelThreshold = 1.20f;
            float gyroThreshold = 0.30f; // ~17 deg/s

            if (currentSensitivity == SENSITIVITY_LOW) {
                accelThreshold = 2.50f;
                gyroThreshold = 0.80f;                // ~45 deg/s
            } else if (currentSensitivity == SENSITIVITY_HIGH) {
                accelThreshold = 0.70f;
                gyroThreshold = 0.18f;                // ~10 deg/s
            }

            float movementVal = dynamicDelta;
            if (baselineDelta > movementVal) movementVal = baselineDelta;

            // Disturbance trigger: dynamic 3D movement, baseline orientation change, or rotation
            bool sampleDisturbed = (dynamicDelta > accelThreshold) ||
                                   (baselineDelta > (accelThreshold * 1.10f)) ||
                                   (gyroRate > gyroThreshold);

            if (sampleDisturbed && (now > armingGracePeriodUntilMs)) {
                if (motionDebounceCount < 6) {
                    motionDebounceCount++;
                }
                if (motionDebounceCount >= 2) {
                    if (!motionFlag) {
                        motionJustTriggeredFlag = true;
                        Serial.print(F("[DEBUG] >>> MOVEMENT DETECTED! MoveVal: "));
                        Serial.print(movementVal, 2);
                        Serial.print(F(" >= Thresh: "));
                        Serial.print(accelThreshold, 2);
                        Serial.println(F(" | Calling alarm function! <<<"));
                    }
                    motionFlag = true;
                    motionHoldUntilMs = now + 1500;
                }
            } else {
                if (motionDebounceCount > 0) {
                    motionDebounceCount--;
                }
                if (now >= motionHoldUntilMs) {
                    motionFlag = false;
                }
            }

            // Periodic Serial Monitor debug reporting (every 1000ms)
            static unsigned long lastDebugPrintMs = 0;
            if (now - lastDebugPrintMs >= 1000) {
                lastDebugPrintMs = now;
                bool isArmed = securityIsArmed();
                Serial.print(F("[DEBUG] Armed: "));
                Serial.print(isArmed ? F("YES") : F("NO "));
                Serial.print(F(" | Accel: X="));
                Serial.print(ax, 2);
                Serial.print(F(" Y="));
                Serial.print(ay, 2);
                Serial.print(F(" Z="));
                Serial.print(az, 2);
                Serial.print(F(" | MoveVal: "));
                Serial.print(movementVal, 2);
                Serial.print(F(" | Thresh: "));
                Serial.print(accelThreshold, 2);
                Serial.print(F(" | Breach: "));
                Serial.println(motionFlag ? F("YES") : F("NO"));
            }
        }
    }

    lastReadings.motionDetected = motionFlag;
}

bool sensorsIsMotionDetected() { return motionFlag; }
bool sensorsIsZipperOpen() { return lastReadings.zipperOpen; }
bool sensorsZipperJustOpened() { return zipperJustOpenedFlag; }
bool sensorsMotionJustTriggered() { return motionJustTriggeredFlag; }

void sensorsClearTriggerFlags() {
    zipperJustOpenedFlag = false;
    zipperJustClosedFlag = false;
    motionJustTriggeredFlag = false;
    magnetMovedFlag = false;
}

SensorReadings sensorsGetReadings() { return lastReadings; }

String sensorsGetJson() {
    String json = "{";
    json += "\"mpuConnected\":" + String(lastReadings.mpuConnected ? "true" : "false") + ",";
    json += "\"motion\":" + String(lastReadings.motionDetected ? "true" : "false") + ",";
    json += "\"zipperOpen\":" + String(lastReadings.zipperOpen ? "true" : "false") + ",";
    json += "\"sensitivity\":\"" + String(sensorsGetSensitivityString()) + "\",";
    json += "\"accel\":{\"x\":" + String(lastReadings.accelX, 2) + ",";
    json += "\"y\":" + String(lastReadings.accelY, 2) + ",";
    json += "\"z\":" + String(lastReadings.accelZ, 2) + ",";
    json += "\"delta\":" + String(lastReadings.deltaAccel, 2) + "},";
    json += "\"gyro\":{\"x\":" + String(lastReadings.gyroX, 2) + ",";
    json += "\"y\":" + String(lastReadings.gyroY, 2) + ",";
    json += "\"z\":" + String(lastReadings.gyroZ, 2) + "},";
    json += "\"temperature\":" + String(lastReadings.tempC, 1);
    json += "}";
    return json;
}