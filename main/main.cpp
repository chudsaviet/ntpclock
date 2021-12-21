#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <Arduino.h>

#include <esp_sntp.h>
#include <nvs_flash.h>

#include <button.h>

#include "arduino_main.h"
#include "arduino_task.h"
#include "wifi_control.h"
#include "filesystem.h"
#include "unique_id.h"
#include "wpa_key_gen.h"
#include "display.h"
#include "als.h"
#include "ntp.h"

#include "secrets.h"

#define SET_BUTTON_GPIO 0
#define MAIN_LOOP_DELAY_MS 500

static const char *TAG = "main.cpp";

static TaskHandle_t xArduinoTaskHandle = NULL;
static TaskHandle_t xWifiControlTaskHandle = NULL;

static TaskHandle_t xDisplayTask = NULL;
static QueueHandle_t xDisplayTaskQueue = 0;

static TaskHandle_t xAlsTask = NULL;
static QueueHandle_t xAlsOutputQueue = 0;

static TaskHandle_t xNtpTask = NULL;
static QueueHandle_t xNtpOutputQueue = 0;

extern "C" void app_main(void)
{
    // Init unique ID.
    init_unique_id();

    // Init filesystem.
    filesystem_init();

    // Create I2C semaphore.
    SemaphoreHandle_t i2cSemaphore = xSemaphoreCreateBinary();
    xSemaphoreGive(i2cSemaphore);

    // Start Arduino task.
    initArduino();
    arduinoSetup(i2cSemaphore);
    xTaskCreatePinnedToCore(
        vArduinoTask,
        "Arduino loop",
        configMINIMAL_STACK_SIZE * 2,
        NULL,
        tskIDLE_PRIORITY,
        &xArduinoTaskHandle,
        1);

    // Start displaying task.
    ESP_LOGI(TAG, "Starting display task.");
    vStartDisplayTask(&xDisplayTask, &xDisplayTaskQueue, i2cSemaphore);

    // Start ALS tast.
    ESP_LOGI(TAG, "Starting ALS task.");
    vStartAlsTask(&xAlsTask, &xAlsOutputQueue, i2cSemaphore);

    //Initialize NVS (Non-Volatile Storage, flash).
    ESP_LOGI(TAG, "Initializing NVS.");
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Starting WiFi control task.
    xTaskCreatePinnedToCore(
        vWifiControlTask,
        "WiFi control",
        configMINIMAL_STACK_SIZE * 4,
        NULL,
        tskIDLE_PRIORITY,
        &xWifiControlTaskHandle,
        1);

    // Start SNTP.
    vStartNtpTask(&xNtpTask, &xNtpOutputQueue, i2cSemaphore);

    // Init button watcher task.
    button_event_t ev;
    QueueHandle_t button_events = button_init(PIN_BIT(SET_BUTTON_GPIO));

    // Work indefinitely.
    for (;;)
    {
        if (xQueueReceive(button_events, &ev, (TickType_t)0))
        {
            if ((ev.pin == SET_BUTTON_GPIO) && (ev.event == BUTTON_UP))
            {
                char wpa_key[WPA_KEY_LENGTH_CHARS + 1] = {0};
                generate_wpa_key((char *)&wpa_key);
                ESP_LOGI(TAG, "Generated WPA key: %s", (char *)&wpa_key);
            }
        }

        AlsDataMessage alsDataMessage = {};
        if (xQueueReceive(xAlsOutputQueue, &alsDataMessage, (TickType_t)0))
        {
            ESP_LOGD(TAG, "Got %f lux from ALS. Sending it to display.", alsDataMessage.lux);
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
                DisplayCommandMessage displayCommandMessage = {};
                displayCommandMessage.command = DisplayCommand::SET_BLINK_COLONS;
                displayCommandMessage.payload[0] = (uint8_t) true;
                xQueueSend(xDisplayTaskQueue, &displayCommandMessage, (TickType_t)0);
            }
            break;
            case NtpEvent::SYNC_TIMEOUT:
            {
                ESP_LOGI(TAG, "NtpEvent::SYNC_TIMEOUT received.");
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