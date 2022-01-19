#include "display.h"

#define TAG "display.cpp"

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
#define DISPLAY_CHARS 4

enum DisplayShowMode
{
    TIME,
    TEXT
};

static Adafruit_7segment clockDisplay = Adafruit_7segment();
static bool blinkColons = false;
static SemaphoreHandle_t i2cSemaphore = 0;
static QueueHandle_t xCommandQueue = 0;
static float currentBrightness = 0.0;
static DisplayShowMode currentMode = DisplayShowMode::TIME;
static char *currentText = NULL;
static size_t currentTextLength = 0;
static int64_t textShowStartTimestamp = 0;

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

void vDisplayTime()
{
    timeval currentTime;
    gettimeofday(&currentTime, NULL);
    currentTime.tv_sec = utc2local(currentTime.tv_sec);

    if (xSemaphoreTake(i2cSemaphore, pdMS_TO_TICKS(DISPLAY_UPDATE_SEMAPHORE_TIMEOUT_MS)) == pdPASS)
    {
        clockDisplay.clear();
        clockDisplay.setBrightness(currentBrightness);

        int currentHour = hour(currentTime.tv_sec);
        int currentMinute = minute(currentTime.tv_sec);

        uint8_t hourFirstDigit = currentHour / 10;
        uint8_t hourSecondDigit = currentHour % 10;
        uint8_t minuteFirstDigit = currentMinute / 10;
        uint8_t minuteSecondDigit = currentMinute % 10;

        if (hourFirstDigit > 0)
        {
            clockDisplay.writeDigitNum(0, hourFirstDigit);
        }
        clockDisplay.writeDigitNum(1, hourSecondDigit);
        clockDisplay.writeDigitNum(3, minuteFirstDigit);
        clockDisplay.writeDigitNum(4, minuteSecondDigit);
        
        if (blinkColons)
        {
            clockDisplay.drawColon(second(currentTime.tv_sec) % 2 == 0);
        }
        else
        {
            clockDisplay.drawColon(true);
        }
        clockDisplay.writeDisplay();

        xSemaphoreGive(i2cSemaphore);
    }
    else
    {
        Serial.println("displayUpdate: couldn't take i2c semaphore.");
    }
}

void vDisplayText()
{
    char finalText[DISPLAY_CHARS] = {0};

    if (currentTextLength <= DISPLAY_CHARS)
    {

        strncpy((char *)&finalText, currentText, currentTextLength);
    }
    else
    {
        uint8_t textShift = ((esp_timer_get_time() - textShowStartTimestamp) / 1000000) % currentTextLength;
        size_t tailLen = currentTextLength - textShift;
        ESP_LOGV(TAG, "vDisplayText() textShift: %d tailLen: %d", textShift, tailLen);
        if (tailLen < DISPLAY_CHARS)
        {
            memcpy(&finalText, currentText + textShift, tailLen);
            memcpy((char *)&finalText + tailLen, currentText, DISPLAY_CHARS - tailLen);
        }
        else
        {
            memcpy(&finalText, currentText + textShift, DISPLAY_CHARS);
        }
    }

    if (xSemaphoreTake(i2cSemaphore, pdMS_TO_TICKS(DISPLAY_UPDATE_SEMAPHORE_TIMEOUT_MS)) == pdPASS)
    {
        clockDisplay.println();
        clockDisplay.drawColon(false);
        clockDisplay.setBrightness(currentBrightness);
        for (uint8_t i = 0; i < DISPLAY_CHARS; i++)
        {
            clockDisplay.write(finalText[i]);
        }

        clockDisplay.writeDisplay();
        xSemaphoreGive(i2cSemaphore);
    }
    else
    {
        Serial.println("vDisplayText(): couldn't take i2c semaphore.");
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
            case DisplayCommand::SET_SHOW_TIME:
            {
                currentMode = DisplayShowMode::TIME;
            }
            break;
            case DisplayCommand::SET_SHOW_TEXT:
            {
                currentMode = DisplayShowMode::TEXT;
                currentTextLength = strlen((char *)&message.payload);
                currentText = (char *)malloc(currentTextLength + 2);
                strncpy(currentText, (char *)&message.payload, currentTextLength + 1);
                currentText[currentTextLength] = ' ';
                currentText[currentTextLength + 1] = 0;
                currentTextLength += 1;
                textShowStartTimestamp = esp_timer_get_time();
                ESP_LOGI(TAG, "Will show: '%s'", currentText);
            }
            break;
            default:
                ESP_LOGE(TAG, "Unknown DisplayCommand received: %d", (uint8_t)message.command);
                abort();
                break;
            }
        }

        switch (currentMode)
        {
        case DisplayShowMode::TIME:
            vDisplayTime();
            break;
        case DisplayShowMode::TEXT:
            vDisplayText();
            break;
        default:
            ESP_LOGE(TAG, "Unknown DisplayShowMode received: %d", (uint8_t)currentMode);
            abort();
            break;
        }

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