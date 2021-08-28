#include <Arduino.h>
#include <TimeLib.h>
#include <TimeZoneInfo.h>

#include "wifi.h"
#include "ntp.h"
#include "timezone.h"

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
}

void printDigits(int digits)
{
  // utility for digital clock display: prints preceding colon and leading 0
  Serial.print(":");
  if (digits < 10)
    Serial.print('0');
  Serial.print(digits);
}

void digitalClockDisplay()
{
  // digital clock display of the time
  Serial.print(hour());
  printDigits(minute());
  printDigits(second());
  Serial.print(" ");
  Serial.print(day());
  Serial.print(".");
  Serial.print(month());
  Serial.print(".");
  Serial.print(year());
  Serial.println();
}

void loop()
{
  digitalClockDisplay();
  delay(10000);
}