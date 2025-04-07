#ifndef BBAI64_CLOCKS_H
#define BBAI64_CLOCKS_H
#include <stdint.h>

#define GTC_HZ  200000000
#define GTC_CYCLES_PER_MICROSECOND (GTC_HZ/1000000)

// ti/csl/src/ip/gtc/V0/cslr_gtc.h holds a bunch of definitions for the GTC registers.
// defined in ti/csl/soc/j721e/src/cslr_soc_baseaddress.h as CSL_GTC0_GTC_CFG1_BASE
#define GTC0_GTC_CFG1_BASE 0x00A90000

double dtime_print(void);
double dtime(void);

static inline uint64_t get_current_ticks(void) {
    uint32_t GTC_CNTCV_LO, GTC_CNTCV_HI, GTC_CNTCV_HI2;
    do {
        GTC_CNTCV_HI = *((uint32_t volatile *)(0x00A9000C));
        GTC_CNTCV_LO = *((uint32_t volatile *)(0x00A90008));
        GTC_CNTCV_HI2 = *((uint32_t volatile *)(0x00A9000C));
        // Catch 32bit overflow
    } while (GTC_CNTCV_HI != GTC_CNTCV_HI2);

    return ((uint64_t)GTC_CNTCV_HI << 32) | GTC_CNTCV_LO;
}

static inline uint64_t dmicroseconds_to_ticks(uint64_t q_us){
    return q_us * GTC_CYCLES_PER_MICROSECOND;
}

static inline double get_gtc_as_dseconds(void){
    double q_s;
    uint64_t t;
    t = get_current_ticks();
    q_s = (double) t / (double) GTC_HZ;
    return q_s;
}

static inline double get_gtc_as_u64seconds(void){
    return (uint64_t)get_gtc_as_dseconds();
}

static inline double get_gtc_as_dmicroseconds(void){
    double q_us;
    uint64_t t;
    t = get_current_ticks();
    q_us = (double) t / (double) GTC_CYCLES_PER_MICROSECOND;
    return q_us;
}

static inline uint64_t get_gtc_as_microseconds(void){
    return (uint64_t) get_gtc_as_dmicroseconds();
}

// This function has been tested from 1 microsecond to 1ms to have about a 1 +- 0.5 microsecond 
// latency. At a 1 second delay, there is a 0.25ms +- 0.01ms latency.
//
// This function has been placed in this .h file so that it will be inline compiled. If this
// function is not inline compiled, and the -O3 optimization option is not used, it will have
// greater latency.
static inline void delay_mirco_seconds(uint64_t us)
{
	uint64_t delay_ticks, start_ticks, end_ticks;

    // Convert from microseconds to GTC ticks
	delay_ticks =  ((double)us / (double)1000000) * GTC_HZ;
	start_ticks = get_current_ticks();
	end_ticks = start_ticks + delay_ticks;
	while (get_current_ticks() < end_ticks);
}

static inline void delay_integer_math(uint64_t us) {
    // Convert the delay from microseconds to ticks
    // (us * GTC_HZ) / 1000000 gives the number of ticks for the desired delay
    uint64_t delay_ticks = (us * GTC_HZ) / 1000000;
    uint64_t start_ticks = get_current_ticks();
    uint64_t end_ticks = start_ticks + delay_ticks;
    while (get_current_ticks() < end_ticks);
}

static inline void delay_ticks(uint64_t delay_ticks) {
    // Get the start tick count
    uint64_t start_ticks = get_current_ticks();
    uint64_t end_ticks = start_ticks + delay_ticks;
    while (get_current_ticks() < end_ticks);
}


#endif // BBAI64_CLOCKS_H

// Kevin Cahalan
