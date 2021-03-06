#include "filesystem.h"

#include "abort.h"

static const char *TAG = "filesystem.cpp";

#define SPIFFS_MAX_FILES 64

void filesystem_init()
{
    ESP_LOGI(TAG, "Initializing SPIFFS.");

    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = NULL,
        .max_files = SPIFFS_MAX_FILES,
        .format_if_mount_failed = false};

    esp_err_t ret = esp_vfs_spiffs_register(&conf);
    if (ret != ESP_OK)
    {
        if (ret == ESP_FAIL)
        {
            ESP_LOGE(TAG, "Failed to mount or format filesystem.");
        }
        else if (ret == ESP_ERR_NOT_FOUND)
        {
            ESP_LOGE(TAG, "Failed to find SPIFFS partition.");
        }
        else
        {
            ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s).", esp_err_to_name(ret));
        }
        delayed_abort();
    }

    size_t total = 0, used = 0;
    ret = esp_spiffs_info(conf.partition_label, &total, &used);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s).", esp_err_to_name(ret));
    }
    else
    {
        ESP_LOGI(TAG, "Partition size: total: %d bytes, used: %d bytes.", total, used);
    }
}