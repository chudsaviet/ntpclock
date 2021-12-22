#pragma once

// Can't simply include `time.h` because of conflict with Arduino's `time.h`.
// `esp_sntp.h` seems to include the right one that have `timeval` defined.
#include <esp_sntp.h>

#define RTC_MESSAGE_PAYLOAD_SIZE_BYTES 1

enum RtcCommand
{
    SET_RTC_AUTO_SYSTEM_CLOCK_SYNC,
    DO_SYSTEM_CLOCK_SYNC,
};

struct RtcMessage
{
    RtcCommand command;
    uint8_t payload[RTC_MESSAGE_PAYLOAD_SIZE_BYTES];
};

void vStartRtcTask(TaskHandle_t *taskHandle, QueueHandle_t *outputQueue, SemaphoreHandle_t i2cSemaphoreParameter);

void setRTC(timeval time, SemaphoreHandle_t i2cSemaphoreParameter);