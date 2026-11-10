#include <Arduino.h>
#include "alarm.h"
#include "config.h"

static BuzzerPattern currentBuzzerPattern = BUZZER_PATTERN_OFF;
static RedLedMode currentRedLedMode = RED_LED_OFF;

static bool buzzerPinState = false;
static bool greenLedState = false;
static bool blueLedState = false;

static unsigned long buzzerStepStartMs = 0;
static int buzzerStep = 0;

static unsigned long redLedStepStartMs = 0;
static bool redLedBlinkState = false;

static bool isOneShot = false;

void alarmInit() {
    // FORCE LOW IMMEDIATELY BEFORE PINMODE (Active-HIGH: LOW = OFF)
    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, LOW); // Explicitly OFF at boot

    pinMode(RED_LED_PIN, OUTPUT);
    pinMode(GREEN_LED_PIN, OUTPUT);
    pinMode(BLUE_LED_PIN, OUTPUT);

    digitalWrite(RED_LED_PIN, LOW);
    digitalWrite(GREEN_LED_PIN, LOW);
    digitalWrite(BLUE_LED_PIN, LOW);

    buzzerStepStartMs = millis();
    redLedStepStartMs = millis();
}

void alarmSetBuzzerPattern(BuzzerPattern pattern) {
    if (currentBuzzerPattern == pattern && !isOneShot) return;
    currentBuzzerPattern = pattern;
    buzzerStep = 0;
    buzzerStepStartMs = millis();
    isOneShot = false;

    if (pattern == BUZZER_PATTERN_OFF) {
        buzzerPinState = false;
        digitalWrite(BUZZER_PIN, LOW); // Ensure OFF
    }
}

BuzzerPattern alarmGetBuzzerPattern() {
    return currentBuzzerPattern;
}

void alarmStopBuzzer() {
    currentBuzzerPattern = BUZZER_PATTERN_OFF;
    buzzerStep = 0;
    isOneShot = false;
    buzzerPinState = false;
    digitalWrite(BUZZER_PIN, LOW); // Force OFF
}

void alarmPlayConfirmChirp() {
    currentBuzzerPattern = BUZZER_PATTERN_CONFIRM;
    buzzerStep = 0;
    buzzerStepStartMs = millis();
    isOneShot = true;
}

void alarmPlayErrorTone() {
    currentBuzzerPattern = BUZZER_PATTERN_ERROR;
    buzzerStep = 0;
    buzzerStepStartMs = millis();
    isOneShot = true;
}

void alarmSetGreenLed(bool on) {
    greenLedState = on;
    digitalWrite(GREEN_LED_PIN, on ? HIGH : LOW);
}

void alarmSetBlueLed(bool on) {
    blueLedState = on;
    digitalWrite(BLUE_LED_PIN, on ? HIGH : LOW);
}

void alarmSetRedLedMode(RedLedMode mode) {
    if (currentRedLedMode == mode) return;
    currentRedLedMode = mode;
    redLedStepStartMs = millis();
    redLedBlinkState = false;

    switch (mode) {
        case RED_LED_OFF:
            digitalWrite(RED_LED_PIN, LOW);
            break;
        case RED_LED_ON:
            digitalWrite(RED_LED_PIN, HIGH);
            break;
        case RED_LED_BLINK_FAST:
        case RED_LED_BLINK_SLOW:
            digitalWrite(RED_LED_PIN, HIGH);
            redLedBlinkState = true;
            break;
    }
}

void alarmUpdate() {
    unsigned long now = millis();

    // 1. Red LED State Machine
    if (currentRedLedMode == RED_LED_BLINK_FAST) {
        if (now - redLedStepStartMs >= 120) {
            redLedStepStartMs = now;
            redLedBlinkState = !redLedBlinkState;
            digitalWrite(RED_LED_PIN, redLedBlinkState ? HIGH : LOW);
        }
    } else if (currentRedLedMode == RED_LED_BLINK_SLOW) {
        if (now - redLedStepStartMs >= 400) {
            redLedStepStartMs = now;
            redLedBlinkState = !redLedBlinkState;
            digitalWrite(RED_LED_PIN, redLedBlinkState ? HIGH : LOW);
        }
    }

    // 2. Buzzer State Machine (Active-HIGH: LOW = OFF, HIGH = ON)
    if (currentBuzzerPattern == BUZZER_PATTERN_OFF) {
        digitalWrite(BUZZER_PIN, LOW); // SAFEGUARD: Force pin LOW (OFF)
        return;
    }

    switch (currentBuzzerPattern) {
        case BUZZER_PATTERN_OFF:
            digitalWrite(BUZZER_PIN, LOW);
            break;

        case BUZZER_PATTERN_MOTION:
        case BUZZER_PATTERN_ZIPPER:
            if (now - buzzerStepStartMs >= 60) {
                buzzerStepStartMs = now;
                buzzerPinState = !buzzerPinState;
                digitalWrite(BUZZER_PIN, buzzerPinState ? HIGH : LOW);
            }
            break;

        case BUZZER_PATTERN_FIND:
            if (buzzerStep == 0) {
                digitalWrite(BUZZER_PIN, HIGH);
                if (now - buzzerStepStartMs >= 100) { buzzerStepStartMs = now; buzzerStep = 1; }
            } else if (buzzerStep == 1) {
                digitalWrite(BUZZER_PIN, LOW);
                if (now - buzzerStepStartMs >= 80) { buzzerStepStartMs = now; buzzerStep = 2; }
            } else if (buzzerStep == 2) {
                digitalWrite(BUZZER_PIN, HIGH);
                if (now - buzzerStepStartMs >= 100) { buzzerStepStartMs = now; buzzerStep = 3; }
            } else if (buzzerStep == 3) {
                digitalWrite(BUZZER_PIN, LOW);
                if (now - buzzerStepStartMs >= 1200) { buzzerStepStartMs = now; buzzerStep = 0; }
            }
            break;

        case BUZZER_PATTERN_CONFIRM:
            if (buzzerStep == 0) {
                digitalWrite(BUZZER_PIN, HIGH);
                if (now - buzzerStepStartMs >= 80) {
                    buzzerStepStartMs = now;
                    digitalWrite(BUZZER_PIN, LOW);
                    buzzerStep = 1;
                }
            } else if (buzzerStep == 1) {
                currentBuzzerPattern = BUZZER_PATTERN_OFF;
                isOneShot = false;
            }
            break;

        case BUZZER_PATTERN_ERROR:
            if (buzzerStep == 0) {
                digitalWrite(BUZZER_PIN, HIGH);
                if (now - buzzerStepStartMs >= 180) {
                    buzzerStepStartMs = now;
                    digitalWrite(BUZZER_PIN, LOW);
                    buzzerStep = 1;
                }
            } else if (buzzerStep == 1) {
                if (now - buzzerStepStartMs >= 90) {
                    buzzerStepStartMs = now;
                    digitalWrite(BUZZER_PIN, HIGH);
                    buzzerStep = 2;
                }
            } else if (buzzerStep == 2) {
                if (now - buzzerStepStartMs >= 180) {
                    digitalWrite(BUZZER_PIN, LOW);
                    currentBuzzerPattern = BUZZER_PATTERN_OFF;
                    isOneShot = false;
                }
            }
            break;
    }
}