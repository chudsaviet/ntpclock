#include "nvs_local.h"

#include <esp_log.h>
#include <esp_system.h>
#include <esp_err.h>

#include "abort.h"

#define TAG "nvs_local.cpp"

#define NVS_NAMESPACE "device_settings"

nvs_handle_t nvs_open(nvs_open_mode_t open_mode)
{
    // Initialize NVS
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        // NVS partition was truncated and needs to be erased
        // Retry nvs_flash_init
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    // Open
    ESP_LOGD(TAG, "Opening Non-Volatile Storage (NVS) handle...");
    nvs_handle_t handle = 0;
    err = nvs_open(NVS_NAMESPACE, open_mode, &handle);
    switch (err)
    {
    case ESP_OK:
        ESP_LOGD(TAG, "NVS handle opened.");
        break;
    case ESP_ERR_NVS_NOT_FOUND:
        ESP_LOGE(TAG, "NVS namespace '%s' not found.", NVS_NAMESPACE);
        break;
    default:
        ESP_LOGE(TAG, "Error opening NVS handle! (%s)", esp_err_to_name(err));
        delayed_abort();
    }

    return handle;
}