#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>

#define NTP_SERVER_MAX_LEN_CHARS 256

enum NtpEvent
{
    SYNC_HAPPENED,
    SYNC_TIMEOUT
};

struct NtpMessage
{
    NtpEvent event;
};

const char *xGetNtpServer();
void vWaitForNtpSync();
void vSetNtpServer(char *address);

void vStartNtpTask(TaskHandle_t *taskHandle, QueueHandle_t *outputQueue, SemaphoreHandle_t i2cSemaphore);