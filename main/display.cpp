#include "display.h"

#include <Arduino.h>
#include <TimeLib.h>

// Can't simply include `time.h` because of conflict with Arduino's `time.h`.
// `esp_sntp.h` seems to include the right one.
#include <esp_sntp.h>

// Otherwise `Adafruit LEDBackpack` -> `Adafruit GFX` fails to build.
#include <Adafruit_I2CDevice.h>
#include <Adafruit_LEDBackpack.h>

#include "tz_info_local.h"

#define DISPLAY_TASK_CORE 1
#define DISPLAY_COMMAND_QUEUE_SIZE 8

#define DISPLAY_I2C_ADDRESS 0x70
#define DISPLAY_BEGIN_SEMAPHORE_TIMEOUT_MS 2048
#define DISPLAY_UPDATE_SEMAPHORE_TIMEOUT_MS 512

#define DISPLAY_LOOP_DELAY_MS 128L
#define DISPLAY_MAX_BRIGHTNESS 16L
#define DISPLAY_MIN_BRIGNTNESS 0L
#define DISPLAY_MAX_LUX 500.0F

Adafruit_7segment clockDisplay = Adafruit_7segment();

static bool blinkColons = false;
static SemaphoreHandle_t i2cSemaphore = 0;
static QueueHandle_t xCommandQueue = 0;
static float currentBrightness = 0.0;

void displayBegin()
{
    if (xSemaphoreTake(i2cSemaphore, pdMS_TO_TICKS(DISPLAY_BEGIN_SEMAPHORE_TIMEOUT_MS)) == pdPASS)
    {
        clockDisplay.begin(DISPLAY_I2C_ADDRESS);
        xSemaphoreGive(i2cSemaphore);
    }
    else
    {
        Serial.println("displayBegin: couldn't take i2c semaphore.");
    }
}

void display(time_t time)
{
    clockDisplay.setBrightness(currentBrightness);
    clockDisplay.print(hour(time) * 100 + minute(time), DEC);
    if (blinkColons)
    {
        clockDisplay.drawColon(second(time) % 2 == 0);
    }
    else
    {
        clockDisplay.drawColon(true);
    }
    clockDisplay.writeDisplay();
}

void displayUpdate()
{
    timeval currentTime;
    gettimeofday(&currentTime, NULL);
    currentTime.tv_sec = utc2local(currentTime.tv_sec);

    if (xSemaphoreTake(i2cSemaphore, pdMS_TO_TICKS(DISPLAY_UPDATE_SEMAPHORE_TIMEOUT_MS)) == pdPASS)
    {
        display(currentTime.tv_sec);
        xSemaphoreGive(i2cSemaphore);
    }
    else
    {
        Serial.println("displayUpdate: couldn't take i2c semaphore.");
    }
}

void setBrightness(float lux)
{
    float k = lux / DISPLAY_MAX_LUX;
    currentBrightness = min(DISPLAY_MAX_BRIGHTNESS,
                            DISPLAY_MIN_BRIGNTNESS +
                                static_cast<uint8_t>(
                                    static_cast<float>(DISPLAY_MAX_BRIGHTNESS - DISPLAY_MIN_BRIGNTNESS) * k));
}

void vDisplayTask(void *pvParameters)
{
    SemaphoreHandle_t i2cSemaphore = *((SemaphoreHandle_t *)pvParameters);
    displayBegin();

    for (;;)
    {
        while (uxQueueMessagesWaiting(xCommandQueue) != 0)
        {
            DisplayCommandMessage message = {};
            xQueueReceive(xCommandQueue, &message, (TickType_t)0);
            switch (message.command)
            {
            case DisplayCommand::SET_BRIGHTNESS:
            {
                float lux = 0;
                memcpy(&lux, &message.payload, sizeof(float));
                setBrightness(lux);
            }
            break;
            case DisplayCommand::SET_BLINK_COLONS:
            {
                blinkColons = (message.payload[0] > 0);
            }
            break;
            default:
                break;
            }
        }

        displayUpdate();

        vTaskDelay(pdMS_TO_TICKS(DISPLAY_LOOP_DELAY_MS));
    }
    vTaskDelete(NULL);
}

void vStartDisplayTask(TaskHandle_t *task_handle, QueueHandle_t *queue_handle, SemaphoreHandle_t i2cSemaphoreParameter)
{
    i2cSemaphore = i2cSemaphoreParameter;

    xCommandQueue = xQueueCreate(DISPLAY_COMMAND_QUEUE_SIZE, sizeof(DisplayCommandMessage));
    *queue_handle = xCommandQueue;

    xTaskCreatePinnedToCore(
        vDisplayTask,
        "Display control",
        configMINIMAL_STACK_SIZE * 4,
        NULL,
        tskIDLE_PRIORITY,
        task_handle,
        DISPLAY_TASK_CORE);
}