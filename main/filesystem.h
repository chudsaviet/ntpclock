#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "esp_err.h"
#include "esp_log.h"
#include "esp_spiffs.h"

void filesystem_init();