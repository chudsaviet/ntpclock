#include "display.h"

#define DISPLAY_ADDRESS 0x70

Adafruit_7segment clockDisplay = Adafruit_7segment();

void displayBegin(SemaphoreHandle_t i2cSemaphore)
{
    if (xSemaphoreTake(i2cSemaphore, pdMS_TO_TICKS(DISPLAY_BEGIN_SEMAPHORE_TIMEOUT_MS)) == pdPASS)
    {
        clockDisplay.begin(DISPLAY_ADDRESS);
        xSemaphoreGive(i2cSemaphore);
    }
    else
    {
        Serial.println("displayBegin: couldn't take i2c semaphore.");
    }
}

void display(time_t time, bool flickColons, uint8_t brightness)
{
    clockDisplay.setBrightness(brightness);
    clockDisplay.print(hour(time) * 100 + minute(time), DEC);
    if (flickColons)
    {
        clockDisplay.drawColon(second(time) % 2 == 0);
    }
    else
    {
        clockDisplay.drawColon(true);
    }
    clockDisplay.writeDisplay();
}

void displayUpdate(uint8_t currentBrightness, bool flickColons, SemaphoreHandle_t i2cSemaphore)
{
    timeval currentTime;
    gettimeofday(&currentTime, NULL);
    currentTime.tv_sec = utc2local(currentTime.tv_sec);

    if (xSemaphoreTake(i2cSemaphore, pdMS_TO_TICKS(DISPLAY_UPDATE_SEMAPHORE_TIMEOUT_MS)) == pdPASS)
    {
        display(currentTime.tv_sec, flickColons, currentBrightness);
        xSemaphoreGive(i2cSemaphore);
    }
    else
    {
        Serial.println("displayUpdate: couldn't take i2c semaphore.");
    }
}