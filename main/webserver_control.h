#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>

enum WebserverCommand
{
    START
};

struct WebserverMessage
{
    WebserverCommand command;
};

void vStartWebserverTask(TaskHandle_t *xTaskHandle, QueueHandle_t *xCommandQueue);