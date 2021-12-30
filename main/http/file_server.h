#pragma once

#include <esp_http_server.h>

esp_err_t file_server_init();
void register_file_server_handlers(httpd_handle_t server);