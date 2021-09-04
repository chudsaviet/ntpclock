#include "ntp.h"

#define ATTEMPT_DELAY_MS 3803
#define ATTEMPTS 8

#ifdef NTP_SERVER
#define NTP_SERVER_FINAL NTP_SERVER
#else
#define NTP_SERVER_FINAL "pool.ntp.org"
#endif

timespec ntpGetTime()
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
            return {0, 0};
        }

        Serial.print("Attempt ");
        Serial.print(attempt);
        Serial.println(" failed.");
        wifiReconnect();
        delay(ATTEMPT_DELAY_MS);
    }

    timeClient.end();
    wifiLowPower();

    time_t tv_sec = static_cast<time_t>(timeClient.getEpochTime()) + (NTP_ERA * 4294967296LL);
    long tv_nsec = static_cast<long>(static_cast<double>(timeClient.get_millis()) * 1000000.0);

    return {tv_sec, tv_nsec};
}
