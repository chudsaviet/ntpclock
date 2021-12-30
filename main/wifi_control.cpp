#include "wifi_control.h"

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_err.h"
#include "nvs.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"

#include "secrets.h"

#include "unique_id.h"
#include "nvs_local.h"
#include "abort.h"

/*
    Based on 
    https://github.com/espressif/esp-idf/blob/master/examples/wifi/getting_started/station/main/station_example_main.c
*/

static EventGroupHandle_t s_wifi_event_group = NULL;

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1
#define WIFI_CONTROL_TASK_LOOP_DELAY_MS 4000
#define WIFI_RESTART_TIMEOUT_SEC 900 // 15 min
#define WIFI_SHORTTERM_MAX_RETRY_COUNT 7
#define WIFI_COMMAND_QUEUE_SIZE 4
#define WIFI_CONTROL_TASK_CORE 1
#define WIFI_AP_CHANNEL 6
#define WIFI_AP_MAX_CONNECTIONS 4
#define WIFI_AP_AUTH_MODE WIFI_AUTH_WPA2_PSK
#define WPA_PASSWORD_MAX_LENGTH_CHARS 64
#define WIFI_STA_SSID_NVS_KEY "wifi_sta_ssid"
#define WIFI_STA_PASS_NVS_KEY "wifi_sta_pass"

#define HOSTNAME_TEMPLATE "ntpclock-XXXXXXXX"
#define HOSTNAME_SUFFIX_LENGTH_CHARS 8

static const char *TAG = "wifi_control.cpp";
static char hostname[sizeof HOSTNAME_TEMPLATE + 1] = {0};

static char wpaPassword[WPA_PASSWORD_MAX_LENGTH_CHARS] = {0};
static size_t wpaPasswordLength = 0;

static int s_retry_num = 0;
static timeval s_wifi_fail_timestamp = {0, 0};
static volatile bool s_wifi_failed = false;
static esp_netif_t *s_sta_netif = NULL;
static esp_netif_t *s_ap_netif = NULL;
static esp_event_handler_instance_t s_instance_any_id = NULL;
static esp_event_handler_instance_t s_instance_got_ip = NULL;

static QueueHandle_t xCommandQueue = 0;

void vWifiSaveStaSsid(char *ssid) {
    ESP_LOGI(TAG, "Saving WiFi STA SSID to NVS.");

    nvs_handle_t nvs_handle = nvs_open(NVS_READWRITE);

    esp_err_t err = nvs_set_str(nvs_handle, WIFI_STA_SSID_NVS_KEY, ssid);
    switch (err)
    {
    case ESP_OK:
        ESP_LOGD(TAG, "WiFi STA SSID saved to NVS.");
        break;
    default:
        ESP_LOGE(TAG, "Error saving WiFi STA SSID! (%s)", esp_err_to_name(err));
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

void vWifiSaveStaPass(char *pass) {
    ESP_LOGI(TAG, "Saving WiFi STA password to NVS.");
    nvs_handle_t nvs_handle = nvs_open(NVS_READWRITE);

    esp_err_t err = nvs_set_str(nvs_handle, WIFI_STA_PASS_NVS_KEY, pass);
    switch (err)
    {
    case ESP_OK:
        ESP_LOGD(TAG, "WiFi STA password saved to NVS.");
        break;
    default:
        ESP_LOGE(TAG, "Error saving WiFi STA password! (%s)", esp_err_to_name(err));
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

void vPrepareStaSsidAndPass(wifi_config_t *wifi_config) {
    nvs_handle_t nvs_handle = nvs_open(NVS_READONLY);
    if (nvs_handle == 0)
    {
        ESP_LOGI(TAG, "Cannot opend NVS.");
        abort();
    }

    size_t ssid_size = sizeof(wifi_config->sta.ssid);
    esp_err_t ssid_read_result = nvs_get_str(nvs_handle, WIFI_STA_SSID_NVS_KEY, (char *)&wifi_config->sta.ssid, &ssid_size);
    size_t pass_size = sizeof(wifi_config->sta.password);
    esp_err_t pass_read_result = nvs_get_str(nvs_handle, WIFI_STA_PASS_NVS_KEY, (char *)&wifi_config->sta.password, &pass_size);

    if (ssid_read_result != ESP_OK || pass_read_result != ESP_OK) {
        strcpy((char *)wifi_config->sta.ssid, WIFI_SSID);
        strcpy((char *)wifi_config->sta.password, WIFI_PASS);
        
        nvs_close(nvs_handle);

        ESP_LOGI(TAG, "WiFi STA SSID and/or password not found in NVS. Loading default values.");

        vWifiSaveStaSsid(WIFI_SSID);
        vWifiSaveStaPass(WIFI_PASS);
    } else {
        ESP_LOGI(TAG, "WiFi STA SSID and password loaded from NVS.");
    }
}

static void event_handler(void *arg, esp_event_base_t event_base,
                          int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        if (s_retry_num < WIFI_SHORTTERM_MAX_RETRY_COUNT)
        {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "Retry WiFi connection. Attempt: `%d`.", s_retry_num);
        }
        else
        {
            gettimeofday(&s_wifi_fail_timestamp, NULL);
            s_wifi_failed = true;
            s_retry_num = 0;
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
            ESP_LOGI(TAG, "WiFi connection attempts failed.");
        }
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "Got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

void wifi_bits_wait()
{
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
                                           WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                           pdFALSE,
                                           pdFALSE,
                                           portMAX_DELAY);

    if (bits & WIFI_CONNECTED_BIT)
    {
        ESP_LOGI(TAG, "Connected to ap SSID: `%s`.", WIFI_SSID);
    }
    else if (bits & WIFI_FAIL_BIT)
    {
        ESP_LOGI(TAG, "Failed to connect to SSID: `%s`.", WIFI_SSID);
    }
    else
    {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
    }
}

void init_hostname()
{
    uint8_t *unique_id = get_unique_id();

    memcpy((void *)&hostname, &HOSTNAME_TEMPLATE, sizeof HOSTNAME_TEMPLATE);
    for (uint8_t i = 0; i < HOSTNAME_SUFFIX_LENGTH_CHARS / 2; i++)
    {
        // Each id byte will be converted to two characters in hex.
        sprintf(
            (char *)&hostname + sizeof HOSTNAME_TEMPLATE - HOSTNAME_SUFFIX_LENGTH_CHARS + i * 2 - 1,
            "%02X",
            unique_id[i]);
    }
}

void wifi_start(void)
{
    s_sta_netif = esp_netif_create_default_wifi_sta();

    esp_netif_set_hostname(s_sta_netif, (char *)&hostname);

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        NULL,
                                                        &s_instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &event_handler,
                                                        NULL,
                                                        &s_instance_got_ip));

    wifi_config_t wifi_config = {};
    
    strcpy((char *)wifi_config.sta.ssid, WIFI_SSID);
    strcpy((char *)wifi_config.sta.password, WIFI_PASS);
    wifi_config.sta.threshold.authmode = WIFI_AUTH_MODE;
    wifi_config.sta.pmf_cfg.capable = true;
    wifi_config.sta.pmf_cfg.required = WIFI_PMF_REQUIRED;
    vPrepareStaSsidAndPass(&wifi_config);

    ESP_LOGI(TAG, "Connecting to '%s'", (char *)&wifi_config.sta.ssid);

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_MIN_MODEM));
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_wifi_start());

    ESP_LOGI(TAG, "esp_wifi_start() finished.");

    wifi_bits_wait();
}

void wifi_restart()
{
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_wifi_disconnect());
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_wifi_stop());
    ESP_ERROR_CHECK(esp_wifi_start());
    wifi_bits_wait();
}

void vSwitchToApMode()
{
    // Stop STA mode.
    ESP_ERROR_CHECK(esp_wifi_stop());
    esp_netif_destroy(s_sta_netif);

    s_ap_netif = esp_netif_create_default_wifi_ap();

    // SSID will be equal to the hostname
    wifi_config_t wifi_config = {0};
    wifi_config.ap.channel = WIFI_AP_CHANNEL;
    wifi_config.ap.max_connection = WIFI_AP_MAX_CONNECTIONS;
    wifi_config.ap.authmode = WIFI_AP_AUTH_MODE;
    wifi_config.ap.ssid_len = sizeof hostname;
    memcpy(&wifi_config.ap.ssid, &hostname, sizeof hostname);
    memcpy(&wifi_config.ap.password, &wpaPassword, wpaPasswordLength);

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "Switching to WiFi AP mode finished. SSID: '%s', channel: %d.",
             (char *)&wifi_config.ap.ssid, wifi_config.ap.channel);
}

void vWifiControlTask(void *pvParameters)
{
    init_hostname();
    ESP_LOGI(TAG, "Generated hostname: %s", hostname);

    ESP_LOGI(TAG, "Starting WiFi.");
    ESP_ERROR_CHECK(esp_netif_init());

    s_wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    wifi_start();

    for (;;)
    {

        while (uxQueueMessagesWaiting(xCommandQueue) != 0)
        {
            WifiCommandMessage message = {};
            xQueueReceive(xCommandQueue, &message, (TickType_t)0);
            switch (message.command)
            {
            case WifiCommand::SWITCH_TO_AP_MODE:
            {
                wpaPasswordLength = strlen((char *)&message.payload);
                memcpy(&wpaPassword, &message.payload, wpaPasswordLength + 1);

                ESP_LOGI(TAG, "Switching to WiFi AP mode.");
                vSwitchToApMode();
            }
            break;
            default:
                ESP_LOGE(TAG, "Unknown DisplayCommand received: %d", (uint8_t)message.command);
                abort();
                break;
            }
        }

        timeval current_timestamp = {0, 0};
        gettimeofday(&current_timestamp, NULL);

        if (s_wifi_failed &&
            current_timestamp.tv_sec - s_wifi_fail_timestamp.tv_sec >= WIFI_RESTART_TIMEOUT_SEC)
        {
            ESP_LOGI(TAG, "WiFi hard reset timeout.");
            s_wifi_failed = false;
            wifi_restart();
        };

        vTaskDelay(pdMS_TO_TICKS(WIFI_CONTROL_TASK_LOOP_DELAY_MS));
    }
    vTaskDelete(NULL);
}

void vStartWifiTask(TaskHandle_t *taskHandle, QueueHandle_t *queueHandle)
{
    xCommandQueue = xQueueCreate(WIFI_COMMAND_QUEUE_SIZE, sizeof(WifiCommandMessage));
    *queueHandle = xCommandQueue;

    xTaskCreatePinnedToCore(
        vWifiControlTask,
        "WiFi control",
        configMINIMAL_STACK_SIZE * 4,
        NULL,
        tskIDLE_PRIORITY,
        taskHandle,
        WIFI_CONTROL_TASK_CORE);
}

char *xGetWifiStaSsid()
{
    wifi_config_t wifi_config = {0};
    ESP_ERROR_CHECK(esp_wifi_get_config(WIFI_IF_STA, &wifi_config));

    size_t ssid_len = strlen((char *)&wifi_config.sta.ssid);
    char *ssid = (char *)malloc(ssid_len + 1);
    strcpy(ssid, (char *)&wifi_config.sta.ssid);

    return ssid;
}
