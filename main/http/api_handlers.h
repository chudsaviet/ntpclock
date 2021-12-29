#pragma once

#include <esp_err.h>
#include <esp_log.h>
#include <esp_http_server.h>

void register_api_handlers(httpd_handle_t server);