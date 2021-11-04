#include "als.h"

#define ALS_GAIN VEML7700_GAIN_1
#define ALS_INTEGRATION_TIME VEML7700_IT_800MS

Adafruit_VEML7700 veml = Adafruit_VEML7700();

float prevLux = 0.0f;

void alsBegin(SemaphoreHandle_t i2cSemaphore)
{
    if (xSemaphoreTake(i2cSemaphore, pdMS_TO_TICKS(ALS_SEMAPHORE_TIMEOUT_MS)) == pdPASS)
    {
        Serial.println("ALS initialization...");
        if (!veml.begin())
        {
            veml.setGain(ALS_GAIN);
            veml.setIntegrationTime(ALS_INTEGRATION_TIME);
            Serial.println("ALS intialized.");
        }
        else
        {
            Serial.println("ALS not found.");
        }
        xSemaphoreGive(i2cSemaphore);
    }
    else
    {
        Serial.println("alsBegin: couldn't take i2c semaphore. Not initializing.");
    }
}

float alsGetLux(SemaphoreHandle_t i2cSemaphore)
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