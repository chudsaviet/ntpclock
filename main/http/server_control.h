#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>

enum HttpServerCommand
{
    START
};

struct HttpServerMessage
{
    HttpServerCommand command;
};

void vStartHttpServerControlTask(TaskHandle_t *xTaskHandle, QueueHandle_t *xCommandQueue);