#include "nvs_local.h"

#include <memory.h>
#include <esp_log.h>
#include <esp_system.h>
#include <esp_err.h>
#include <nvs.h>
#include <nvs_flash.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

#include "abort.h"

#define TAG "nvs_local.cpp"

#define NVS_NAMESPACE "device_settings"

#define NVS_SEMAPTHORE_WAIT_TIME_MS 1000
#define NVS_GET_STRING_BUFFER_SIZE_BYTES 256

static nvs_handle_t nvs = 0;
static SemaphoreHandle_t nvs_semaphore = 0;

void nvs_begin()
{
    nvs_semaphore = xSemaphoreCreateBinary();
    
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
    err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs);
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

    xSemaphoreGive(nvs_semaphore);
}

void l_nvs_set_str(const char* key, const char* value) {
    ESP_LOGD(TAG, "Attempting to write NVS key <%s>.", key);
    if (xSemaphoreTake(nvs_semaphore, NVS_SEMAPTHORE_WAIT_TIME_MS) != pdTRUE) {
        ESP_LOGE(TAG, "Cannot take NVS semaphore for writing key <%s>.", key);
        delayed_abort();
    }
    nvs_set_str(nvs, key, value);
    nvs_commit(nvs);
    xSemaphoreGive(nvs_semaphore);
}

void l_nvs_set_i64 (const char* key, int64_t value) {
    ESP_LOGD(TAG, "Attempting to write NVS key <%s>.", key);
    if (xSemaphoreTake(nvs_semaphore, NVS_SEMAPTHORE_WAIT_TIME_MS) != pdTRUE) {
        ESP_LOGE(TAG, "Cannot take NVS semaphore for writing key <%s>.", key);
        delayed_abort();
    }

    nvs_set_i64(nvs, key, value);

    nvs_commit(nvs);
    xSemaphoreGive(nvs_semaphore);
}

void l_nvs_set_array(const char* key, const void* value, size_t length) {
    ESP_LOGD(TAG, "Attempting to write NVS key <%s>.", key);
    if (xSemaphoreTake(nvs_semaphore, NVS_SEMAPTHORE_WAIT_TIME_MS) != pdTRUE) {
        ESP_LOGE(TAG, "Cannot take NVS semaphore for writing key <%s>.", key);
        delayed_abort();
    }
    
    nvs_set_blob(nvs, key, value, length);

    nvs_commit(nvs);
    xSemaphoreGive(nvs_semaphore);
}

esp_err_t l_nvs_get_str(const char* key, char* out_value, size_t buffer_length) {
    char buffer[NVS_GET_STRING_BUFFER_SIZE_BYTES];
    memset(buffer, 0, NVS_GET_STRING_BUFFER_SIZE_BYTES);
    
    size_t read_bytes = NVS_GET_STRING_BUFFER_SIZE_BYTES;

    ESP_LOGD(TAG, "Attempting to read NVS key <%s>.", key);
    if (xSemaphoreTake(nvs_semaphore, NVS_SEMAPTHORE_WAIT_TIME_MS) != pdTRUE) {
        ESP_LOGE(TAG, "Cannot take NVS semaphore for reading key <%s>.", key);
        delayed_abort();
    }

    esp_err_t err = ESP_OK;
    if (nvs_get_str(nvs, key, buffer, &read_bytes) != ESP_OK) {
        ESP_LOGE(TAG, "Cannot read NVS key <%s>.", key);
        err = ESP_FAIL;
    } else {
        ESP_LOGD(TAG, "Read %d bytes for NVS key <%s>.", read_bytes, key);
        if (read_bytes > buffer_length) {
            ESP_LOGE(TAG, "Output buffer for NVS key <%s> is %d bytes, which is too small for value of %d bytes.", key, buffer_length, read_bytes);
            err = ESP_FAIL;
        } else {
            memcpy(out_value, buffer, read_bytes);
        }
    }

    xSemaphoreGive(nvs_semaphore);
    return err;
}

esp_err_t l_nvs_get_i64 (const char* key, int64_t* out_value) {
    ESP_LOGD(TAG, "Attempting to read NVS key <%s>.", key);
    if (xSemaphoreTake(nvs_semaphore, NVS_SEMAPTHORE_WAIT_TIME_MS) != pdTRUE) {
        ESP_LOGE(TAG, "Cannot take NVS semaphore for reading key <%s>.", key);
        delayed_abort();
    }

    esp_err_t err = nvs_get_i64(nvs, key, out_value);

    xSemaphoreGive(nvs_semaphore);
    return err;
}

esp_err_t l_nvs_get_array(const char* key, void* out_value, size_t length) {
    size_t read_bytes = length;

    ESP_LOGD(TAG, "Attempting to read NVS key <%s>.", key);
    if (xSemaphoreTake(nvs_semaphore, NVS_SEMAPTHORE_WAIT_TIME_MS) != pdTRUE) {
        ESP_LOGE(TAG, "Cannot take NVS semaphore for reading key <%s>.", key);
        delayed_abort();
    }

    esp_err_t err = nvs_get_blob(nvs, key, out_value, &read_bytes);
    if (read_bytes != length) {
        ESP_LOGE(TAG, "Wrong array size %d read for key <%s>. %d was expected.", read_bytes, key, length);
        err = ESP_FAIL;
    }

    xSemaphoreGive(nvs_semaphore);
    return err;
}