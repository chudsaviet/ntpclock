#include "wpa_key_gen.h"

#include "unique_id.h"
#include <string.h>
#include <sha/sha_parallel_engine.h>

static const char *TAG = "wpa_key_gen.cpp";

const uint8_t ap_key_salt[] = {0x81, 0xfc, 0x72, 0x37, 0x22, 0xe6, 0x6d, 0x86, 0xe9, 0xab, 0x6f, 0xe7, 0x62, 0x52,
                               0x01, 0x17, 0x32, 0xdd, 0x61, 0xec, 0x4c, 0xd0, 0x34, 0x05, 0x8f, 0x71, 0xa8, 0x99,
                               0xb0, 0x4a, 0x55, 0x61, 0x98, 0x92, 0x64, 0x1c, 0x1d, 0xb2, 0x26, 0x5b, 0x49, 0x92,
                               0x23, 0x69, 0x7f, 0xc9, 0xc7, 0x96, 0x0d, 0x7b, 0x58, 0xbe, 0x95, 0x3a, 0x89, 0x64,
                               0x6a, 0x28, 0x04, 0x48, 0xda, 0x72, 0xa1, 0x1b};

// The alphabet we can show on a 7-segment display.
const char alphabet[] = {
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
    'a', 'B', 'C', 'd', 'E', 'F'};

#define SHA1_SIZE_BYTES 20

void generate_wpa_key(char *output)
{
    uint8_t *unique_id = get_unique_id();

    uint8_t hash_base[sizeof ap_key_salt + UNIQUE_ID_SIZE_BYTES] = {0};
    memcpy(&hash_base, unique_id, UNIQUE_ID_SIZE_BYTES);
    memcpy(&hash_base, ap_key_salt, sizeof ap_key_salt);

    uint8_t hash[SHA1_SIZE_BYTES] = {0};
    esp_sha(SHA1, (uint8_t *)&hash_base, sizeof hash_base, (uint8_t *)hash);

    for (uint8_t i = 0; i < WPA_KEY_LENGTH_CHARS; i++)
    {
        output[i] = alphabet[hash[i] % (sizeof alphabet / sizeof alphabet[0])];
    }
    output[WPA_KEY_LENGTH_CHARS] = 0;
}