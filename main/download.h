#pragma once

#include <esp_err.h>

esp_err_t http_download(char *url, char *vfs_path);