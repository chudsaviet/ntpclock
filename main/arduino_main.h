#pragma once

#include <Arduino.h>
#include <esp_log.h>
#include <esp_sntp.h>

#include "ntp.h"
#include "display.h"
#include "rtc.h"
#include "als.h"
#include "display_task.h"

#define MAIN_LOOP_DELAY_INTERVAL_MS 128L
#define DISPLAY_UPDATE_INTERVAL_MS 32L
#define MAX_NTP_SYNC_INTERVAL_MS 3600000L // 1 hour
#define RTC_SYNC_INTERVAL_MS 1800000L     // 30 min

#define BRIGHTNESS_ADJUST_INTERVAL_MS 1024L
#define DEFAULT_BRIGHTNESS 10L

enum SyncSource
{
    none,
    rtc,
    ntp
};

void arduinoSetup();
void arduinoLoop();

SyncSource getLastSyncSource();