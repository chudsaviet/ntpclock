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

#define WAIT_FOR_SYNC_LOOP_DELAY_MS 1000

#ifdef NTP_SERVER
#define NTP_SERVER_FINAL NTP_SERVER
#else
#define NTP_SERVER_FINAL "pool.ntp.org"
#endif

#define NTP_SERVER_NVS_KEY "ntp_server"

static int64_t lastNTPSyncTimerTime = 0;

static SemaphoreHandle_t i2cSemaphore;
static QueueHandle_t xOutputQueue = 0;

static bool syncHappened = false;

void vWaitForNtpSync() {
    while (!syncHappened) {
        vTaskDelay(pdMS_TO_TICKS(WAIT_FOR_SYNC_LOOP_DELAY_MS));
    }
}

const char *xGetNtpServer()
{
    return sntp_getservername(0);
}

void vSetNtpServer(char *address)
{
    ESP_LOGI(TAG, "Setting NTP server <%s>.", address);

    size_t address_len = strlen(address);
    char *internal_address = (char *)malloc(address_len+1);
    memset(internal_address, 0, address_len + 1);
    memcpy(internal_address, address, address_len);

    l_nvs_set_str(NTP_SERVER_NVS_KEY, internal_address);

    sntp_setservername(0, internal_address);
}

void vNtpServerAddressInit()
{
    char buffer[NTP_SERVER_MAX_LEN_CHARS + 1];
    memset(buffer, 0, sizeof(buffer));
    size_t read_bytes = sizeof(buffer) - 1;

    if (l_nvs_get_str(NTP_SERVER_NVS_KEY, buffer, NTP_SERVER_MAX_LEN_CHARS + 1) != ESP_OK)
    {
        ESP_LOGI(TAG, "Using hardcoded NTP server <%s>.", NTP_SERVER_FINAL);
        vSetNtpServer(NTP_SERVER_FINAL);
    }
    else
    {
        ESP_LOGI(TAG, "Using NTP server <%s>.", buffer);
        vSetNtpServer(buffer);
    }
}

void sntpSyncCallback(timeval *tv)
{
    // Here we are calling RTC module bypassing any queues to minimize time dicrepancy.
    setRTC(*tv, i2cSemaphore);

    lastNTPSyncTimerTime = esp_timer_get_time();

    NtpMessage message = {};
    message.event = NtpEvent::SYNC_HAPPENED;
    xQueueSend(xOutputQueue, &message, pdMS_TO_TICKS(NTP_QUEUE_SEND_TIMEOUT_MS));

    syncHappened = true;
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
        configMINIMAL_STACK_SIZE * 4,
        NULL,
        tskIDLE_PRIORITY,
        taskHandle,
        NTP_TASK_CORE);
}