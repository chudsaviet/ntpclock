#include "ntp.h"

#define ATTEMPT_DELAY_MS 3803
#define ATTEMPTS 3

#ifdef NTP_SERVER
#define NTP_SERVER_FINAL NTP_SERVER
#else
#define NTP_SERVER_FINAL "pool.ntp.org"
#endif
