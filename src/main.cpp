#include <Arduino.h>

#include "wifi.h"
#include "ntp.h"

void setup()
{
  Serial.begin(9600);

  wifiBegin();
  wifiLowPower();
}

void loop()
{
  unsigned long ntp_time = ntpGetEpochTime();
  Serial.print("NTP epoch time: ");
  Serial.println(ntp_time);
  delay(10000);
}