#ifndef SECURITY_H
#define SECURITY_H

#include <Arduino.h>

enum SystemState {
    STATE_DISARMED,
    STATE_ARMED,
    STATE_WARNING,
    STATE_ALARM,
    STATE_FIND_MY_BAG
};

enum AlarmCause {
    ALARM_CAUSE_NONE,
    ALARM_CAUSE_MOTION,
    ALARM_CAUSE_ZIPPER,
    ALARM_CAUSE_FIND
};

void securityInit();
void securityUpdate();

// Authentication & control actions
bool securityValidatePin(const String& pin);
bool securityChangePin(const String& oldPin, const String& newPin);

bool securityArm(const String& pin);
bool securityDisarm(const String& pin);
bool securityStopAlarm(const String& pin);
bool securityFindMyBag(const String& pin);

// State queries
SystemState securityGetState();
const char* securityGetStateString();
AlarmCause securityGetAlarmCause();
const char* securityGetAlarmCauseString();
bool securityIsArmed();
bool securityIsAlarmActive();

// Complete state JSON representation
String securityGetStatusJson();

#endif // SECURITY_H