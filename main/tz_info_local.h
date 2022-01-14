#pragma once

#include <TimeZoneInfo.h>

int32_t utc2local(int32_t utc);

void setTimezone(char *value);
char *getTimezone();

void vStartTzdataTask(TaskHandle_t *taskHandle);