#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>

#include <ai64/bbai64_clocks.h>

static char bfr[20+1];
static char *uint64ToDecimal(uint64_t v)
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

// Print time in ticks and seconds
double dtime_print(void){
    double q;
    uint64_t t;
    t = get_current_ticks();
    q = (double) t / (double) GTC_HZ;
    printf("BASED ticks=%s dtime=%lf\n", uint64ToDecimal(t), q);
    return q;
}

// Get seconds since clock reset (likely since board boot)
double dtime(void){
    double q;
    uint64_t t;
    t = get_current_ticks();
    q = (double) t / (double) GTC_HZ;
    return q;
}

