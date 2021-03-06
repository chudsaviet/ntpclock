#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <Arduino.h>

#include <esp_sntp.h>

#include <button.h>

#include "wifi_control.h"
#include "filesystem.h"
#include "unique_id.h"
#include "wpa_key_gen.h"
#include "display.h"
#include "als.h"
#include "ntp.h"
#include "rtc.h"
#include "tz_info_local.h"
#include "nvs_local.h"
#include "http/server_control.h"

#include "secrets.h"

#define SET_BUTTON_GPIO 0
#define MAIN_LOOP_DELAY_MS 500

static const char *TAG = "main.cpp";

static QueueHandle_t xButtonEventQueue = 0;

static TaskHandle_t xWifiControlTaskHandle = NULL;

static TaskHandle_t xDisplayTask = NULL;
static QueueHandle_t xDisplayTaskQueue = 0;

static TaskHandle_t xAlsTask = NULL;
static QueueHandle_t xAlsOutputQueue = 0;

static TaskHandle_t xNtpTask = NULL;
static QueueHandle_t xNtpOutputQueue = 0;

static TaskHandle_t xRtcTask = NULL;
static QueueHandle_t xRtcCommandQueue = 0;

static TaskHandle_t xWifiTask = NULL;
static QueueHandle_t xWifiCommandQueue = 0;

static TaskHandle_t xHttpServerControlTask = NULL;
static QueueHandle_t xHttpServerControlQueue = 0;

static TaskHandle_t xTzdataControlTask = NULL;

void setup()
{
    ESP_LOGI(TAG, "Initializing NVS.");
    nvs_begin();

    ESP_LOGI(TAG, "Initializing unique ID.");
    init_unique_id();

    ESP_LOGI(TAG, "Initializing filesystem.");
    filesystem_init();

    ESP_LOGI(TAG, "Creating I2C semaphore.");
    SemaphoreHandle_t i2cSemaphore = xSemaphoreCreateBinary();
    xSemaphoreGive(i2cSemaphore);

    ESP_LOGI(TAG, "Initializing Arduino framework.");
    initArduino();

    ESP_LOGI(TAG, "Starting RTC control task.");
    vStartRtcTask(&xRtcTask, &xRtcCommandQueue, i2cSemaphore);
    RtcMessage rtcMessage = {};
    rtcMessage.command = RtcCommand::DO_SYSTEM_CLOCK_SYNC;
    xQueueSend(xRtcCommandQueue, &rtcMessage, (TickType_t)0);

    ESP_LOGI(TAG, "Starting TZ data control task.");
    vStartTzdataTask(&xTzdataControlTask);

    ESP_LOGI(TAG, "Starting display task.");
    vStartDisplayTask(&xDisplayTask, &xDisplayTaskQueue, i2cSemaphore);

    ESP_LOGI(TAG, "Starting ALS task.");
    vStartAlsTask(&xAlsTask, &xAlsOutputQueue, i2cSemaphore);

    ESP_LOGI(TAG, "Starting WiFi control task.");
    vStartWifiTask(&xWifiTask, &xWifiCommandQueue);

    ESP_LOGI(TAG, "Starting SNTP control task.");
    vStartNtpTask(&xNtpTask, &xNtpOutputQueue, i2cSemaphore);

    ESP_LOGI(TAG, "Starting button watcher task.");
    xButtonEventQueue = button_init(PIN_BIT(SET_BUTTON_GPIO));

    ESP_LOGI(TAG, "Starting WiFi control task.");
    vStartHttpServerControlTask(&xHttpServerControlTask, &xHttpServerControlQueue);
}

void loopIndefinitely()
{
    for (;;)
    {
        button_event_t buttonEvent;
        while (uxQueueMessagesWaiting(xButtonEventQueue) > 0)
        {
            if (xQueueReceive(xButtonEventQueue, &buttonEvent, (TickType_t)0))
            {
                if ((buttonEvent.pin == SET_BUTTON_GPIO) && (buttonEvent.event == BUTTON_UP))
                {
                    char wpa_key[WPA_KEY_LENGTH_CHARS + 1] = {0};
                    generate_wpa_key((char *)&wpa_key);
                    ESP_LOGI(TAG, "Generated WPA key: %s", (char *)&wpa_key);

                    ESP_LOGI(TAG, "Sending WPA key to display.");
                    DisplayCommandMessage displayCommandMessage = {};
                    displayCommandMessage.command = DisplayCommand::SET_SHOW_TEXT;
                    memcpy(displayCommandMessage.payload, &wpa_key, WPA_KEY_LENGTH_CHARS + 1);
                    xQueueSend(xDisplayTaskQueue, &displayCommandMessage, (TickType_t)0);

                    ESP_LOGI(TAG, "Sending WPA key to WiFi control task. Switching to AP mode.");
                    WifiCommandMessage wifiCommandMessage = {};
                    wifiCommandMessage.command = WifiCommand::SWITCH_TO_AP_MODE;
                    memcpy(wifiCommandMessage.payload, &wpa_key, WPA_KEY_LENGTH_CHARS + 1);
                    xQueueSend(xWifiCommandQueue, &wifiCommandMessage, (TickType_t)0);

                    ESP_LOGI(TAG, "Starting webserver.");
                    HttpServerMessage webserverCommandMessage = {};
                    webserverCommandMessage.command = HttpServerCommand::START;
                    xQueueSend(xHttpServerControlQueue, &webserverCommandMessage, (TickType_t)0);
                }
            }
        }

        AlsDataMessage alsDataMessage = {};
        if (xQueueReceive(xAlsOutputQueue, &alsDataMessage, (TickType_t)0))
        {
            ESP_LOGV(TAG, "Got %f lux from ALS. Sending it to display.", alsDataMessage.lux);
            DisplayCommandMessage displayCommandMessage = {};
            displayCommandMessage.command = DisplayCommand::SET_BRIGHTNESS;
            memcpy((void *)displayCommandMessage.payload, (void *)&alsDataMessage.lux, sizeof(float));
            xQueueSend(xDisplayTaskQueue, &displayCommandMessage, (TickType_t)0);
        }

        NtpMessage ntpMessage = {};
        while (uxQueueMessagesWaiting(xNtpOutputQueue) != 0)
        {
            xQueueReceive(xNtpOutputQueue, &ntpMessage, (TickType_t)0);
            switch (ntpMessage.event)
            {
            case NtpEvent::SYNC_HAPPENED:
            {
                ESP_LOGI(TAG, "NtpEvent::SYNC_HAPPENED received.");

                RtcMessage rtcMessage = {};
                rtcMessage.command = RtcCommand::SET_RTC_AUTO_SYSTEM_CLOCK_SYNC;
                rtcMessage.payload[0] = (uint8_t) false;
                xQueueSend(xRtcCommandQueue, &rtcMessage, (TickType_t)0);

                DisplayCommandMessage displayCommandMessage = {};
                displayCommandMessage.command = DisplayCommand::SET_BLINK_COLONS;
                displayCommandMessage.payload[0] = (uint8_t) true;
                xQueueSend(xDisplayTaskQueue, &displayCommandMessage, (TickType_t)0);
            }
            break;
            case NtpEvent::SYNC_TIMEOUT:
            {
                ESP_LOGI(TAG, "NtpEvent::SYNC_TIMEOUT received.");

                RtcMessage rtcMessage = {};
                rtcMessage.command = RtcCommand::SET_RTC_AUTO_SYSTEM_CLOCK_SYNC;
                rtcMessage.payload[0] = (uint8_t) true;
                xQueueSend(xRtcCommandQueue, &rtcMessage, (TickType_t)0);

                DisplayCommandMessage displayCommandMessage = {};
                displayCommandMessage.command = DisplayCommand::SET_BLINK_COLONS;
                displayCommandMessage.payload[0] = (uint8_t) false;
                xQueueSend(xDisplayTaskQueue, &displayCommandMessage, (TickType_t)0);
            }
            break;
            default:
                ESP_LOGE(TAG, "Received unimplemented NTP event: %d .", (uint8_t)ntpMessage.event);
                abort();
                break;
            }
        }

        vTaskDelay(pdMS_TO_TICKS(MAIN_LOOP_DELAY_MS));
    }
}

extern "C" void app_main(void)
{
    setup();
    loopIndefinitely();

    ESP_LOGE(TAG, "Shall never reach end of app_main().");
    abort();
}