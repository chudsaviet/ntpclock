#include <Arduino.h>
#include <TimeLib.h>
#include <TimeZoneInfo.h>

#include "wifi.h"
#include "ntp.h"
#include "timezone.h"
#include "display.h"

TimeZoneInfo tzInfo;

time_t getLocalTime()
{
  tzInfo.utc2local(ntpGetEpochTime());
}

void setup()
{
  Serial.begin(9600);

  wifiBegin();
  wifiLowPower();

  tzInfo.setLocation_P(tzData);

  setSyncProvider(getLocalTime);
  setSyncInterval(300);

  displayBegin();
}

bool blink = false;
void loop()
{
  display(now(), blink);

  blink = !blink;
  delay(1000);
}