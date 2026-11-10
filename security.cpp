#include <Arduino.h>
#include "security.h"
#include "alarm.h"
#include "sensors.h"
#include "events.h"
#include "battery.h"
#include "config.h"

// Internal security states
static SystemState currentSystemState = STATE_DISARMED;
static AlarmCause currentAlarmCause = ALARM_CAUSE_NONE;
static String currentSecurityPin = DEFAULT_SECURITY_PIN;

void securityInit() {
    currentSystemState = STATE_DISARMED;
    currentAlarmCause = ALARM_CAUSE_NONE;
}

bool securityIsArmed() {
    return (currentSystemState == STATE_ARMED || currentSystemState == STATE_ALARM);
}

bool securityIsAlarmActive() {
    return (currentSystemState == STATE_ALARM);
}

SystemState securityGetState() {
    return currentSystemState;
}

const char* securityGetStateString() {
    switch (currentSystemState) {
        case STATE_DISARMED: return "DISARMED";
        case STATE_ARMED:    return "ARMED";
        case STATE_WARNING:  return "WARNING";
        case STATE_ALARM:    return "ALARM";
        case STATE_FIND_MY_BAG: return "FIND_MY_BAG";
        default:             return "UNKNOWN";
    }
}

AlarmCause securityGetAlarmCause() {
    return currentAlarmCause;
}

const char* securityGetAlarmCauseString() {
    switch (currentAlarmCause) {
        case ALARM_CAUSE_NONE:   return "NONE";
        case ALARM_CAUSE_MOTION: return "MOTION";
        case ALARM_CAUSE_ZIPPER: return "ZIPPER";
        case ALARM_CAUSE_FIND:   return "FIND";
        default:                 return "UNKNOWN";
    }
}

bool securityValidatePin(const String& pin) {
    return pin == currentSecurityPin;
}

bool securityArm(const String& pin) {
    if (!securityValidatePin(pin)) {
        alarmPlayErrorTone();
        eventsAdd("SECURITY", "Arm failed: Invalid PIN");
        return false;
    }

    currentSystemState = STATE_ARMED;
    currentAlarmCause = ALARM_CAUSE_NONE;
    alarmSetGreenLed(false);
    alarmSetRedLedMode(RED_LED_BLINK_SLOW); 
    alarmStopBuzzer();                      
    alarmPlayConfirmChirp();                
    eventsAdd("SECURITY", "System Armed");
    sensorsResetBaseline();
    return true;
}

bool securityDisarm(const String& pin) {
    if (!securityValidatePin(pin)) {
        alarmPlayErrorTone();
        eventsAdd("SECURITY", "Disarm failed: Invalid PIN");
        return false;
    }

    currentSystemState = STATE_DISARMED;
    currentAlarmCause = ALARM_CAUSE_NONE;
    alarmSetGreenLed(true);
    alarmSetRedLedMode(RED_LED_OFF);
    alarmStopBuzzer();                      
    alarmPlayConfirmChirp();
    eventsAdd("SECURITY", "System Disarmed");
    return true;
}

bool securityStopAlarm(const String& pin) {
    if (!securityValidatePin(pin)) {
        alarmPlayErrorTone();
        return false;
    }

    currentSystemState = STATE_ARMED;
    currentAlarmCause = ALARM_CAUSE_NONE;
    alarmStopBuzzer();
    alarmSetRedLedMode(RED_LED_BLINK_SLOW);
    alarmPlayConfirmChirp();
    eventsAdd("SECURITY", "Alarm Stopped via PIN");
    sensorsResetBaseline();
    return true;
}

bool securityFindMyBag(const String& pin) {
    if (!securityValidatePin(pin)) {
        alarmPlayErrorTone();
        return false;
    }

    currentSystemState = STATE_FIND_MY_BAG;
    currentAlarmCause = ALARM_CAUSE_FIND;
    alarmSetBuzzerPattern(BUZZER_PATTERN_FIND);
    eventsAdd("SECURITY", "Find My Bag triggered");
    return true;
}

bool securityChangePin(const String& oldPin, const String& newPin) {
    if (!securityValidatePin(oldPin)) {
        alarmPlayErrorTone();
        eventsAdd("SECURITY", "Change PIN failed: Old PIN incorrect");
        return false;
    }

    if (newPin.length() < 4) {
        alarmPlayErrorTone();
        eventsAdd("SECURITY", "Change PIN failed: Too short");
        return false;
    }

    currentSecurityPin = newPin;
    alarmPlayConfirmChirp();
    eventsAdd("SECURITY", "Security PIN changed successfully");
    return true;
}

void securityUpdate() {
    // If the system is DISARMED, completely ignore sensors
    if (currentSystemState == STATE_DISARMED || currentSystemState == STATE_FIND_MY_BAG) {
        return;
    }

    // Check sensors ONLY when the system is actively armed
    if (currentSystemState == STATE_ARMED) {
        bool zipperOpened = sensorsIsZipperOpen(); // Magnetic reed switch
        bool motionDetected = sensorsIsMotionDetected(); // MPU6050 motion sensor

        if (zipperOpened) {
            Serial.println(F("[SECURITY] >>> ZIPPER BREACH: Magnet removed! Calling alarm function."));
            currentSystemState = STATE_ALARM;
            currentAlarmCause = ALARM_CAUSE_ZIPPER;
            alarmSetBuzzerPattern(BUZZER_PATTERN_ZIPPER);
            alarmSetRedLedMode(RED_LED_BLINK_FAST);
            eventsAdd("BREACH", "Zipper/Magnet Opened!");
        } 
        else if (motionDetected) {
            Serial.println(F("[SECURITY] >>> MOTION BREACH: MPU6050 detected movement! Calling alarm function."));
            currentSystemState = STATE_ALARM;
            currentAlarmCause = ALARM_CAUSE_MOTION;
            alarmSetBuzzerPattern(BUZZER_PATTERN_ZIPPER);
            alarmSetRedLedMode(RED_LED_BLINK_FAST);
            eventsAdd("BREACH", "Unauthorized bag motion detected!");
        }
    }
}
String securityGetStatusJson() {
    bool isArmedState = securityIsArmed();
    bool alarmActive = securityIsAlarmActive();

    String json = "{";
    json += "\"armed\":" + String(isArmedState ? "true" : "false") + ",";
    json += "\"alarm\":" + String(alarmActive ? "true" : "false") + ",";
    json += "\"state\":\"" + String(securityGetStateString()) + "\",";
    json += "\"alarmCause\":\"" + String(securityGetAlarmCauseString()) + "\",";
    json += "\"reedOpen\":" + String(sensorsIsZipperOpen() ? "true" : "false") + ",";
    json += "\"battery\":" + String(batteryGetPercentage()) + ",";
    json += "\"batteryState\":\"" + String(batteryGetStateString()) + "\",";
    json += "\"sensitivity\":\"" + String(sensorsGetSensitivityString()) + "\"";
    json += "}";
    return json;
}