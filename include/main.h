#pragma once

#include <Arduino.h>
#include <FreeRTOS.h>
#include <esp_sntp.h>

#include "wifi_local.h"
#include "ntp.h"
#include "display.h"
#include "rtc.h"
#include "als.h"
#include "display_task.h"

#define MAIN_LOOP_DELAY_INTERVAL_MS 128L
#define DISPLAY_UPDATE_INTERVAL_MS 32L
#define MAX_NTP_SYNC_INTERVAL_MS 14400000L // 4 hours
#define RTC_SYNC_INTERVAL_MS 1800000L      // 30 min

#define BRIGHTNESS_ADJUST_INTERVAL_MS 1024L
#define DEFAULT_BRIGHTNESS 10L

enum SyncSource
{
    none,
    rtc,
    ntp
};

SyncSource getLastSyncSource();