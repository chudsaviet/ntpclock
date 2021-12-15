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

#include "secrets.h"

#define SET_BUTTON_GPIO 0
#define BUTTON_QUEUE_TIMEOUT_MS 200

static const char *TAG = "main.cpp";

TaskHandle_t xArduinoTaskHandle = NULL;
TaskHandle_t xWifiControlTaskHandle = NULL;

extern "C" void app_main(void)
{
    // Init unique ID.
    init_unique_id();

    // Init filesystem.
    filesystem_init();

    // Start Arduino task.
    initArduino();
    arduinoSetup();
    xTaskCreatePinnedToCore(
        vArduinoTask,
        "Arduino loop",
        configMINIMAL_STACK_SIZE * 2,
        NULL,
        tskIDLE_PRIORITY,
        &xArduinoTaskHandle,
        1);

    //Initialize NVS (Non-Volatile Storage, flash).
    ESP_LOGI(TAG, "Initializing NVS");
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
    sntp_init();

    // Init button watcher task.
    button_event_t ev;
    QueueHandle_t button_events = button_init(PIN_BIT(SET_BUTTON_GPIO));

    // Work indefinitely.
    for (;;)
    {
        if (xQueueReceive(button_events, &ev, pdMS_TO_TICKS(BUTTON_QUEUE_TIMEOUT_MS)))
        {
            if ((ev.pin == SET_BUTTON_GPIO) && (ev.event == BUTTON_UP))
            {
                char wpa_key[WPA_KEY_LENGTH_CHARS + 1] = {0};
                generate_wpa_key((char *)&wpa_key);
                ESP_LOGI(TAG, "Generated WPA key: %s", (char *)&wpa_key);
            }
        }
    }
}