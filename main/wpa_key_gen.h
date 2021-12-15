#pragma once

// Shall be shorter that SHA1 hash length (20 bytes).
// If we will need longer - will use longer hash.
#define WPA_KEY_LENGTH_CHARS 16

void generate_wpa_key(char *output);