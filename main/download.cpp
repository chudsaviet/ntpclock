#include "download.h"

#include <esp_err.h>
#include <esp_log.h>
#include <esp_http_client.h>

#define TAG "download.cpp"

#define HTTP_TIMEOUT_MS 5000
#define HTTP_MAX_AUTHORIZATION_RETRIES 5
#define HTTP_MAX_OUTPUT_BUFFER_BYTES 4096
#define DNS_ATTEMPTS 3
#define DNS_ATTEMPT_DELAY_MS 1000

struct download_user_data_t
{
    char *vfs_path;
    FILE *file_handle;
};

esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
    static char *output_buffer; // Buffer to store response of http request from event handler
    static int output_len;      // Stores number of bytes read
    switch (evt->event_id)
    {
    case HTTP_EVENT_ERROR:
        ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
        if (((download_user_data_t *)evt->user_data)->file_handle != NULL)
        {
            fclose(((download_user_data_t *)evt->user_data)->file_handle);
        }
        break;
    case HTTP_EVENT_ON_CONNECTED:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
        ((download_user_data_t *)evt->user_data)->file_handle =
            fopen(((download_user_data_t *)evt->user_data)->vfs_path, "w");
        break;
    case HTTP_EVENT_HEADER_SENT:
        ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
        break;
    case HTTP_EVENT_ON_HEADER:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
        break;
    case HTTP_EVENT_ON_DATA:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
        fwrite(evt->data, 1, evt->data_len,
               ((download_user_data_t *)evt->user_data)->file_handle);
        break;
    case HTTP_EVENT_ON_FINISH:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
        if (((download_user_data_t *)evt->user_data)->file_handle != NULL)
        {
            fclose(((download_user_data_t *)evt->user_data)->file_handle);
        }
        break;
    case HTTP_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED");
        if (((download_user_data_t *)evt->user_data)->file_handle != NULL)
        {
            fclose(((download_user_data_t *)evt->user_data)->file_handle);
        }
        break;
    }
    return ESP_OK;
}

esp_err_t http_download(char *url, char *vfs_path)
{
    ESP_LOGI(TAG, "Attempting to download fron <%s> to <%s>.", url, vfs_path);

    download_user_data_t user_data;
    memset(&user_data, 0, sizeof(user_data));
    user_data.vfs_path = vfs_path;

    esp_http_client_config_t config;
    memset(&config, 0, sizeof(config));
    config.url = url;
    config.method = HTTP_METHOD_GET;
    config.timeout_ms = HTTP_TIMEOUT_MS;
    config.disable_auto_redirect = false;
    config.max_authorization_retries = HTTP_MAX_AUTHORIZATION_RETRIES;
    config.event_handler = _http_event_handler;
    config.user_data = &user_data;
    config.keep_alive_enable = false;

    esp_http_client_handle_t client = esp_http_client_init(&config);

    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK)
    {
        ESP_LOGI(TAG, "HTTP GET Status = %d, content_length = %d",
                 esp_http_client_get_status_code(client),
                 esp_http_client_get_content_length(client));
    }
    else
    {
        ESP_LOGE(TAG, "HTTP GET request failed: %s", esp_err_to_name(err));
        esp_http_client_cleanup(client);
        return ESP_FAIL;
    }

    esp_http_client_cleanup(client);
    return ESP_OK;
}