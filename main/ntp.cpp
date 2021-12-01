#include "ntp.h"

#define TAG "ntp.cpp"

#ifdef NTP_SERVER
#define NTP_SERVER_FINAL NTP_SERVER
#else
#define NTP_SERVER_FINAL "pool.ntp.org"
#endif

volatile unsigned long lastSNTPSyncMillis = 0;
volatile bool ntpSyncHappened = false;

SemaphoreHandle_t i2cSemaphoreNTP;

void sntpSyncCallback(timeval *tv)
{
    lastSNTPSyncMillis = millis();
    ntpSyncHappened = true;
    ESP_LOGD(TAG, "lastSNTPSyncMillis: %lu", lastSNTPSyncMillis);
    setRTC(*tv, i2cSemaphoreNTP);
}

void ntpBegin(SemaphoreHandle_t i2cSemaphoreExternal)
{
    i2cSemaphoreNTP = i2cSemaphoreExternal;

    sntp_set_sync_mode(SNTP_SYNC_MODE_IMMED);
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, NTP_SERVER_FINAL);
    sntp_set_time_sync_notification_cb(sntpSyncCallback);
}

uint32_t getLastNTPSyncMillis()
{
    return lastSNTPSyncMillis;
}

bool getNTPSyncHappened()
{
    return ntpSyncHappened;
}

void setLastNTPSyncMillis(uint32_t m)
{
    lastSNTPSyncMillis = m;
}