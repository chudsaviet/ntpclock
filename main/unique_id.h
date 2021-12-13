#pragma once

#include <esp_log.h>
#include <esp_system.h>
#include <nvs.h>
#include <nvs_flash.h>
#include <sha/sha_parallel_engine.h>

#include "abort.h"

#define UNIQUE_ID_SIZE_BYTES 20
#define SHA1_BLOCK_SIZE_BYTES 64
#define MAC_ADDRESS_SIZE_BYTES 8

#define UNIQUE_ID_NVS_KEY "unique_id"
#define UNIQUE_ID_NVS_NAMESPACE "device_settings"

void init_unique_id();

uint8_t *get_unique_id();