#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <esp_system.h>

#define DELAYED_ABORT_DELAY_SEC 16

void delayed_abort();