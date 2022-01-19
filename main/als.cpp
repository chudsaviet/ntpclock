#include "als.h"

#include <Arduino.h>
#include <Adafruit_VEML7700.h>

#define TAG "als.cpp"

#define ALS_SEMAPHORE_TIMEOUT_MS 1000

#define ALS_OUTPUT_QUEUE_SIZE 4

#define ALS_TASK_CORE 1

#define ALS_GAIN VEML7700_GAIN_1
#define ALS_INTEGRATION_TIME VEML7700_IT_800MS

#define ALS_LOOP_DELAY_MS 800

Adafruit_VEML7700 veml = Adafruit_VEML7700();

float prevLux = 0.0f;

static SemaphoreHandle_t i2cSemaphore = 0;
static QueueHandle_t xOutputQueue = 0;

void alsBegin()
{
    if (xSemaphoreTake(i2cSemaphore, pdMS_TO_TICKS(ALS_SEMAPHORE_TIMEOUT_MS)) == pdPASS)
    {
        ESP_LOGI(TAG, "ALS initialization...");
        
        veml.begin();
        veml.setGain(ALS_GAIN);
        veml.setIntegrationTime(ALS_INTEGRATION_TIME);
        
        xSemaphoreGive(i2cSemaphore);
    }
    else
    {
        ESP_LOGE(TAG, "alsBegin: couldn't take i2c semaphore. Not initializing.");
        abort();
    }
}

float alsGetLux()
{
    if (xSemaphoreTake(i2cSemaphore, pdMS_TO_TICKS(ALS_SEMAPHORE_TIMEOUT_MS)) == pdPASS)
    {
        prevLux = veml.readLux();
        xSemaphoreGive(i2cSemaphore);
        return prevLux;
    }
    else
    {
        Serial.println("alsBegin: couldn't take i2c semaphore.");
        return prevLux;
    }
}

void vAlsTask(void *pvParameters)
{
    SemaphoreHandle_t i2cSemaphore = *((SemaphoreHandle_t *)pvParameters);
    alsBegin();

    for (;;)
    {
        AlsDataMessage message = {0};
        message.lux = alsGetLux();

        if (xQueueSend(xOutputQueue, (void *)&message, 0) != pdPASS)
        {
            ESP_LOGE(TAG, "Attempt to put new ALS data to output queue failed. Clearing queue.");
            xQueueReset(xOutputQueue);
        }

        vTaskDelay(pdMS_TO_TICKS(ALS_LOOP_DELAY_MS));
    }
    vTaskDelete(NULL);
}

void vStartAlsTask(TaskHandle_t *taskHandle, QueueHandle_t *outputQueueHandle, SemaphoreHandle_t i2cSemaphoreParameter)
{
    i2cSemaphore = i2cSemaphoreParameter;

    xOutputQueue = xQueueCreate(ALS_OUTPUT_QUEUE_SIZE, sizeof(AlsDataMessage));
    *outputQueueHandle = xOutputQueue;

    xTaskCreatePinnedToCore(
        vAlsTask,
        "ALS control",
        configMINIMAL_STACK_SIZE * 4,
        NULL,
        tskIDLE_PRIORITY,
        taskHandle,
        ALS_TASK_CORE);
}