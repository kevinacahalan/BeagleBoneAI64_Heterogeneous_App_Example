#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <ti/csl/csl_epwm.h>
#include <ti/csl/soc.h>

#include <ai64/bbai64_epwm.h>

#include <ai64/sys_pwmss.h>
#include <ai64/sys_ctrlmmr.h>
#include <io_test_functions/epwm_tests.h>

// This example here to show what happens on the low low level. For feature reference this example
// may come to use.
void test_epwm4b_csl_layer()
{
    printf("\nStarting %s:%s\n", __func__, __FILE__);
    const uint32_t base_addr = CSL_EHRPWM4_EPWM_BASE;
    const uint32_t moduleClk = 125000000; // figured out by running `sudo k3conf dump clocks 87` on board
    const uint32_t clkDiv = 8;            // Not sure why this is 8, seems to work better at 4
    const uint32_t pwmFreq = 10000;
    const float duty_cycle = 23.0; // 23% duty cycle

    const uint32_t tbClk = moduleClk / clkDiv;
    const uint32_t tbPeriod = (tbClk / pwmFreq) / 2U;
    const uint32_t cmpValue = (tbPeriod - ((duty_cycle * tbPeriod) / 100U));

    // uint32_t periodCounts;

    // EPWM4.TBCTL = 0x0000; // maybe not needed

    // Configure Timebase
    CSL_epwmTbTimebaseClkCfg(base_addr, tbClk, moduleClk);
    // EPWM4.TBCTL_bit.HSPCLKDIV = TB_DIV4; // If this line is commented out my CSL implementation freq is halved
    // EPWM4.TBCTL_bit.CLKDIV = TB_DIV1;

    // For some reason this function halves your freq with counterDir set to up-down
    CSL_epwmTbPwmFreqCfg(base_addr, tbClk, pwmFreq, CSL_EPWM_TB_COUNTER_DIR_UP_DOWN, CSL_EPWM_SHADOW_REG_CTRL_ENABLE);
    // periodCounts = 250000000/pwmFreq;
    // periodCounts/= 4; // why?... was 4
    // periodCounts/= 4;
    // EPWM4.TBPRD = periodCounts;
    // EPWM4.TBCTL_bit.CTRMODE = TB_COUNT_UPDOWN;
    // EPWM4.TBCTL_bit.PRDLD = TB_SHADOW;

    CSL_epwmTbSyncDisable(base_addr);
    CSL_epwmTbSetSyncOutMode(base_addr, CSL_EPWM_TB_SYNC_OUT_EVT_SYNCIN);
    CSL_epwmTbSetEmulationMode(base_addr, EPWM_TB_EMU_MODE_FREE_RUN);

    // EPWM4.CMPCTL = 0;

    // It turns out that setting up channel A still matters, even when using channel B
    if (!CSL_epwmCounterComparatorCfg(base_addr,
                                      CSL_EPWM_CC_CMP_A,
                                      cmpValue,
                                      CSL_EPWM_SHADOW_REG_CTRL_ENABLE,
                                      CSL_EPWM_CC_CMP_LOAD_MODE_CNT_EQ_ZERO,
                                      TRUE))
    {
        printf("Problem configuring counter comparator A\n");
    }
    // EPWM4.CMPCTL_bit.LOADAMODE = CC_CTR_ZERO;
    // EPWM4.CMPCTL_bit.SHDWAMODE = CC_SHADOW;

    // periodCounts/= 2; // /1 for no pulse, /2 for 50%, /4 for 25%
    //  EPWM4.CMPB = periodCounts;
    if (!CSL_epwmCounterComparatorCfg(base_addr,
                                      CSL_EPWM_CC_CMP_B,
                                      cmpValue,
                                      CSL_EPWM_SHADOW_REG_CTRL_ENABLE,
                                      CSL_EPWM_CC_CMP_LOAD_MODE_CNT_EQ_ZERO,
                                      TRUE))
    {
        printf("Problem configuring counter comparator B\n");
    }
    // EPWM4.CMPCTL_bit.LOADBMODE = CC_CTR_ZERO;
    // EPWM4.CMPCTL_bit.SHDWBMODE = CC_SHADOW;

    //////
    const CSL_EpwmAqActionCfg_t aqConfig = {
        .zeroAction = CSL_EPWM_AQ_ACTION_DONOTHING,
        .prdAction = CSL_EPWM_AQ_ACTION_DONOTHING,
        .cmpAUpAction = CSL_EPWM_AQ_ACTION_HIGH,
        .cmpADownAction = CSL_EPWM_AQ_ACTION_LOW,
        .cmpBUpAction = CSL_EPWM_AQ_ACTION_HIGH,
        .cmpBDownAction = CSL_EPWM_AQ_ACTION_LOW,
    };
    CSL_epwmAqActionOnOutputCfg(base_addr, CSL_EPWM_OUTPUT_CH_B, &aqConfig);
    //////

    CSL_epwmDeadbandBypass(base_addr);
    CSL_epwmChopperEnable(base_addr, FALSE);
    // EPWM4.DBCTL = 0;
    // EPWM4.DBRED = 0;
    // EPWM4.DBFED = 0;

    CSL_epwmTzTripEventDisable(base_addr, CSL_EPWM_TZ_EVENT_ONE_SHOT, 0U);
    CSL_epwmTzTripEventDisable(base_addr, CSL_EPWM_TZ_EVENT_CYCLE_BY_CYCLE, 0U);
    // EPWM4.TZSEL = 0;
    // EPWM4.TZSEL_bit.OSHT = 0;
    // EPWM4.TZSEL_bit.CBC = 0;

    CSL_epwmEtIntrCfg(base_addr, CSL_EPWM_ET_INTR_EVT_CNT_EQ_ZRO, CSL_EPWM_ET_INTR_PERIOD_FIRST_EVT);
    // EPWM4.ETSEL = 0;
    // EPWM4.ETSEL_bit.INTEN  = ET_DISABLE;
    // EPWM4.ETSEL_bit.INTSEL = ET_CTR_ZERO;
    // EPWM4.ETPS_bit.INTCNT  = ET_1ST;
    // EPWM4.ETPS_bit.INTPRD  = ET_1ST;

    // CSL_epwmEtIntrEnable(base_addr);
    // EPWM4.ETSEL_bit.INTEN  = ET_ENABLE;

    EHRPWM_CTRLMMR.EHRPWM4_bit.TB_CLKEN = 1; // Enable clock (SUPER IMPORTANT!)

    printf("Done %s:%s\n", __func__, __FILE__);
}

// This function should fire off a pwm signal from P9-25
// Make sure to load our custom device tree overlay that mux's out PWM. You can check if ehrpwm4_b is
// muxed to P9-25 using the showpins.pl script
void run_pwm_test(int n) {
    printf("Doing an EPWM test on P9_25...Flashing an LED\n");
    pwm_init(P9_25b_EHRPWM4_B);
    pwm_stop(P9_25b_EHRPWM4_B);

    #define DELAY 20000000
    for (int i = 0; i < n; i++)
    {
        for(volatile int j=0; j< DELAY; j++);
        pwm_set_frequency_and_duty_cycle(P9_25b_EHRPWM4_B, 5000, 15.0);
        for(volatile int j=0; j< DELAY; j++);
        pwm_set_frequency_and_duty_cycle(P9_25b_EHRPWM4_B, 5000, 75.0);
    }

    pwm_deinit(P9_25b_EHRPWM4_B);
    printf("Done PWM test\n");
}