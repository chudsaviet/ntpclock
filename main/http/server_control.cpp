#include "server_control.h"

#include <stdio.h>
#include <string.h>
#include <sys/param.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include <dirent.h>

#include "esp_err.h"
#include "esp_log.h"
#include "esp_http_server.h"

#include "api_handlers.h"
#include "file_server.h"

#include "abort.h"

#define HTTP_SERVER_CONTROL_LOOP_DELAY_MS 1000
#define HTTP_SERVER_CONTROL_TASK_CORE 1
#define HTTP_SERVER_TASK_CORE 1
#define HTTP_SERVER_CONTROL_QUEUE_SIZE 4
#define HTTP_SERVER_MAX_OPEN_SOCKETS 13
#define HTTP_SERVER_MAX_RESP_HEADERS 16
#define HTTP_SERVER_MAX_URI_HANDLERS 16
#define HTTP_SERVER_BACKLOG_CONN 10

static QueueHandle_t xCommandQueue = 0;

static const char *TAG = "http/server_control.cpp";

void start_http_server()
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.core_id = HTTP_SERVER_TASK_CORE;
    config.max_open_sockets = HTTP_SERVER_MAX_OPEN_SOCKETS;
    config.max_resp_headers = HTTP_SERVER_MAX_RESP_HEADERS;
    config.max_uri_handlers = HTTP_SERVER_MAX_URI_HANDLERS;
    config.backlog_conn = HTTP_SERVER_BACKLOG_CONN;
    config.uri_match_fn = httpd_uri_match_wildcard;

    ESP_LOGI(TAG, "Starting HTTP Server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to start HTTP server!");
        delayed_abort();
    }

    register_api_handlers(server);
    register_file_server_handlers(server);
}

void vHttpServerControlTask(void *pvParameters)
{
    for (;;)
    {
        while (uxQueueMessagesWaiting(xCommandQueue) != 0)
        {
            HttpServerMessage message = {};
            xQueueReceive(xCommandQueue, &message, (TickType_t)0);
            switch (message.command)
            {
            case HttpServerCommand::START:
            {
                ESP_LOGI(TAG, "Starting HTTP server.");
                start_http_server();
            }
            break;
            default:
                break;
            }
        }

        vTaskDelay(pdMS_TO_TICKS(HTTP_SERVER_CONTROL_LOOP_DELAY_MS));
    }
    vTaskDelete(NULL);
}

void vStartHttpServerControlTask(TaskHandle_t *xTaskHandle, QueueHandle_t *xOutputQueue)
{
    xCommandQueue = xQueueCreate(HTTP_SERVER_CONTROL_QUEUE_SIZE, sizeof(HttpServerMessage));
    *xOutputQueue = xCommandQueue;

    xTaskCreatePinnedToCore(
        vHttpServerControlTask,
        "HTTP server control",
        configMINIMAL_STACK_SIZE * 4,
        NULL,
        tskIDLE_PRIORITY,
        xTaskHandle,
        HTTP_SERVER_CONTROL_TASK_CORE);
}