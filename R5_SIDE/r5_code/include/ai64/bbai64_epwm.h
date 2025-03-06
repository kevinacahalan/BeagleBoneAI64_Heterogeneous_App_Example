#ifndef BBAI64_EPWM_H
#define BBAI64_EPWM_H

#include <stdint.h>
#include <stdbool.h>

// figured out by running `sudo k3conf dump clocks 87` on board
#define EPWM_MODULE_CLOCK_RATE 125000000
// Bigger clock divider allows lower frequencies at the cost of signal accuracy at higher
// frequencies. With a Clk_DIV of 1 stuff would start acting weird with a period around 
// 1ms or greater.
//
// A clock divider of 16 or greater seems to engage some special automatic clk divider
// calculating logic in CSL_epwmTbTimebaseClkCfg(). This logic breaks our code!
//
// DEFAULT_CLK_DIV = 1: max period is around 1ms (0.5ms with counter mode CSL_EPWM_TB_COUNTER_DIR_UP)
// DEFAULT_CLK_DIV = 4: max period is around 4ms (2ms with counter mode CSL_EPWM_TB_COUNTER_DIR_UP)
// DEFAULT_CLK_DIV = 8: max period is around 8ms (4ms with counter mode CSL_EPWM_TB_COUNTER_DIR_UP)
// DEFAULT_CLK_DIV = 16: STUFF BREAKS, duty cycle comes out correct, frequency will be wrong
#define DEFAULT_CLK_DIV 8

enum epwm_ch
{
    A,
    B,
};

// The pins exposed on the BBAI64
enum epwm_pin
{
    P9_14_EHRPWM2_A,
    P9_16_EHRPWM2_B,
    P9_21b_EHRPWM1_A,
    P9_22b_EHRPWM1_B,
    P9_25b_EHRPWM4_B,
    P8_13_EHRPWM0_B,
    P8_19_EHRPWM0_A,
    P8_37a_EHRPWM5_A,
};

struct epwm_ch_info
{
    enum epwm_ch channel;
    uint32_t base_addr;
};

/* Helper functions */
void enable_epwm4_clock();
void disable_epwm4_clock();
// struct epwm_ch_info epwm_pin_to_ch_info(enum epwm_pin pin);


/* PWM API*/
bool pwm_init_verbose(enum epwm_pin pin, uint32_t frequency, double duty_cycle, uint32_t clkDiv);
bool pwm_set_frequency_and_duty_cycle_verbose(enum epwm_pin pin, uint32_t frequency, double duty_cycle, uint32_t clkDiv);

// simple api
bool pwm_init(enum epwm_pin pin);
bool pwm_set_frequency_and_duty_cycle(enum epwm_pin pin, uint32_t frequency, double duty_cycle);
bool pwm_stop(enum epwm_pin pin);
bool pwm_deinit(enum epwm_pin pin);

#endif