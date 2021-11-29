#pragma once

#include "display.h"
#include "als.h"
#include "main.h"

#define DISPLAY_LOOP_DELAY_MS 128L
#define MAX_BRIGHTNESS 16L
#define MIN_BRIGNTNESS 0L
#define MAX_LUX 500.0F

void vDisplayTask(void *pvParameters);