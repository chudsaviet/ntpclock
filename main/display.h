#pragma once

#include <stdint.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>

#define DISPLAY_PAYLOAD_SIZE_BYTES 32

enum DisplayCommand
{
    SET_BRIGHTNESS,
    SET_BLINK_COLONS,
    SET_SHOW_TIME,
    SET_SHOW_TEXT
};

struct DisplayCommandMessage
{
    DisplayCommand command;
    uint8_t payload[DISPLAY_PAYLOAD_SIZE_BYTES];
};

void vStartDisplayTask(TaskHandle_t *task_handle, QueueHandle_t *queue_handle, SemaphoreHandle_t i2cSemaphore);