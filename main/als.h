#include <Arduino.h>
#include <Adafruit_VEML7700.h>

#define ALS_SEMAPHORE_TIMEOUT_MS 128

void alsBegin(SemaphoreHandle_t i2cSemaphore);
float alsGetLux(SemaphoreHandle_t i2cSemaphore);