#pragma once

#include <esp_system.h>

#define UNIQUE_ID_SIZE_BYTES 20
#define SHA1_BLOCK_SIZE_BYTES 64
#define MAC_ADDRESS_SIZE_BYTES 8

#define UNIQUE_ID_NVS_KEY "unique_id"

void init_unique_id();

uint8_t *get_unique_id();