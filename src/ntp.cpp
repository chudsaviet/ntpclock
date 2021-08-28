#include "ntp.h"

#define ATTEMPT_DELAY_MS 3803
#define ATTEMPTS 8

#ifdef NTP_SERVER
#define NTP_SERVER_FINAL NTP_SERVER
#else
#define NTP_SERVER_FINAL "pool.ntp.org"
#endif

time_t ntpGetEpochTime()
{
    Serial.println("Attempting to get time using NTP.");
    wifiNoLowPower();

    WiFiUDP wifiUdp;
    NTPClient timeClient(wifiUdp, NTP_SERVER_FINAL);

    timeClient.begin();

    int attempt = 0;
    Serial.print("Attempting to query NTP server ");
    Serial.println(NTP_SERVER_FINAL);
    while (!timeClient.forceUpdate())
    {
        attempt++;
        if (attempt > ATTEMPTS)
        {
            Serial.println("NTP cannot query server.");
            timeClient.end();
            wifiLowPower();
            return 0;
        }

        Serial.print("Attempt ");
        Serial.print(attempt);
        Serial.println(" failed.");
        delay(ATTEMPT_DELAY_MS);
    }

    timeClient.end();
    wifiLowPower();

    // TODO(before 2038): make timeClient use 64-bit time.
    return (time_t)timeClient.getEpochTime();
}