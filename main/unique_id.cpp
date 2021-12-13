#include "unique_id.h"

#define TAG "unique_id.cpp"

uint8_t global_unique_id[UNIQUE_ID_SIZE_BYTES];

/*
    Generate new unique ID.
    The problem here is that ESP32 MAC addresses are not unique.
    They are set per silicon batch.
    Parameters:
        output - 20 bytes buffer.
*/
void generate_unique_id(uint8_t *output)
{
    // Set uniqueness base size equal to hash block size.
    // Just because.
    uint8_t uniqueness_base[SHA1_BLOCK_SIZE_BYTES] = {};

    // Fill first 8 bytes with base MAC address.
    ESP_LOGI(TAG, "Reading base MAC address from EFUSE BLK0.");
    esp_err_t ret = ESP_OK;
    ret = esp_efuse_mac_get_default(uniqueness_base);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to get base MAC address. (%s)", esp_err_to_name(ret));
        delayed_abort();
    }
    else
    {
        ESP_LOGI(TAG, "Base MAC Address read.");
    }

    // Fill the rest with pseudo random values.
    // We just need high degree of uniqueness here, not unpredictability, so pseudo random is ok.
    ESP_LOGI(TAG, "Generating random bytes.");
    esp_fill_random((uint8_t *)&uniqueness_base + MAC_ADDRESS_SIZE_BYTES, SHA1_BLOCK_SIZE_BYTES - MAC_ADDRESS_SIZE_BYTES);

    // Calculate SHA1 hash of the uniqueness_base.
    ESP_LOGI(TAG, "Calculating SHA1.");
    esp_sha(SHA1, (uint8_t *)&uniqueness_base, sizeof uniqueness_base, output);
}

void log_unique_id(uint8_t *unique_id)
{
    char *s = (char *)malloc(UNIQUE_ID_SIZE_BYTES * 2 + 1);
    for (size_t i = 0; i < UNIQUE_ID_SIZE_BYTES; i++)
    {
        sprintf(s + i * 2, "%02x", unique_id[i]);
    }

    ESP_LOGI(TAG, "Unique ID: %s", s);
    free(s);
}

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
    ESP_LOGI(TAG, "Opening Non-Volatile Storage (NVS) handle...");
    nvs_handle_t handle = 0;
    err = nvs_open(UNIQUE_ID_NVS_NAMESPACE, open_mode, &handle);
    switch (err)
    {
    case ESP_OK:
        ESP_LOGI(TAG, "NVS handle opened.");
        break;
    case ESP_ERR_NVS_NOT_FOUND:
        ESP_LOGI(TAG, "NVS namespace '%s' not found.", UNIQUE_ID_NVS_NAMESPACE);
        break;
    default:
        ESP_LOGE(TAG, "Error opening NVS handle! (%s)", esp_err_to_name(err));
        delayed_abort();
    }

    return handle;
}

esp_err_t load_unique_id_from_nvs(uint8_t *output)
{
    nvs_handle_t nvs_handle = nvs_open(NVS_READONLY);
    if (nvs_handle == 0)
    {
        ESP_LOGI(TAG, "Unique ID is not found in NVS.");
        return 1;
    }

    size_t read_bytes = UNIQUE_ID_SIZE_BYTES;

    ESP_LOGI(TAG, "Reading unique ID from NVS...");
    esp_err_t err = nvs_get_blob(nvs_handle, UNIQUE_ID_NVS_KEY, (void *)&global_unique_id, &read_bytes);
    switch (err)
    {
    case ESP_OK:
        ESP_LOGI(TAG, "Unique ID read from NVS.");
        if (read_bytes != UNIQUE_ID_SIZE_BYTES)
        {
            ESP_LOGE(TAG, "Wrong unique ID bytes count read - %d.", read_bytes);
            return 1;
        }
        return 0;
        break;
    case ESP_ERR_NVS_NOT_FOUND:
        ESP_LOGI(TAG, "Unique ID is not found in NVS.");
        return 1;
        break;
    default:
        ESP_LOGE(TAG, "Error NVS reading! (%s)", esp_err_to_name(err));
        delayed_abort();
    }

    nvs_close(nvs_handle);
    return 2;
}

void save_unique_id_to_nvs(uint8_t *unique_id)
{
    nvs_handle_t nvs_handle = nvs_open(NVS_READWRITE);

    esp_err_t err = nvs_set_blob(nvs_handle, UNIQUE_ID_NVS_KEY, (void *)global_unique_id, UNIQUE_ID_SIZE_BYTES);
    switch (err)
    {
    case ESP_OK:
        ESP_LOGI(TAG, "Unique ID saved to NVS.");
        break;
    default:
        ESP_LOGE(TAG, "Error saving unique ID! (%s)", esp_err_to_name(err));
        delayed_abort();
    }

    err = nvs_commit(nvs_handle);
    switch (err)
    {
    case ESP_OK:
        ESP_LOGI(TAG, "NVS commit successful.");
        break;
    default:
        ESP_LOGE(TAG, "Error commiting NVS! (%s)", esp_err_to_name(err));
        delayed_abort();
    }

    nvs_close(nvs_handle);
}

void init_unique_id()
{
    esp_err_t ret = load_unique_id_from_nvs((uint8_t *)&global_unique_id);
    if (ret != 0)
    {
        ESP_LOGI(TAG, "No unique ID found in NVS, generating new one.");
        generate_unique_id((uint8_t *)&global_unique_id);
        save_unique_id_to_nvs((uint8_t *)&global_unique_id);
    }
    else
    {
        ESP_LOGI(TAG, "Unique ID loaded from NVS.");
    }
    log_unique_id((uint8_t *)&global_unique_id);
}

uint8_t *get_unique_id()
{
    return (uint8_t *)global_unique_id;
}