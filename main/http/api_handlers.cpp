#include "api_handlers.h"

#include <stdio.h>
#include <cJSON.h>
#include <esp_err.h>
#include <esp_log.h>

#include "../wifi_control.h"
#include "../tz_info_local.h"
#include "../ntp.h"

#define WIFI_SSID_MAX_SIZE_CHARS 32
#define HTTP_PUT_MAX_CONTENT_SIZE_BYTES 4096

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
    if (req->content_len > HTTP_PUT_MAX_CONTENT_SIZE_BYTES)
    {
        // ESP-IDF does not support HTTP error 413 Content too long.
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Content too long. Max length HTTP_PUT_MAX_CONTENT_SIZE_BYTES bytes.");
        return ESP_OK;
    }

    char content[req->content_len + 1] = {0};

    int ret = httpd_req_recv(req, content, req->content_len);
    if (ret <= 0)
    {
        if (ret == HTTPD_SOCK_ERR_TIMEOUT)
        {
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
    if (req->content_len > HTTP_PUT_MAX_CONTENT_SIZE_BYTES)
    {
        // ESP-IDF does not support HTTP error 413 Content too long.
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Content too long. Max length HTTP_PUT_MAX_CONTENT_SIZE_BYTES bytes.");
        return ESP_OK;
    }

    char content[req->content_len + 1] = {0};

    int ret = httpd_req_recv(req, content, req->content_len);
    if (ret <= 0)
    {
        if (ret == HTTPD_SOCK_ERR_TIMEOUT)
        {
            httpd_resp_send_408(req);
        }
        return ESP_FAIL;
    }
    ESP_LOGD(TAG, "put_wifi_sta_password_handler received.");

    cJSON *json = cJSON_ParseWithLength((char *)&content, req->content_len);
    cJSON *password = cJSON_GetObjectItemCaseSensitive(json, "password");

    if (cJSON_IsString(password) && (password->valuestring != NULL))
    {
        ESP_LOGI(TAG, "Setting WiFi STA password.");
        vWifiSaveStaPass(password->valuestring);
    }

    cJSON_Delete(json);

    // Send empty response to complete the request.
    httpd_resp_send(req, NULL, 0);

    return ESP_OK;
}

static char *wifi_auth_mode_to_string(wifi_auth_mode_t authmode)
{
    switch (authmode)
    {
    case WIFI_AUTH_OPEN:
        return "Open";
        break;
    case WIFI_AUTH_WEP:
        return "WEP";
        break;
    case WIFI_AUTH_WPA_PSK:
        return "WPA_PSK";
        break;
    case WIFI_AUTH_WPA2_PSK:
        return "WPA2_PSK";
        break;
    case WIFI_AUTH_WPA_WPA2_PSK:
        return "WPA_WPA2_PSK";
        break;
    case WIFI_AUTH_WPA2_ENTERPRISE:
        return "WPA2_ENTERPRISE";
        break;
    case WIFI_AUTH_WPA3_PSK:
        return "WPA3_PSK";
        break;
    case WIFI_AUTH_WPA2_WPA3_PSK:
        return "WPA2_WPA3_PSK";
        break;
    default:
        return "Unknown";
        break;
    }
}

static esp_err_t get_wifi_scan_handler(httpd_req_t *req)
{
    wifi_ap_record_t *ap_info;
    int16_t ap_count = usScanWifi(&ap_info);
    if (ap_count < 0) {
        free(ap_info);
        httpd_resp_send_500(req);
        return ESP_OK;
    }

    cJSON *response = cJSON_CreateObject();
    cJSON *ap_array = cJSON_AddArrayToObject(response, "access_points");

    for (int i = 0; i < ap_count; i++)
    {
        cJSON *ap_record = cJSON_CreateObject();
        cJSON_AddStringToObject(ap_record, "ssid", (char *)ap_info[i].ssid);
        double rssi = ap_info[i].rssi;
        cJSON_AddNumberToObject(ap_record, "rssi", rssi);
        cJSON_AddStringToObject(ap_record, "auth_mode", wifi_auth_mode_to_string(ap_info[i].authmode));
        cJSON_AddItemToArray(ap_array, ap_record);
    }
    free(ap_info);

    char *send_buffer = cJSON_Print(response);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, send_buffer);

    free(send_buffer);
    cJSON_Delete(response);

    return ESP_OK;
}

static esp_err_t restart_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "Restart request received.");
    esp_restart();

    return ESP_OK;
}

static esp_err_t get_timezone_handler(httpd_req_t *req)
{
    char *timezone = getTimezone();

    static char *format = "{\"timezone\": \"%s\"}";
    char send_buffer[TIMEZONE_MAX_LEN_CHARS + sizeof(format) + 1] = {0};
    sprintf((char *)&send_buffer, format, timezone);

    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, (const char *)send_buffer, strlen((char *)&send_buffer));

    return ESP_OK;
}

static esp_err_t put_timezone_handler(httpd_req_t *req)
{
    if (req->content_len > HTTP_PUT_MAX_CONTENT_SIZE_BYTES)
    {
        // ESP-IDF does not support HTTP error 413 Content too long.
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Content too long. Max length HTTP_PUT_MAX_CONTENT_SIZE_BYTES bytes.");
        return ESP_OK;
    }

    char content[req->content_len + 1] = {0};

    int ret = httpd_req_recv(req, content, req->content_len);
    if (ret <= 0)
    {
        if (ret == HTTPD_SOCK_ERR_TIMEOUT)
        {
            httpd_resp_send_408(req);
        }
        return ESP_FAIL;
    }
    ESP_LOGD(TAG, "put_timezone_handler received:\n%s", content);

    cJSON *json = cJSON_ParseWithLength((char *)&content, req->content_len);
    cJSON *timezone = cJSON_GetObjectItemCaseSensitive(json, "timezone");

    if (cJSON_IsString(timezone) && (timezone->valuestring != NULL))
    {
        ESP_LOGI(TAG, "Setting timezone to '%s'.", timezone->valuestring);
        setTimezone(timezone->valuestring);
    }

    cJSON_Delete(json);

    // Send empty response to complete the request.
    httpd_resp_send(req, NULL, 0);

    return ESP_OK;
}

static esp_err_t get_ntp_server_handler(httpd_req_t *req)
{
    const char *ntp_server = xGetNtpServer();

    static char *format = "{\"ntp_server\": \"%s\"}";
    char send_buffer[NTP_SERVER_MAX_LEN_CHARS + sizeof(format) + 1] = {0};
    sprintf((char *)&send_buffer, format, ntp_server);

    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, (const char *)send_buffer, strlen((char *)&send_buffer));

    return ESP_OK;
}

static esp_err_t put_ntp_server_handler(httpd_req_t *req)
{
    if (req->content_len > HTTP_PUT_MAX_CONTENT_SIZE_BYTES)
    {
        // ESP-IDF does not support HTTP error 413 Content too long.
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Content too long. Max length HTTP_PUT_MAX_CONTENT_SIZE_BYTES bytes.");
        return ESP_OK;
    }

    char content[req->content_len + 1] = {0};

    int ret = httpd_req_recv(req, content, req->content_len);
    if (ret <= 0)
    {
        if (ret == HTTPD_SOCK_ERR_TIMEOUT)
        {
            httpd_resp_send_408(req);
        }
        return ESP_FAIL;
    }
    ESP_LOGD(TAG, "put_ntp_server_handler received:\n%s", content);

    cJSON *json = cJSON_ParseWithLength((char *)&content, req->content_len);
    cJSON *ntp_server = cJSON_GetObjectItemCaseSensitive(json, "ntp_server");

    if (cJSON_IsString(ntp_server) && (ntp_server->valuestring != NULL))
    {
        ESP_LOGI(TAG, "Setting NTP server to '%s'.", ntp_server->valuestring);
        vSetNtpServer(ntp_server->valuestring);
    }

    cJSON_Delete(json);

    // Send empty response to complete the request.
    httpd_resp_send(req, NULL, 0);

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

    ESP_LOGD(TAG, "Registering GET WiFi Scan handler.");
    handler_params = {
        .uri = "/api/wifi_scan",
        .method = HTTP_GET,
        .handler = get_wifi_scan_handler,
        .user_ctx = NULL};
    httpd_register_uri_handler(server, &handler_params);

    ESP_LOGD(TAG, "Registering GET timezone handler.");
    handler_params = {
        .uri = "/api/timezone",
        .method = HTTP_GET,
        .handler = get_timezone_handler,
        .user_ctx = NULL};
    httpd_register_uri_handler(server, &handler_params);

    ESP_LOGD(TAG, "Registering PUT timezone handler.");
    handler_params = {
        .uri = "/api/timezone",
        .method = HTTP_PUT,
        .handler = put_timezone_handler,
        .user_ctx = NULL};
    httpd_register_uri_handler(server, &handler_params);

    ESP_LOGD(TAG, "Registering GET NTP server handler.");
    handler_params = {
        .uri = "/api/ntp_server",
        .method = HTTP_GET,
        .handler = get_ntp_server_handler,
        .user_ctx = NULL};
    httpd_register_uri_handler(server, &handler_params);

    ESP_LOGD(TAG, "Registering PUT NTP server handler.");
    handler_params = {
        .uri = "/api/ntp_server",
        .method = HTTP_PUT,
        .handler = put_ntp_server_handler,
        .user_ctx = NULL};
    httpd_register_uri_handler(server, &handler_params);
}