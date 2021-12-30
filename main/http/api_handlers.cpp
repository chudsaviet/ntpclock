#include "api_handlers.h"

#include <stdio.h>
#include <cJSON.h>

#include "../wifi_control.h"

#define WIFI_SSID_MAX_SIZE_CHARS 32
#define HTTP_PUT_MAX_CONTENT_SIZE_BYTES 4096

#define MIN(a, b) ((a) < (b)) ? (a) : (b)))

static const char *TAG = "http/request_handler.cpp";

static esp_err_t get_wifi_sta_ssid_handler(httpd_req_t *req)
{
    char *ssid = xGetWifiStaSsid();

    static char *format = "{\"ssid\": \"%s\"}";
    char send_buffer[WIFI_SSID_MAX_LEN_CHARS + sizeof(format) + 1] = {0};
    sprintf((char *)&send_buffer, format, ssid);

    free(ssid);

    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, (const char *)send_buffer, strlen((char *)&send_buffer));
    
    return ESP_OK;
}

static esp_err_t put_wifi_sta_ssid_handler(httpd_req_t *req)
{
    if(req->content_len > HTTP_PUT_MAX_CONTENT_SIZE_BYTES) {
        // ESP-IDF does not support HTTP error 413 Content too long.
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Content too long. Max length HTTP_PUT_MAX_CONTENT_SIZE_BYTES bytes.");
        return ESP_OK;
    }

    char content[req->content_len+1] = {0};

    int ret = httpd_req_recv(req, content, req->content_len);
    if (ret <= 0) {
        if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
            httpd_resp_send_408(req);
        }
        return ESP_FAIL;
    }
    ESP_LOGD(TAG, "put_wifi_sta_ssid_handler received:\n%s", content);

    cJSON *json = cJSON_ParseWithLength((char *)&content, req->content_len);
    cJSON *ssid = cJSON_GetObjectItemCaseSensitive(json, "ssid");

    if (cJSON_IsString(ssid) && (ssid->valuestring != NULL))
    {
        ESP_LOGI(TAG, "Setting WiFi STA ssid to '%s'.", ssid->valuestring);
        vWifiSaveStaSsid(ssid->valuestring);
    }

    cJSON_Delete(json);

    // Send empty response to complete the request.
    httpd_resp_send(req, NULL, 0);
    
    return ESP_OK;
}

static esp_err_t put_wifi_sta_password_handler(httpd_req_t *req)
{
    if(req->content_len > HTTP_PUT_MAX_CONTENT_SIZE_BYTES) {
        // ESP-IDF does not support HTTP error 413 Content too long.
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Content too long. Max length HTTP_PUT_MAX_CONTENT_SIZE_BYTES bytes.");
        return ESP_OK;
    }

    char content[req->content_len+1] = {0};

    int ret = httpd_req_recv(req, content, req->content_len);
    if (ret <= 0) {
        if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
            httpd_resp_send_408(req);
        }
        return ESP_FAIL;
    }
    ESP_LOGD(TAG, "put_wifi_sta_password_handler received:\n%s", content);

    cJSON *json = cJSON_ParseWithLength((char *)&content, req->content_len);
    cJSON *ssid = cJSON_GetObjectItemCaseSensitive(json, "password");

    if (cJSON_IsString(ssid) && (ssid->valuestring != NULL))
    {
        ESP_LOGI(TAG, "Setting WiFi STA password.");
        vWifiSaveStaPass(ssid->valuestring);
    }

    cJSON_Delete(json);

    // Send empty response to complete the request.
    httpd_resp_send(req, NULL, 0);
    
    return ESP_OK;
}

static esp_err_t restart_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "Restart request received.");
    esp_restart();
    
    return ESP_OK;
}

void register_api_handlers(httpd_handle_t server)
{
    httpd_uri_t handler_params = {0};

    ESP_LOGD(TAG, "Registering PUT restart handler.");
    handler_params = {
        .uri = "/api/restart",
        .method = HTTP_PUT,
        .handler = restart_handler,
        .user_ctx = NULL};
    httpd_register_uri_handler(server, &handler_params);

    ESP_LOGD(TAG, "Registering GET WiFi STA SSID handler.");
    handler_params = {
        .uri = "/api/wifi_sta_ssid",
        .method = HTTP_GET,
        .handler = get_wifi_sta_ssid_handler,
        .user_ctx = NULL};
    httpd_register_uri_handler(server, &handler_params);

    ESP_LOGD(TAG, "Registering PUT WiFi STA SSID handler.");
    handler_params = {
        .uri = "/api/wifi_sta_ssid",
        .method = HTTP_PUT,
        .handler = put_wifi_sta_ssid_handler,
        .user_ctx = NULL};
    httpd_register_uri_handler(server, &handler_params);

    ESP_LOGD(TAG, "Registering PUT WiFi STA password handler.");
    handler_params = {
        .uri = "/api/wifi_sta_password",
        .method = HTTP_PUT,
        .handler = put_wifi_sta_password_handler,
        .user_ctx = NULL};
    httpd_register_uri_handler(server, &handler_params);
}