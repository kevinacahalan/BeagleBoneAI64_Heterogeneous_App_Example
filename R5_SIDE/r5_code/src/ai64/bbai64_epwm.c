// This file will hold functions to spit out EPWM on the BeagleBoneAI64
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <ti/csl/csl_epwm.h>
#include <ti/csl/soc.h>

#include <ai64/sys_pwmss.h>
#include <ai64/sys_ctrlmmr.h>

#include <ai64/bbai64_epwm.h>

/* Helper functions */
static struct epwm_ch_info epwm_pin_to_ch_info(enum epwm_pin pin)
{
    struct epwm_ch_info info;
    switch (pin)
    {
    case P9_14_EHRPWM2_A:
        info.channel = A;
        info.base_addr = CSL_EHRPWM2_EPWM_BASE;
        break;
    case P9_16_EHRPWM2_B:
        info.channel = B;
        info.base_addr = CSL_EHRPWM2_EPWM_BASE;
        break;
    case P9_21b_EHRPWM1_A:
        info.channel = A;
        info.base_addr = CSL_EHRPWM1_EPWM_BASE;
        break;
    case P9_22b_EHRPWM1_B:
        info.channel = B;
        info.base_addr = CSL_EHRPWM1_EPWM_BASE;
        break;
    case P9_25b_EHRPWM4_B:
        info.channel = B;
        info.base_addr = CSL_EHRPWM4_EPWM_BASE;
        break;
    case P8_13_EHRPWM0_B:
        info.channel = B;
        info.base_addr = CSL_EHRPWM0_EPWM_BASE;
        break;
    case P8_19_EHRPWM0_A:
        info.channel = A;
        info.base_addr = CSL_EHRPWM0_EPWM_BASE;
        break;
    case P8_37a_EHRPWM5_A:
        info.channel = A;
        info.base_addr = CSL_EHRPWM5_EPWM_BASE;
        break;
    default:
        printf("\nUnknown EPWM PIN!\n");
        break;
    }

    return info;
}

/* Lower level API */

// This would be best done by device tree overlay
void enable_epwm4_clock()
{
    EHRPWM_CTRLMMR.EHRPWM4_bit.TB_CLKEN = 1;
}

void disable_epwm4_clock()
{
    EHRPWM_CTRLMMR.EHRPWM4_bit.TB_CLKEN = 0;
}

// Set the duty cycle
// epwm_pin: Value from "enum epwm_pin"
// frequency: Desired frequency in Hz
// duty_cycle: Desired duty cycle as a percentage (0.0 to 100.0)

// (WARNING) DO NOT TOTALLY REMOVE THIS CODE YET, IT HAS A NICE FEATURE OF NOT MESSING UP PULSE SEPARATION WITH POWER CHANGE..
// The down side with this code is that sometimes you'll get an extra pulse of the last duty cycle setting
// At the time of writing this comment this function can also go to lower frequencies than the new revision.
/*
bool pwm_set_frequency_and_duty_cycle_verbose(enum epwm_pin pin, uint32_t frequency, double duty_cycle, uint32_t clkDiv)
{
    struct epwm_ch_info ch_info = epwm_pin_to_ch_info(pin);

    uint32_t tbClk = EPWM_MODULE_CLOCK_RATE / clkDiv;
    uint32_t tbPeriod = (tbClk / frequency) / 2U;
    uint32_t cmpValue = (tbPeriod - ((duty_cycle * tbPeriod) / 100U));

    // Configure Timebase
    CSL_epwmTbTimebaseClkCfg(ch_info.base_addr, tbClk, EPWM_MODULE_CLOCK_RATE);
    CSL_epwmTbPwmFreqCfg(ch_info.base_addr, tbClk, frequency, CSL_EPWM_TB_COUNTER_DIR_UP_DOWN, CSL_EPWM_SHADOW_REG_CTRL_ENABLE);

    // If these are not configured the same weird stuff happens
    CSL_epwmCounterComparatorCfg(ch_info.base_addr, CSL_EPWM_CC_CMP_A,
                                 cmpValue, CSL_EPWM_SHADOW_REG_CTRL_ENABLE,
                                 CSL_EPWM_CC_CMP_LOAD_MODE_CNT_EQ_ZERO, TRUE);
    CSL_epwmCounterComparatorCfg(ch_info.base_addr, CSL_EPWM_CC_CMP_B,
                                 cmpValue, CSL_EPWM_SHADOW_REG_CTRL_ENABLE,
                                 CSL_EPWM_CC_CMP_LOAD_MODE_CNT_EQ_ZERO, TRUE);
    return true;
}
*/

// This version immediately switches to new a frequency and duty cycle. With a clkDiv of 8, currently this 
// fuction has only can only do a max period of 4000us, after that, it bugs out.
// This fuction can not get to as low of frequencies as the original attempt at this function above... The
// Original attempt used CSL_EPWM_TB_COUNTER_DIR_UP_DOWN instead of CSL_EPWM_TB_COUNTER_DIR_UP for counterDir.
//
// Other random thing, since this fuction now messes with the TB counter, it now messes up pulse alignment.
// Another thing, this function now starts it first pulse consistently in deterministic time... This enables
// the use of pulse alignment.
//
// FIXME, more work to be done on this functions
//
// Possible bug: This functions might start a powerful pulse early in relative to the last powerful pulse. In
// other words, the separation between two pulses right before and after this function is called may not
// be well defined and consistant. If this is the case, look into changing the TB counter direction, and
// look into playing with the aqConfig in pwm_init_verbose()
bool pwm_set_frequency_and_duty_cycle_verbose(enum epwm_pin pin, uint32_t frequency, double duty_cycle, uint32_t clkDiv)
{
    // FIXME, at some point add code to prevent function from generating out of control PWM when given parameters that cause hell
    struct epwm_ch_info ch_info = epwm_pin_to_ch_info(pin);

    uint32_t tbClk = EPWM_MODULE_CLOCK_RATE / clkDiv;
    uint32_t tbPeriod = (tbClk / frequency) / 1U;
    uint32_t cmpValue = (tbPeriod - ((duty_cycle * tbPeriod) / 100U));

    // Stop the time-base counter to prevent any output glitches
    CSL_epwmTbSyncDisable(ch_info.base_addr);

    // Reset the time-base counter to zero to ensure a clean start
    CSL_epwmTbWriteTbCount(ch_info.base_addr, 0);

    // Configure Timebase in UP counting mode with immediate load
    CSL_epwmTbTimebaseClkCfg(ch_info.base_addr, tbClk, EPWM_MODULE_CLOCK_RATE);
    CSL_epwmTbPwmFreqCfg(ch_info.base_addr, tbClk, frequency, CSL_EPWM_TB_COUNTER_DIR_UP, CSL_EPWM_SHADOW_REG_CTRL_DISABLE);

    // Update compare values with immediate load
    CSL_epwmCounterComparatorCfg(ch_info.base_addr, CSL_EPWM_CC_CMP_A,
                                 cmpValue, CSL_EPWM_SHADOW_REG_CTRL_DISABLE,
                                 CSL_EPWM_CC_CMP_LOAD_MODE_CNT_EQ_ZERO, TRUE);
    CSL_epwmCounterComparatorCfg(ch_info.base_addr, CSL_EPWM_CC_CMP_B,
                                 cmpValue, CSL_EPWM_SHADOW_REG_CTRL_DISABLE,
                                 CSL_EPWM_CC_CMP_LOAD_MODE_CNT_EQ_ZERO, TRUE);

    // Reset the counter to zero to start with a fresh cycle 
    //
    // (IMPORTANT NOTE!) YES YOU DO NEED TO CALL THIS FUNCTION TWICE. ONCE ABOVE, AND ONCE 
    // RIGHT HERE. THIS WAS TESTED...NO IDEA WHY this IS THE CASE?!?!
    //
    // If this function is not both called here, and at the place above, you may end up with a
    // super powered pulse when transitioning from a higher duty cycle to a lower duty cycle.
    // For example, when transitioning from 65% duty to 1% duty may get a 80% duty cycle
    // bleed over pulse. Happens maybe 1 in 20 ish transitions.
    CSL_epwmTbWriteTbCount(ch_info.base_addr, 0);

    // Re-enable synchronization and start the counter
    CSL_epwmTbSyncEnable(ch_info.base_addr, 0, CSL_EPWM_TB_COUNTER_DIR_UP);

    return true;
}

// Example function call: pwm_init(P9_25b_EHRPWM4_B, 1, 5000, 25.0);
// clkDiv: clkDiv=1 seems the best for most cases. Need to study device more...
// 
// Initialization is slow due to a 700-microsecond delay for TB_CLKEN to kick in.
bool pwm_init_verbose(const enum epwm_pin pin, const uint32_t frequency, const double duty_cycle, const uint32_t clkDiv)
{
    struct epwm_ch_info ch_info = epwm_pin_to_ch_info(pin);

    // Set up sync mode and emulation mode
    CSL_epwmTbSetSyncOutMode(ch_info.base_addr, CSL_EPWM_TB_SYNC_OUT_EVT_SYNCIN);
    CSL_epwmTbSetEmulationMode(ch_info.base_addr, EPWM_TB_EMU_MODE_FREE_RUN);

    // Set frequency and duty cycle
    if (!pwm_set_frequency_and_duty_cycle_verbose(pin, frequency, duty_cycle, clkDiv))
    {
        return false; // Early return on failure
    }

    // Setup comparators and AQ actions
    const CSL_EpwmAqActionCfg_t aqConfig = {
        .zeroAction = CSL_EPWM_AQ_ACTION_LOW,
        .prdAction = CSL_EPWM_AQ_ACTION_DONOTHING,
        .cmpAUpAction = CSL_EPWM_AQ_ACTION_HIGH,
        .cmpADownAction = CSL_EPWM_AQ_ACTION_LOW,
        .cmpBUpAction = CSL_EPWM_AQ_ACTION_HIGH,
        .cmpBDownAction = CSL_EPWM_AQ_ACTION_LOW,
    };

    // Configure output based on channel option
    switch (ch_info.channel)
    {
    case A:
        CSL_epwmAqActionOnOutputCfg(ch_info.base_addr, CSL_EPWM_OUTPUT_CH_A, &aqConfig);
        break;
    case B:
        CSL_epwmAqActionOnOutputCfg(ch_info.base_addr, CSL_EPWM_OUTPUT_CH_B, &aqConfig);
        break;
    default:
        return false;
    }

    // Disable deadband and chopper, if not needed
    CSL_epwmDeadbandBypass(ch_info.base_addr);
    CSL_epwmChopperEnable(ch_info.base_addr, FALSE);

    // Disable trip zones (one-shot and cycle-by-cycle)
    CSL_epwmTzTripEventDisable(ch_info.base_addr, CSL_EPWM_TZ_EVENT_ONE_SHOT, 0U);
    CSL_epwmTzTripEventDisable(ch_info.base_addr, CSL_EPWM_TZ_EVENT_CYCLE_BY_CYCLE, 0U);

    // Configure event-trigger interrupt
    CSL_epwmEtIntrCfg(ch_info.base_addr, CSL_EPWM_ET_INTR_EVT_CNT_EQ_ZRO, CSL_EPWM_ET_INTR_PERIOD_FIRST_EVT);

    // TB_CLKEN update takes around 700 microseconds to take effect
    enable_epwm4_clock(); // Enable time-base clock (SUPER IMPORTANT!)

    return true;
}

// Init PWM with voltage set low
bool pwm_init(const enum epwm_pin pin){
    return pwm_init_verbose(pin, 1, 0.0, DEFAULT_CLK_DIV);
}

bool pwm_set_frequency_and_duty_cycle(enum epwm_pin pin, uint32_t frequency, double duty_cycle) {
    return pwm_set_frequency_and_duty_cycle_verbose(pin, frequency, duty_cycle, DEFAULT_CLK_DIV);
}

// Set PWM output voltage low
// pin: Value from "enum epwm_pin"
bool pwm_stop(enum epwm_pin pin)
{
    pwm_set_frequency_and_duty_cycle_verbose(pin, 1, 0.0, DEFAULT_CLK_DIV);

    return true;
}

// De-initialize/cleanup PWM
// pin: Value from "enum epwm_pin"
bool pwm_deinit(enum epwm_pin pin)
{
    pwm_stop(pin);

    // struct epwm_ch_info ch_info = epwm_pin_to_ch_info(pin);
    // EPWM_etIntrDisable(ch_info.base_addr);
    // EPWM_etIntrClear(ch_info.base_addr);
    return true;
}
