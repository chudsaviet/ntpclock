#include "rtc.h"

#include <Arduino.h>
#include <RTClib.h>

#include <esp_log.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>

#include "tz_info_local.h"

#define TAG "rtc.cpp"

#define RTC_BEGIN_SEMAPHORE_TIMEOUT_MS 2048L
#define RTC_ACCESS_SEMAPHORE_TIMEOUT_MS 128L
#define RTC_QUEUE_SIZE 4
#define RTC_TASK_CORE 1
#define RTC_LOOP_DELAY_MS 1000
#define RTC_SYSTEM_CLOCK_SYNC_INTERVAL_MS 3600000

RTC_DS3231 rtc;

static SemaphoreHandle_t i2cSemaphore = 0;
static QueueHandle_t xCommandQueue = 0;
static bool xAutoSyncSystemTime = true;
static int64_t lastSytemClockSync = 0;

bool beginRTC()
{
    if (xSemaphoreTake(i2cSemaphore, pdMS_TO_TICKS(RTC_BEGIN_SEMAPHORE_TIMEOUT_MS)) == pdPASS)
    {
        bool result = rtc.begin();
        xSemaphoreGive(i2cSemaphore);

        return result;
    }
    else
    {
        Serial.println("beginRTC: couldn't take i2c semaphore.");
        return false;
    }
}

timeval getRTC()
{
    if (xSemaphoreTake(i2cSemaphore, pdMS_TO_TICKS(RTC_ACCESS_SEMAPHORE_TIMEOUT_MS)) == pdPASS)
    {
        DateTime rtcTime = rtc.now();
        xSemaphoreGive(i2cSemaphore);
        return {static_cast<time_t>(rtcTime.unixtime()), 0};
    }
    else
    {
        Serial.println("getRTC: couldn't take i2c semaphore.");
        timeval osTime;
        gettimeofday(&osTime, NULL);
        return osTime;
    }
}

void systemClockSync()
{
    ESP_LOGI(TAG, "System clock sync from RTC.");
    timeval rtcTime = getRTC();
    settimeofday(&rtcTime, NULL);
}

// Using i2cSemaphoreParameter istead of static i2cSemaphore because it is call from outside.
void setRTC(timeval time, SemaphoreHandle_t i2cSemaphoreParameter)
{
    if (xSemaphoreTake(i2cSemaphore, pdMS_TO_TICKS(RTC_ACCESS_SEMAPHORE_TIMEOUT_MS)) == pdPASS)
    {
        Serial.print("Setting RTC to ");
        Serial.println(time.tv_sec);
        rtc.adjust(DateTime(time.tv_sec));
        xSemaphoreGive(i2cSemaphore);
    }
    else
    {
        Serial.println("setRTC: couldn't take i2c semaphore.");
    }
}

void vRtcTask(void *pvParameters)
{
    beginRTC();

    for (;;)
    {
        while (uxQueueMessagesWaiting(xCommandQueue) != 0)
        {
            RtcMessage message = {};
            xQueueReceive(xCommandQueue, &message, (TickType_t)0);
            switch (message.command)
            {
            case RtcCommand::SET_RTC_AUTO_SYSTEM_CLOCK_SYNC:
            {
                xAutoSyncSystemTime = (message.payload[0] > 0);
            }
            break;
            case RtcCommand::DO_SYSTEM_CLOCK_SYNC:
            {
                systemClockSync();
                lastSytemClockSync = esp_timer_get_time();
            }
            break;
            default:
                break;
            }
        }

        if (esp_timer_get_time() - lastSytemClockSync > RTC_SYSTEM_CLOCK_SYNC_INTERVAL_MS * (int64_t)1000)
        {
            systemClockSync();
            lastSytemClockSync = esp_timer_get_time();
        }

        vTaskDelay(pdMS_TO_TICKS(RTC_LOOP_DELAY_MS));
    }
    vTaskDelete(NULL);
}

void vStartRtcTask(TaskHandle_t *taskHandle, QueueHandle_t *outputQueue, SemaphoreHandle_t i2cSemaphoreParameter)
{
    i2cSemaphore = i2cSemaphoreParameter;

    xCommandQueue = xQueueCreate(RTC_QUEUE_SIZE, sizeof(RtcMessage));
    *outputQueue = xCommandQueue;

    xTaskCreatePinnedToCore(
        vRtcTask,
        "RTC control",
        configMINIMAL_STACK_SIZE * 4,
        NULL,
        tskIDLE_PRIORITY,
        taskHandle,
        RTC_TASK_CORE);
}