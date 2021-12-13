#include "abort.h"

#define TAG "abort.cpp"

void delayed_abort()
{
    ESP_LOGI(TAG, "Aborting.");
    vTaskDelay(pdMS_TO_TICKS(DELAYED_ABORT_DELAY_SEC * 1000));
    abort();
}