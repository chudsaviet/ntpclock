#include "tz_info_local.h"

#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <esp_log.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include "nvs.h"

#include "abort.h"
#include "download.h"
#include "nvs_local.h"
#include "secrets.h"

#define TAG "tz_info_local.cpp"

#define TIMEZONE_NVS_KEY "timezone"
#define TIMEZONE_LEN_MAX_CHARS 64
#define TZ_DATA_URL_BASE "http://tzdata.dranik.xyz/"
#define TZ_DATA_VFS_PATH "/spiffs/tzdata/timezone"
#define TZ_DATA_UPDATE_DATE_NVS_KEY "tz_data_last_update"
#define TZ_DATA_TASK_CORE 1
#define TZ_DATA_LOOP_DELAY_MS 86400000    // 1 day
#define TZDATA_UPDATE_TIMEOUT_SEC 2505600 // 29 days
#define TZ_LIST_URL "http://tzdata.dranik.xyz/timezones.json"
#define TZ_LIST_VFS_PATH "/spiffs/w/timezones.json"
#define TZ_LIST_UPDATE_DATE_NVS_KEY "tz_data_last_update"
#define TZ_LIST_UPDATE_TIMEOUT_SEC 15552000 // Approximately 6 months

#define READ_CHUNK_BYTES 256
#define SEMAPHORE_WAIT_MS 10000

static byte *tzData = NULL;

static TimeZoneInfo tzInfoLib;

static SemaphoreHandle_t tzDataSemaphore = 0;

static char *timezone = DEFAULT_TIMEZONE;

static int64_t lastTzdataUpdateTime = 0;
static int64_t lastTzListUpdateTime = 0;

void setTimezone(char *value) {
    size_t value_len = strlen(value);
    if (timezone != NULL) {
        free(timezone);
    }
    timezone = (char *)malloc(value_len+1);
    memset(timezone, 0, value_len+1);
    memcpy(timezone, value, value_len);

    nvs_handle_t nvs = nvs_open(NVS_READWRITE);
    nvs_set_blob(nvs, TIMEZONE_NVS_KEY, timezone, value_len+1);
    nvs_commit(nvs);
}

char *getTimezone() {
    return timezone;
}

void loadTimezoneFromNVS() {
    char read_buffer[TIMEZONE_LEN_MAX_CHARS+1] = {0};
    size_t read_bytes = 0;
    nvs_handle_t nvs = nvs_open(NVS_READONLY);
    if (nvs_get_blob(nvs, TIMEZONE_NVS_KEY, read_buffer, &read_bytes) != ESP_OK) {
        ESP_LOGE(TAG, "Timezone record not found in NVS. Setting default to <%s>.", DEFAULT_TIMEZONE);
        nvs_close(nvs);
        setTimezone(DEFAULT_TIMEZONE);
    } else {
        nvs_close(nvs);
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

    if (xSemaphoreTake(tzDataSemaphore, pdMS_TO_TICKS(SEMAPHORE_WAIT_MS)) != pdTRUE)
    {
        ESP_LOGE(TAG, "Failed to take TX data semaphore.");
        delayed_abort();
    };
    if (tzData != NULL) {
        free(tzData);
    }
    tzData = (byte *)malloc(file_stat.st_size);
    byte *chunk = tzData;
    size_t chunksize;
    do
    {
        chunksize = fread(chunk, 1, READ_CHUNK_BYTES, file);
        chunk += chunksize;
    } while (chunksize != 0);
    xSemaphoreGive(tzDataSemaphore);

    ESP_LOGI(TAG, "Read %ld TZ data file bytes.", file_stat.st_size);
}

void tzBegin()
{
    ESP_LOGI(TAG, "Initializing TZ subsystem.");

    tzDataSemaphore = xSemaphoreCreateBinary();
    xSemaphoreGive(tzDataSemaphore);

    loadTzFile();
    tzInfoLib.setLocation_P(tzData);
}

int32_t utc2local(int32_t utc)
{
    if (xSemaphoreTake(tzDataSemaphore, pdMS_TO_TICKS(SEMAPHORE_WAIT_MS)) != pdTRUE)
    {
        ESP_LOGE(TAG, "Failed to take TX data semaphore.");
        delayed_abort();
    };
    return tzInfoLib.utc2local(utc);
    xSemaphoreGive(tzDataSemaphore);
}

void lastTimestampsReadFromNVS()
{
    nvs_handle_t nvs = nvs_open(NVS_READONLY);
    if (nvs_get_i64(nvs, TZ_DATA_UPDATE_DATE_NVS_KEY, &lastTzdataUpdateTime) != ESP_OK)
    {
        ESP_LOGE(TAG, "%s not found in NVS.", TZ_DATA_UPDATE_DATE_NVS_KEY);
    }
    if (nvs_get_i64(nvs, TZ_LIST_UPDATE_DATE_NVS_KEY, &lastTzListUpdateTime) != ESP_OK)
    {
        ESP_LOGE(TAG, "%s not found in NVS.", TZ_LIST_UPDATE_DATE_NVS_KEY);
    }
    nvs_close(nvs);
}

void tzDataUpdate()
{
    size_t timezone_len = strlen(timezone);
    char tz_data_url[sizeof(TZ_DATA_URL_BASE) + timezone_len];
    memcpy(tz_data_url, TZ_DATA_URL_BASE, sizeof(TZ_DATA_URL_BASE));
    memcpy(tz_data_url + sizeof(TZ_DATA_URL_BASE), timezone, timezone_len);

    if (http_download(tz_data_url, TZ_DATA_VFS_PATH) == ESP_OK)
    {
        timeval currentTime;
        gettimeofday(&currentTime, NULL);
        lastTzdataUpdateTime = currentTime.tv_sec;
        nvs_handle_t nvs = nvs_open(NVS_READWRITE);
        nvs_set_i64(nvs, TZ_DATA_UPDATE_DATE_NVS_KEY, lastTzdataUpdateTime);
        nvs_commit(nvs);
        nvs_close(nvs);

        loadTzFile();
    } else {
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
        nvs_handle_t nvs = nvs_open(NVS_READWRITE);
        nvs_set_i64(nvs, TZ_LIST_UPDATE_DATE_NVS_KEY, lastTzListUpdateTime);
        nvs_commit(nvs);
        nvs_close(nvs);
    } else {
        ESP_LOGE(TAG, "TZ list file download failed.");
    }
}

void vTzdataTask(void *pvParameters)
{
    loadTimezoneFromNVS();
    lastTimestampsReadFromNVS();
    tzBegin();

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
    xTaskCreatePinnedToCore(
        vTzdataTask,
        "NTP watch",
        configMINIMAL_STACK_SIZE * 2,
        NULL,
        tskIDLE_PRIORITY,
        taskHandle,
        TZ_DATA_TASK_CORE);
}