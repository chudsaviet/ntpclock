#pragma once

#include <stdint.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>

struct AlsDataMessage
{
    float lux;
};

void vStartAlsTask(TaskHandle_t *taskHandle, QueueHandle_t *outputQueueHandle, SemaphoreHandle_t i2cSemaphore);