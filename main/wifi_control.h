#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"

#define WIFI_MESSAGE_PAYLOAD_SIZE_BYTES 32
#define WIFI_SSID_MAX_LEN_CHARS 32

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

void vWifiSaveStaSsid(char *ssid);
void vWifiSaveStaPass(char *pass);

char *xGetWifiStaSsid();
int16_t usScanWifi(wifi_ap_record_t **ap_info);