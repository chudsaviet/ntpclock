#pragma once

#include <TimeZoneInfo.h>

#define TIMEZONE_MAX_LEN_CHARS 128

int32_t utc2local(int32_t utc);

void setTimezone(char *value);
char *getTimezone();

void vStartTzdataTask(TaskHandle_t *taskHandle);