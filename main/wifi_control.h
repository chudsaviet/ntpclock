#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"

#define WIFI_MESSAGE_PAYLOAD_SIZE_BYTES 32

enum WifiCommand
{
    SWITCH_TO_AP_MODE
};

struct WifiCommandMessage
{
    WifiCommand command;
    uint8_t payload[WIFI_MESSAGE_PAYLOAD_SIZE_BYTES];
};

void vStartWifiTask(TaskHandle_t *taskHandle, QueueHandle_t *queueHandle);