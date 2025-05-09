#include <stdint.h>
#include <stdio.h>
#include <math.h>
#include <stdbool.h>

// to shut up prototype warnings
#include "../include/random_utils.h"

static char bfr[20+1];
char *uint64ToDecimal(uint64_t v)
{
        char* p = bfr + sizeof(bfr);
        *(--p) = '\0';
        for (bool first = true; v || first; first = false)
        {
                const uint32_t digit = v % 10;
                const char c = '0' + digit;
                *(--p) = c;
                v = v / 10;
        }
        return p;
}


#ifdef R5
// R5
#include <ti/osal/osal.h>
#include <ai64/bbai64_clocks.h>

// Waste a pseudo random amount of time between min and max ms
void burn_time_pretending_to_do_stuff(uint32_t min_ms, uint32_t max_ms) {
    if (max_ms <= min_ms) {
        Osal_delay(min_ms);
        return;
    }

    // 1) Grab timer in µs, fold into 32 bits
    uint64_t t64 = get_gtc_as_microseconds();
    uint32_t x  = (uint32_t)(t64 ^ (t64 >> 32));

    // 2) Simple xorshift to mix bits
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;

    // 3) Scale into [0, span), then shift up to [min_ms, max_ms]
    uint32_t span  = max_ms - min_ms + 1;
    uint32_t offs  = x % span;
    Osal_delay(offs + min_ms);
}

#else
// Linux

#include <time.h>
#include <stdlib.h>
#include <unistd.h>
void burn_time_pretending_to_do_stuff(uint32_t min_ms, uint32_t max_ms) {
    // 1) Sanity check
    if (max_ms <= min_ms) {
        struct timespec ts = {
            .tv_sec  = min_ms / 1000,
            .tv_nsec = (min_ms % 1000) * 1000000U
        };
        nanosleep(&ts, NULL);
        return;
    }

    // 2) One‑time seed
    static int seeded = 0;
    if (!seeded) {
        srandom((unsigned)time(NULL) ^ (unsigned)getpid());
        seeded = 1;
    }

    // 3) Compute span and pick a random offset
    uint32_t span     = max_ms - min_ms + 1;
    uint32_t offs     = (uint32_t)random() % span;  // modulo‐bias is negligible for small spans
    uint32_t delay_ms = min_ms + offs;

    // 4) Sleep
    struct timespec req = {
        .tv_sec  = delay_ms / 1000,
        .tv_nsec = (delay_ms % 1000) * 1000000U
    };
    nanosleep(&req, NULL);
}
#endif

void printDataSizes()
{
    // Integer types
    printf("Size of char: %lu bytes\n", (unsigned long)sizeof(char));
    printf("Size of short: %lu bytes\n", (unsigned long)sizeof(short));
    printf("Size of int: %lu bytes\n", (unsigned long)sizeof(int));
    printf("Size of long: %lu bytes\n", (unsigned long)sizeof(long));
    printf("Size of long long: %lu bytes\n", (unsigned long)sizeof(long long));

    // Unsigned integer types
    printf("Size of unsigned char: %lu bytes\n", (unsigned long)sizeof(unsigned char));
    printf("Size of unsigned short: %lu bytes\n", (unsigned long)sizeof(unsigned short));
    printf("Size of unsigned int: %lu bytes\n", (unsigned long)sizeof(unsigned int));
    printf("Size of unsigned long: %lu bytes\n", (unsigned long)sizeof(unsigned long));
    printf("Size of unsigned long long: %lu bytes\n", (unsigned long)sizeof(unsigned long long));

    // Floating-point types
    printf("Size of float: %lu bytes\n", (unsigned long)sizeof(float));
    printf("Size of double: %lu bytes\n", (unsigned long)sizeof(double));
    printf("Size of long double: %lu bytes\n", (unsigned long)sizeof(long double));

    // Other types
    printf("Size of void*: %lu bytes\n", (unsigned long)sizeof(void*));
}