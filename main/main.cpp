#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <Arduino.h>

#include <esp_sntp.h>
#include <nvs_flash.h>

#include "arduino_main.h"
#include "arduino_task.h"
#include "start_wifi.h"
#include "secrets.h"

static const char *TAG = "main.cpp";

TaskHandle_t xArduinoTaskHandle = NULL;

extern "C" void app_main(void)
{
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

    // Start WiFi.
    ESP_LOGI(TAG, "Starting WiFi");
    start_wifi();

    // Start SNTP.
    sntp_init();

    // Work indefinitely.
    while (true)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}