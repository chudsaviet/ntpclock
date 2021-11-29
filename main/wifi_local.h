#pragma once

#include <Arduino.h>
#include <SPI.h>
#include <WiFi.h>

#include "secrets.h"

bool wifiBegin();

// You won't be able to begin again if you call end.
void wifiEnd();
bool wifiConnect();
void wifiDisconnect();
bool wifiReconnect();