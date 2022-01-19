#pragma once

#include <stdint.h>
#include <esp_err.h>

void nvs_begin();

void l_nvs_set_str(const char* key, const char* value);
void l_nvs_set_i64 (const char* key, int64_t value);
void l_nvs_set_array(const char* key, const void* value, size_t length);

esp_err_t l_nvs_get_str(const char* key, char* out_value, size_t length);
esp_err_t l_nvs_get_i64(const char* key, int64_t* out_value);
esp_err_t l_nvs_get_array(const char* key, void* out_value, size_t length);
