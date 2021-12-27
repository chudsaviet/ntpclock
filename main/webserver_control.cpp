#include "webserver_control.h"

#include <stdio.h>
#include <string.h>
#include <sys/param.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include <dirent.h>

#include "esp_err.h"
#include "esp_log.h"

#include "esp_vfs.h"
#include "esp_spiffs.h"
#include "esp_http_server.h"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>

static QueueHandle_t xCommandQueue = 0;

#define TAG "webserver_control.cpp"
#define WEBSERVER_CONTROL_LOOP_DELAY_MS 1000
#define WEBSERVER_CONTROL_TASK_CORE 1
#define WEBSERVER_QUEUE_SIZE 4

/* Our URI handler function to be called during GET /uri request */
esp_err_t get_handler(httpd_req_t *req)
{
    /* Send a simple response */
    const char resp[] = "URI GET Response";
    httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

/* Our URI handler function to be called during POST /uri request */
esp_err_t post_handler(httpd_req_t *req)
{
    /* Destination buffer for content of HTTP POST request.
     * httpd_req_recv() accepts char* only, but content could
     * as well be any binary data (needs type casting).
     * In case of string data, null termination will be absent, and
     * content length would give length of string */
    char content[100];

    /* Truncate if content length larger than the buffer */
    size_t recv_size = MIN(req->content_len, sizeof(content));

    int ret = httpd_req_recv(req, content, recv_size);
    if (ret <= 0)
    { /* 0 return value indicates connection closed */
        /* Check if timeout occurred */
        if (ret == HTTPD_SOCK_ERR_TIMEOUT)
        {
            /* In case of timeout one can choose to retry calling
             * httpd_req_recv(), but to keep it simple, here we
             * respond with an HTTP 408 (Request Timeout) error */
            httpd_resp_send_408(req);
        }
        /* In case of error, returning ESP_FAIL will
         * ensure that the underlying socket is closed */
        return ESP_FAIL;
    }

    /* Send a simple response */
    const char resp[] = "URI POST Response";
    httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

/* URI handler structure for GET /uri */
httpd_uri_t uri_get = {
    .uri = "/uri",
    .method = HTTP_GET,
    .handler = get_handler,
    .user_ctx = NULL};

/* URI handler structure for POST /uri */
httpd_uri_t uri_post = {
    .uri = "/uri",
    .method = HTTP_POST,
    .handler = post_handler,
    .user_ctx = NULL};

/* Function for starting the webserver */
httpd_handle_t start_webserver(void)
{
    /* Generate default configuration */
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    /* Empty handle to esp_http_server */
    httpd_handle_t server = NULL;

    /* Start the httpd server */
    if (httpd_start(&server, &config) == ESP_OK)
    {
        /* Register URI handlers */
        httpd_register_uri_handler(server, &uri_get);
        httpd_register_uri_handler(server, &uri_post);
    }
    /* If server failed to start, handle will be NULL */
    return server;
}

void vWebserverControlTask(void *pvParameters)
{

    for (;;)
    {
        while (uxQueueMessagesWaiting(xCommandQueue) != 0)
        {
            WebserverMessage message = {};
            xQueueReceive(xCommandQueue, &message, (TickType_t)0);
            switch (message.command)
            {
            case WebserverCommand::START:
            {
                ESP_LOGI(TAG, "Starting webserver.");
                start_webserver();
            }
            break;
            default:
                break;
            }
        }

        vTaskDelay(pdMS_TO_TICKS(WEBSERVER_CONTROL_LOOP_DELAY_MS));
    }
    vTaskDelete(NULL);
}

void vStartWebserverTask(TaskHandle_t *xTaskHandle, QueueHandle_t *xOutputQueue)
{
    xCommandQueue = xQueueCreate(WEBSERVER_QUEUE_SIZE, sizeof(WebserverMessage));
    *xOutputQueue = xCommandQueue;

    xTaskCreatePinnedToCore(
        vWebserverControlTask,
        "Webserver control",
        configMINIMAL_STACK_SIZE * 4,
        NULL,
        tskIDLE_PRIORITY,
        xTaskHandle,
        WEBSERVER_CONTROL_TASK_CORE);
}