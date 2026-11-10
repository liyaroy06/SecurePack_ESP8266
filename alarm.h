#ifndef ALARM_H
#define ALARM_H

#include <Arduino.h>

enum BuzzerPattern {
  BUZZER_PATTERN_OFF,
  BUZZER_PATTERN_MOTION,     // Repeated short beeps
  BUZZER_PATTERN_ZIPPER,     // Fast repeated beeps
  BUZZER_PATTERN_FIND,       // Periodic double beeps
  BUZZER_PATTERN_CONFIRM,    // Single confirmation chirp
  BUZZER_PATTERN_ERROR       // Error alert buzz
};

enum RedLedMode {
  RED_LED_OFF,
  RED_LED_ON,
  RED_LED_BLINK_FAST,        // In ALARM state
  RED_LED_BLINK_SLOW         // In WARNING state
};

void alarmInit();
void alarmUpdate();

// Buzzer control
void alarmSetBuzzerPattern(BuzzerPattern pattern);
BuzzerPattern alarmGetBuzzerPattern();
void alarmStopBuzzer();

// LED control
void alarmSetGreenLed(bool on);
void alarmSetBlueLed(bool on);
void alarmSetRedLedMode(RedLedMode mode);

// One-shot feedback sound
void alarmPlayConfirmChirp();
void alarmPlayErrorTone();

#endif // ALARM_H
