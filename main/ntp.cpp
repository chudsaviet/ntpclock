#include "ntp.h"

#include <Arduino.h>
#include <time.h>
#include <esp_sntp.h>
#include <esp_log.h>

#include "rtc.h"
#include "nvs_local.h"

#include "secrets.h"

#define TAG "ntp.cpp"

#define NTP_OUTPUT_QUEUE_SIZE 4
#define NTP_QUEUE_SEND_TIMEOUT_MS 1000

#define NTP_LOOP_DELAY_MS 1000

#define NTP_SYNC_TIMEOUT_MS 3600000

#define NTP_TASK_CORE 1

#ifdef NTP_SERVER
#define NTP_SERVER_FINAL NTP_SERVER
#else
#define NTP_SERVER_FINAL "pool.ntp.org"
#endif

#define NTP_SERVER_NVS_KEY "ntp_server"

static int64_t lastNTPSyncTimerTime = 0;

static SemaphoreHandle_t i2cSemaphore;
static QueueHandle_t xOutputQueue = 0;

const char *xGetNtpServer()
{
    return sntp_getservername(0);
}

void vSetNtpServer(char *address)
{
    nvs_handle_t nvs = nvs_open(NVS_READWRITE);
    nvs_set_str(nvs, NTP_SERVER_NVS_KEY, address);
    nvs_close(nvs);

    sntp_setservername(0, address);
}

void vNtpServerAddressInit()
{
    char buffer[NTP_SERVER_MAX_LEN_CHARS + 1];
    memset(buffer, 0, sizeof(buffer));
    size_t read_bytes = sizeof(buffer) - 1;

    nvs_handle_t nvs = nvs_open(NVS_READONLY);
    if (nvs_get_str(nvs, NTP_SERVER_NVS_KEY, buffer, &read_bytes) != ESP_OK)
    {
        nvs_close(nvs);
        sntp_setservername(0, NTP_SERVER_FINAL);
    }
    else
    {
        sntp_setservername(0, buffer);
    }
    nvs_close(nvs);
}

void sntpSyncCallback(timeval *tv)
{
    // Here we are calling RTC module bypassing any queues to minimize time dicrepancy.
    setRTC(*tv, i2cSemaphore);

    lastNTPSyncTimerTime = esp_timer_get_time();

    NtpMessage message = {};
    message.event = NtpEvent::SYNC_HAPPENED;
    xQueueSend(xOutputQueue, &message, pdMS_TO_TICKS(NTP_QUEUE_SEND_TIMEOUT_MS));
}

void vNtpTask(void *pvParameters)
{
    sntp_set_sync_mode(SNTP_SYNC_MODE_IMMED);
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    vNtpServerAddressInit();
    sntp_set_time_sync_notification_cb(sntpSyncCallback);
    sntp_init();

    for (;;)
    {
        if (esp_timer_get_time() - lastNTPSyncTimerTime > NTP_SYNC_TIMEOUT_MS * 1000ll)
        {
            NtpMessage message = {};
            message.event = NtpEvent::SYNC_TIMEOUT;
            xQueueSend(xOutputQueue, &message, pdMS_TO_TICKS(NTP_QUEUE_SEND_TIMEOUT_MS));
        }

        vTaskDelay(pdMS_TO_TICKS(NTP_LOOP_DELAY_MS));
    }
    vTaskDelete(NULL);
}

void vStartNtpTask(TaskHandle_t *taskHandle, QueueHandle_t *outputQueue, SemaphoreHandle_t i2cSemaphoreParameter)
{
    i2cSemaphore = i2cSemaphoreParameter;

    xOutputQueue = xQueueCreate(NTP_OUTPUT_QUEUE_SIZE, sizeof(NtpMessage));
    *outputQueue = xOutputQueue;

    xTaskCreatePinnedToCore(
        vNtpTask,
        "NTP watch",
        configMINIMAL_STACK_SIZE * 2,
        NULL,
        tskIDLE_PRIORITY,
        taskHandle,
        NTP_TASK_CORE);
}