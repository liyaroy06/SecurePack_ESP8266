#include "events.h"

static SecurityEvent eventBuffer[MAX_EVENTS];
static int eventHead = 0;
static int eventCount = 0;
static uint32_t nextEventId = 1;
static String (*customTimeProvider)() = nullptr;

static String getDefaultTimestamp() {
  if (customTimeProvider != nullptr) {
    String t = customTimeProvider();
    if (t.length() > 0) return t;
  }

  // Fallback to formatted uptime
  unsigned long totalSeconds = millis() / 1000;
  unsigned int hours = (totalSeconds / 3600) % 24;
  unsigned int minutes = (totalSeconds % 3600) / 60;
  unsigned int seconds = totalSeconds % 60;

  char buf[16];
  snprintf(buf, sizeof(buf), "%02u:%02u:%02u", hours, minutes, seconds);
  return String(buf);
}

void eventsInit() {
  eventHead = 0;
  eventCount = 0;
  nextEventId = 1;
  eventsAdd("SYSTEM_BOOT", "SecurePack system booted");
}

void eventsSetTimeProvider(String (*provider)()) {
  customTimeProvider = provider;
}

void eventsAdd(const char* type, const char* message) {
  int index;
  if (eventCount < MAX_EVENTS) {
    index = eventCount;
    eventCount++;
  } else {
    index = eventHead;
    eventHead = (eventHead + 1) % MAX_EVENTS;
  }

  eventBuffer[index].id = nextEventId++;
  
  String timeStr = getDefaultTimestamp();
  strncpy(eventBuffer[index].timestamp, timeStr.c_str(), sizeof(eventBuffer[index].timestamp) - 1);
  eventBuffer[index].timestamp[sizeof(eventBuffer[index].timestamp) - 1] = '\0';

  strncpy(eventBuffer[index].type, type, sizeof(eventBuffer[index].type) - 1);
  eventBuffer[index].type[sizeof(eventBuffer[index].type) - 1] = '\0';

  strncpy(eventBuffer[index].message, message, sizeof(eventBuffer[index].message) - 1);
  eventBuffer[index].message[sizeof(eventBuffer[index].message) - 1] = '\0';
}

void eventsClear() {
  eventHead = 0;
  eventCount = 0;
  eventsAdd("HISTORY_CLEARED", "Event log cleared by user");
}

uint32_t eventsGetCount() {
  return eventCount;
}

String eventsGetJson() {
  String json = "[";
  
  // Return in reverse chronological order (newest first)
  for (int i = 0; i < eventCount; i++) {
    int idx;
    if (eventCount < MAX_EVENTS) {
      idx = eventCount - 1 - i;
    } else {
      idx = (eventHead - 1 - i + MAX_EVENTS) % MAX_EVENTS;
    }

    if (i > 0) json += ",";
    json += "{";
    json += "\"id\":" + String(eventBuffer[idx].id) + ",";
    json += "\"timestamp\":\"" + String(eventBuffer[idx].timestamp) + "\",";
    json += "\"type\":\"" + String(eventBuffer[idx].type) + "\",";
    json += "\"message\":\"" + String(eventBuffer[idx].message) + "\"";
    json += "}";
  }

  json += "]";
  return json;
}
