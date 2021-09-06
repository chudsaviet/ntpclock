#include "als.h"

#define ALS_GAIN VEML7700_GAIN_1
#define ALS_INTEGRATION_TIME VEML7700_IT_800MS

Adafruit_VEML7700 veml = Adafruit_VEML7700();

void alsBegin()
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
}

float alsGetLux()
{
    return veml.readLux();
}