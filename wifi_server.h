#ifndef WIFI_SERVER_H
#define WIFI_SERVER_H

#include <Arduino.h>

void wifiServerInit();
void wifiServerUpdate();
uint8_t wifiGetConnectedClients();
String wifiGetIpAddress();

#endif // WIFI_SERVER_H
