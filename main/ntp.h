#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>

enum NtpEvent
{
    SYNC_HAPPENED,
    SYNC_TIMEOUT
};

struct NtpMessage
{
    NtpEvent event;
};

void vStartNtpTask(TaskHandle_t *taskHandle, QueueHandle_t *outputQueue, SemaphoreHandle_t i2cSemaphore);