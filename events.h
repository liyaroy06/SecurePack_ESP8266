#ifndef EVENTS_H
#define EVENTS_H

#include <Arduino.h>

#define MAX_EVENTS 25

struct SecurityEvent {
  uint32_t id;
  char timestamp[16];   // e.g. "10:30:15 AM" or "00:05:22"
  char type[24];        // e.g. "ARMED", "MOTION_DETECTED", "ZIPPER_OPENED"
  char message[64];     // Human readable description
};

void eventsInit();
void eventsAdd(const char* type, const char* message);
void eventsClear();
String eventsGetJson();
uint32_t eventsGetCount();

// Helper to provide formatted current time from system uptime or GPS
void eventsSetTimeProvider(String (*provider)());

#endif // EVENTS_H
