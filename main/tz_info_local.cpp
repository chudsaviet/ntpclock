#include "tz_info_local.h"

#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <esp_log.h>
#include <freertos/task.h>
#include <freertos/semphr.h>

#include "abort.h"
#include "download.h"
#include "nvs_local.h"
#include "ntp.h"
#include "secrets.h"

#define TAG "tz_info_local.cpp"

#define TIMEZONE_NVS_KEY "timezone"
#define TIMEZONE_LEN_MAX_CHARS 64
#define TZ_DATA_URL_BASE "http://tzdata.dranik.xyz/"
#define TZ_DATA_VFS_PATH "/spiffs/tzdata/timezone"
#define TZ_DATA_UPDATE_DATE_NVS_KEY "tzd_last_upd"
#define TZ_DATA_TASK_CORE 1
#define TZ_DATA_LOOP_DELAY_MS 1000        // 1 second
#define TZDATA_UPDATE_TIMEOUT_SEC 2505600 // 29 days
#define TZ_LIST_URL "http://tzdata.dranik.xyz/timezones.json"
#define TZ_LIST_VFS_PATH "/spiffs/w/timezones.json"
#define TZ_LIST_UPDATE_DATE_NVS_KEY "tzl_last_upd"
#define TZ_LIST_UPDATE_TIMEOUT_SEC 15552000 // Approximately 6 months

#define READ_CHUNK_BYTES 256
#define SEMAPHORE_WAIT_MS 10000

static byte *tzData = NULL;

static TimeZoneInfo tzInfoLib;

static SemaphoreHandle_t tzDataSemaphore = 0;

static char *timezone = DEFAULT_TIMEZONE;

static int64_t lastTzdataUpdateTime = 0;
static int64_t lastTzListUpdateTime = 0;

void lastTimestampsReadFromNVS()
{
    if (l_nvs_get_i64(TZ_DATA_UPDATE_DATE_NVS_KEY, &lastTzdataUpdateTime) != ESP_OK)
    {
        ESP_LOGE(TAG, "%s not found in NVS.", TZ_DATA_UPDATE_DATE_NVS_KEY);
        lastTzdataUpdateTime = 0;
    } else {
        ESP_LOGI(TAG, "TZ data last download timestamp in GMT: %llu", lastTzdataUpdateTime);
    }

    if (l_nvs_get_i64(TZ_LIST_UPDATE_DATE_NVS_KEY, &lastTzListUpdateTime) != ESP_OK)
    {
        ESP_LOGE(TAG, "%s not found in NVS.", TZ_LIST_UPDATE_DATE_NVS_KEY);
        lastTzListUpdateTime = 0;
    } else {
        ESP_LOGI(TAG, "TZ list last download timestamp in GMT: %llu", lastTzListUpdateTime);
    }
}

void setTimezoneGlobalVar(char *value) {
    // Keep timezone in heap and write it down to NVS.
    ESP_LOGI(TAG, "Setting timezone to <%s>.", value);

    size_t value_len = strlen(value);
    timezone = (char *)malloc(value_len + 1);
    memset(timezone, 0, value_len + 1);
    memcpy(timezone, value, value_len); 
}

void setTimezone(char *value)
{
    setTimezoneGlobalVar(value);

    l_nvs_set_str(TIMEZONE_NVS_KEY, timezone);

    // Zero lastTzdataUpdateTime to force TZ data file update on next occasion.
    l_nvs_set_i64(TZ_DATA_UPDATE_DATE_NVS_KEY, 0);
}

char *getTimezone()
{
    return timezone;
}

void loadTimezoneFromNVS()
{
    char read_buffer[TIMEZONE_LEN_MAX_CHARS + 1] = {0};
    size_t read_bytes = 0;

    if (l_nvs_get_str(TIMEZONE_NVS_KEY, read_buffer, TIMEZONE_LEN_MAX_CHARS + 1) != ESP_OK)
    {
        ESP_LOGE(TAG, "Timezone record not found in NVS. Setting default to <%s>.", DEFAULT_TIMEZONE);
        setTimezone(DEFAULT_TIMEZONE);
    }
    else
    {
        setTimezoneGlobalVar(read_buffer);
    }
}

void loadTzFile()
{
    ESP_LOGI(TAG, "Reading TZ data file <%s>.", TZ_DATA_VFS_PATH);

    struct stat file_stat;
    if (stat(TZ_DATA_VFS_PATH, &file_stat) != 0)
    {
        ESP_LOGE(TAG, "Failed to stat tz data file.");
        delayed_abort();
    }

    FILE *file = fopen(TZ_DATA_VFS_PATH, "r");
    if (file == NULL)
    {
        ESP_LOGE(TAG, "Failed to open tz data file.");
        delayed_abort();
    }

    tzData = (byte *)malloc(file_stat.st_size);
    byte *chunk = tzData;
    size_t chunksize;
    do
    {
        chunksize = fread(chunk, 1, READ_CHUNK_BYTES, file);
        chunk += chunksize;
    } while (chunksize != 0);

    ESP_LOGI(TAG, "Read %ld TZ data file bytes.", file_stat.st_size);
}

void tzdataReloadFromSpiffs() {
    ESP_LOGI(TAG, "Reloading TZ data from NVS.");

    if (xSemaphoreTake(tzDataSemaphore, pdMS_TO_TICKS(SEMAPHORE_WAIT_MS)) != pdTRUE)
    {
        ESP_LOGE(TAG, "utc2local: Failed to take TZ data semaphore.");
        delayed_abort();
    };

    loadTzFile();
    tzInfoLib.setLocation_P(tzData);

    xSemaphoreGive(tzDataSemaphore);
}

void tzBegin()
{
    ESP_LOGI(TAG, "Initializing TZ subsystem.");
    tzDataSemaphore = xSemaphoreCreateBinary();
    xSemaphoreGive(tzDataSemaphore);

    lastTimestampsReadFromNVS();

    tzdataReloadFromSpiffs();
}

int32_t utc2local(int32_t utc)
{
    if (xSemaphoreTake(tzDataSemaphore, pdMS_TO_TICKS(SEMAPHORE_WAIT_MS)) != pdTRUE)
    {
        ESP_LOGE(TAG, "utc2local: Failed to take TZ data semaphore.");
        delayed_abort();
    };
    int32_t local = tzInfoLib.utc2local(utc);
    xSemaphoreGive(tzDataSemaphore);
    return local;
}

void tzDataUpdate()
{
    size_t timezone_len = strlen(timezone);
    char tz_data_url[sizeof(TZ_DATA_URL_BASE) + timezone_len + 1];
    memset(tz_data_url, 0, sizeof(tz_data_url));
    memcpy(tz_data_url, TZ_DATA_URL_BASE, sizeof(TZ_DATA_URL_BASE));
    memcpy(tz_data_url + sizeof(TZ_DATA_URL_BASE) - 1, timezone, timezone_len);

    if (http_download(tz_data_url, TZ_DATA_VFS_PATH) == ESP_OK)
    {
        timeval currentTime;
        gettimeofday(&currentTime, NULL);
        lastTzdataUpdateTime = currentTime.tv_sec;

        
        l_nvs_set_i64(TZ_DATA_UPDATE_DATE_NVS_KEY, lastTzdataUpdateTime);

        tzdataReloadFromSpiffs();
    }
    else
    {
        ESP_LOGE(TAG, "TZ data file download failed.");
    }
}

void tzListUpdate()
{
    if (http_download(TZ_LIST_URL, TZ_LIST_VFS_PATH) == ESP_OK)
    {
        timeval currentTime;
        gettimeofday(&currentTime, NULL);
        lastTzListUpdateTime = currentTime.tv_sec;
        l_nvs_set_i64(TZ_LIST_UPDATE_DATE_NVS_KEY, lastTzListUpdateTime);
    }
    else
    {
        ESP_LOGE(TAG, "TZ list file download failed.");
    }
}

void vTzdataTask(void *pvParameters)
{
    ESP_LOGI(TAG, "Waiting for NTP sync...");
    vWaitForNtpSync();

    for (;;)
    {
        timeval currentTime;
        gettimeofday(&currentTime, NULL);

        if (currentTime.tv_sec - lastTzdataUpdateTime > TZDATA_UPDATE_TIMEOUT_SEC)
        {
            ESP_LOGI(TAG, "Updating TZ data.");
            tzDataUpdate();
        }

        if (currentTime.tv_sec - lastTzListUpdateTime > TZ_LIST_UPDATE_TIMEOUT_SEC)
        {
            ESP_LOGI(TAG, "Updating TZ list.");
            tzListUpdate();
        }

        vTaskDelay(pdMS_TO_TICKS(TZ_DATA_LOOP_DELAY_MS));
    }
    vTaskDelete(NULL);
}

void vStartTzdataTask(TaskHandle_t *taskHandle)
{
    loadTimezoneFromNVS();
    ESP_LOGI(TAG, "Timezone is <%s>.", timezone);

    tzBegin();

    xTaskCreatePinnedToCore(
        vTzdataTask,
        "Timezone control",
        configMINIMAL_STACK_SIZE * 4,
        NULL,
        tskIDLE_PRIORITY,
        taskHandle,
        TZ_DATA_TASK_CORE);
}