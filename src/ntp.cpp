#include "ntp.h"

#define ATTEMPT_DELAY_MS 3803
#define ATTEMPTS 3

#ifdef NTP_SERVER
#define NTP_SERVER_FINAL NTP_SERVER
#else
#define NTP_SERVER_FINAL "pool.ntp.org"
#endif

timespec ntpGetTime()
{
    Serial.println("Attempting to get time using NTP.");

    IPAddress ntpServerIPAddress;
    int err = WiFi.hostByName(NTP_SERVER_FINAL, ntpServerIPAddress);
    if (err == 1)
    {
        Serial.print("NTP server: ");
        Serial.println(ntpServerIPAddress);
    }
    else
    {
        Serial.print("Error resolving NTP server name, code: ");
        Serial.println(err);
    }

    WiFiUDP wifiUdp;
    NTPClient timeClient(wifiUdp, ntpServerIPAddress);

    timeClient.begin();

    int attempt = 0;
    Serial.print("Attempting to query NTP server.");
    while (!timeClient.forceUpdate())
    {
        attempt++;
        if (attempt > ATTEMPTS)
        {
            Serial.println("NTP cannot query server.");
            timeClient.end();
            return {0, 0};
        }

        Serial.print("Attempt ");
        Serial.print(attempt);
        Serial.println(" failed.");
        wifiReconnect();
        delay(ATTEMPT_DELAY_MS * attempt);
    }

    timeClient.end();

    time_t tv_sec = static_cast<time_t>(timeClient.getEpochTime()) + (NTP_ERA * 4294967296LL);
    long tv_nsec = static_cast<long>(static_cast<double>(timeClient.get_millis()) * 1000000.0);

    return {tv_sec, tv_nsec};
}
