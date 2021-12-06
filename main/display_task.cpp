#include "display_task.h"

void vDisplayTask(void *pvParameters)
{
    SemaphoreHandle_t i2cSemaphore = *((SemaphoreHandle_t *)pvParameters);
    alsBegin(i2cSemaphore);
    displayBegin(i2cSemaphore);

    for (;;)
    {
        float lux = alsGetLux(i2cSemaphore);

        float k = lux / MAX_LUX;
        float currentBrightness = min(MAX_BRIGHTNESS,
                                      MIN_BRIGNTNESS +
                                          static_cast<uint8_t>(
                                              static_cast<float>(MAX_BRIGHTNESS - MIN_BRIGNTNESS) * k));
        displayUpdate(currentBrightness, getLastSyncSource() == SyncSource::ntp, i2cSemaphore);
        vTaskDelay(pdMS_TO_TICKS(DISPLAY_LOOP_DELAY_MS));
    }
    vTaskDelete(NULL);
}