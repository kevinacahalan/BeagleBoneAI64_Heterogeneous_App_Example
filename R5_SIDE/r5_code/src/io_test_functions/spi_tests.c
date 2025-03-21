#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ti/csl/csl_mcspi.h>
#include <ti/csl/soc.h>

#include "io_test_functions/spi_tests.h"


static void SPI6MasterSetUp(uint32_t base_addr, uint32_t ch_num, uint32_t in_clock, uint32_t out_clock)
{   

    uint32_t status = 1U; /* FALSE */

    /* Reset the McSPI instance.*/
    McSPIReset(base_addr);

    /* CLOCKACTIVITY bit - OCP and Functional clocks are maintained           */
    /* SIDLEMODE     bit - Ignore the idle request and configure in normal mode
     */
    /* AUTOIDLE      bit - Disable (OCP clock is running free, no gating)     */
    MCSPISysConfigSetup(base_addr, MCSPI_CLOCKS_OCP_ON_FUNC_ON,
                        MCSPI_SIDLEMODE_NO, MCSPI_WAKEUP_DISABLE,
                        MCSPI_AUTOIDLE_OFF);

    /* Enable chip select pin.*/
    McSPICSEnable(base_addr);

    /* Enable master mode of operation.*/
    McSPIMasterModeEnable(base_addr);

    /* Perform the necessary configuration for master mode. */
    status = McSPIMasterModeConfig(base_addr, MCSPI_SINGLE_CH,
                                   MCSPI_TX_RX_MODE,
                                   MCSPI_DATA_LINE_COMM_MODE_6, // D0 out, D1 in
                                   ch_num);

    if (0U == status)
    {
        printf("\nCommunication not supported, WEIRD NGL\n");
    }

    /* Configure the McSPI bus clock depending on clock mode. */
    McSPIClkConfig(base_addr, in_clock, out_clock, ch_num,
                   MCSPI_CLK_MODE_0);

    /* Configure the word length.*/
    McSPIWordLengthSet(base_addr, MCSPI_WORD_LENGTH(8), ch_num);

    /* Set polarity of SPIEN to low.*/
    McSPICSPolarityConfig(base_addr,
                          (MCSPI_CH0CONF_EPOL_ACTIVELOW <<
                           MCSPI_CH0CONF_EPOL_SHIFT), ch_num);

    /* Enable the transmitter FIFO of McSPI peripheral.*/
    McSPITxFIFOConfig(base_addr, MCSPI_TX_FIFO_ENABLE, ch_num);

    /* Enable the receiver FIFO of McSPI peripheral.*/
    McSPIRxFIFOConfig(base_addr, MCSPI_RX_FIFO_ENABLE, ch_num);
}

static void McSPIInitializeBuffers(size_t len, uint8_t *tx, uint8_t *rx)
{
    uint32_t index = 0;

    for (index = 0; index < len; index++)
    {
        /* Initialize the gTxBuffer McSPI6 with a known pattern of data */
        tx[index] = (uint8_t) index + 1U;
        /* Initialize the gRxBuffer McSPI6 with 0 */
        rx[index] = (uint8_t) 0;
    }
}


static void McSPIMSPolledModeTransfer(uint32_t base_addr, uint32_t ch_num, uint32_t length, uint8_t *tx, uint8_t *rx)
{
    uint32_t          channelStatus = 0;
    volatile uint32_t timeout1      = 0xFFFFU;
    uint8_t          *p_rx;
    uint8_t          *p_tx;

    p_tx    = tx;
    p_rx    = rx;


    while (0 != length)
    {
        timeout1      = 0xFFFF;
        channelStatus = McSPIChannelStatusGet(base_addr, ch_num);
        while (0 == (channelStatus & MCSPI_CH0STAT_TXS_MASK))
        {
            channelStatus = 0;
            channelStatus = McSPIChannelStatusGet(base_addr, ch_num);
            --timeout1;
            if (0 == timeout1)
            {
                printf("\nMcSPI6 TX Timed out!!");
                while (1) ;
            }
        }
        McSPITransmitData(base_addr, (uint32_t) (*p_tx++), ch_num);

        timeout1      = 0xFFFF;
        channelStatus = McSPIChannelStatusGet(base_addr, ch_num);
        while (0 == (channelStatus & MCSPI_CH0STAT_RXS_MASK))
        {
            channelStatus = 0;
            channelStatus = McSPIChannelStatusGet(base_addr, ch_num);
            --timeout1;
            if (0 == timeout1)
            {
                printf("\nMcSPI6 RX Timed out!!");
                while (1) ;
            }
        }
        *p_rx++ = (uint8_t)  McSPIReceiveData(base_addr, ch_num);

        length--;
    }
}

static void McSPIMSTransfer(uint32_t base_addr, uint32_t ch_num, uint32_t length, uint8_t *tx, uint8_t *rx)
{
    /* Enable the McSPI channel for communication.*/
    McSPIChannelEnable(base_addr, ch_num);

    /* SPIEN line is forced to low state.*/
    // This function must be called or the transfer will not work, kinda of weird
    McSPICSAssert(base_addr, ch_num);

    /* Initiate Transfer */
    McSPIMSPolledModeTransfer(base_addr, ch_num, length, tx, rx);

    /* Force SPIEN line to the inactive state.*/
    McSPICSDeAssert(base_addr, ch_num);

    /* Disable the McSPI channel.*/
    McSPIChannelDisable(base_addr, ch_num);
}


void test_spi3(void)
{
    const uint32_t mcspi_in_clock = 48000000;
    const uint32_t mcspi_out_freq = 1000000;
    const uint32_t mcspi_data_count = 256U;
    
    uint8_t tx[mcspi_data_count];
    uint8_t rx[mcspi_data_count];

    size_t base_addr = CSL_MCSPI6_CFG_BASE;
    uint32_t ch_num = 1U;
    SPI6MasterSetUp(base_addr, ch_num, mcspi_in_clock, mcspi_out_freq);
    McSPIInitializeBuffers(mcspi_data_count, tx, rx);
    printf("%d, %d\n", tx[1], tx[2]);
    McSPIMSTransfer(base_addr, ch_num, mcspi_data_count, tx, rx);
}

// Test MCSPI 6
// Before using this function make sure to setup your devices trees overlays correctly. Refer
// to the project README.md for device tree overlay for detail help. 
//
// To enable MCSPI6 with CS1, add /overlays/BONE-SPI1_0.dtbo to fdtoverlays in 
// /boot/firmware/extlinux/extlinux.conf. Restart the board to apply changes.
//
// PINS mappings:
// CS0: P9_17  (As of the writing of this comment, CS0 DOES NOT WORK! It's a device tree overlay problem)
// CS1: P9_23
// CLK: P9_22
// TX:  P9_21
// RX:  P9_18
void test_spi_mcpsi6(const uint32_t channel_number)
{
    const uint32_t mcspi_in_clock = 48000000;
    const uint32_t mcspi_out_freq = 10000000;

    // there are 4 channels, 0,1,2,3. A channel is a CS number
    // const uint32_t channel_number = MCSPI_CHANNEL_1;
    const uint32_t base_address = CSL_MCSPI6_CFG_BASE;
    const uint32_t pin_mode = MCSPI_DATA_LINE_COMM_MODE_6;
    uint32_t status = 1;


    // initialize
    McSPIReset(base_address);
    MCSPISysConfigSetup(base_address, MCSPI_CLOCKS_OCP_ON_FUNC_ON,
                        MCSPI_SIDLEMODE_NO, MCSPI_WAKEUP_DISABLE,
                        MCSPI_AUTOIDLE_OFF);
    McSPICSEnable(base_address);
    McSPIMasterModeEnable(base_address);
    status = McSPIMasterModeConfig(base_address,MCSPI_MODULCTRL_SINGLE_SINGLE, MCSPI_TX_ONLY_MODE,pin_mode, channel_number);
    if (0 == status){
        printf ("Weird issue at %d\n", __LINE__);
        exit(3);
    }
    McSPIClkConfig(base_address, mcspi_in_clock, mcspi_out_freq, channel_number, MCSPI_CLK_MODE_0);
    McSPIWordLengthSet(base_address, MCSPI_WORD_LENGTH(32), channel_number);
    McSPICSPolarityConfig(base_address, (MCSPI_CH1CONF_EPOL_ACTIVELOW <<
                        MCSPI_CH1CONF_EPOL_SHIFT), channel_number);
    McSPITxFIFOConfig(base_address, MCSPI_TX_FIFO_ENABLE, channel_number);
    McSPIRxFIFOConfig(base_address, MCSPI_RX_FIFO_DISABLE, channel_number);

    // transfer
    McSPIChannelEnable(base_address, channel_number);
    for(int j = 0; j < 100; j++){

        // For some reason the SPI clock goes crazy without this delay. Sometimes
        // transfers seem to disappear. Like maybe only about 44 out of 100 transfers
        // will go through. The clock would also sometimes be unstable
        for(volatile int i=0; i<200; i++);
        McSPICSAssert(base_address, channel_number);
        
        volatile uint32_t timeout1 = 0xFFFF;
        volatile uint32_t channelStatus = McSPIChannelStatusGet(base_address, channel_number);
        while (0 == (channelStatus & MCSPI_CH0STAT_TXS_MASK))
        {
            channelStatus = 0;
            channelStatus = McSPIChannelStatusGet(base_address, channel_number);
            --timeout1;
            if (0 == timeout1)
            {
                printf("\nMcSPI6 TX Timed out!!");
                while (1) ;
            }
        }
        McSPITransmitData(base_address, 0x55555500 | j, channel_number);
        
        // Without this delay the last 2 bytes from the final transfer will not go through
        for(volatile int i=0; i<200; i++);

        // wait for transfer to finish before CS de-assert (This wait does not seem to help at all)
        channelStatus = McSPIChannelStatusGet(base_address, channel_number);
        while (channelStatus & MCSPI_CH_STAT_EOT && 0 == (channelStatus & CSL_MCSPI_CH0STAT_TXS_MASK))
        {
            channelStatus = 0;
            channelStatus = McSPIChannelStatusGet(base_address, channel_number);
            --timeout1;
            if (0 == timeout1)
            {
                printf("\nMcSPI6 TX Timed out!!");
                while (1) ;
            }
        }
        McSPICSDeAssert(base_address, channel_number);
    }
}

// Test MCSPI 7
// Before using this function make sure to setup your devices trees overlays correctly. Refer
// to the project README.md for device tree overlay for detail help. 
//
// To enable MCSPI7 with CS0, add /overlays/BONE-SPI0_0.dtbo to fdtoverlays in 
// /boot/firmware/extlinux/extlinux.conf. Restart the board to apply changes.
//
// PINS mappings:
// CS0: P9_28
// CLK: P9_31
// TX:  P9_30
// RX:  P9_29
void test_spi_mcspi7(const uint32_t channel_number)
{
    // printf("Testing mcspi7 100 32bit transfers at 48MHz in format 0x555555[NN], NN is transfer counter\n");
    printf("MCSPI7, connect logic analyzer to pins P9_28(cs), P9_31(clk), and P9_30(MOSI)\n");
    const uint32_t mcspi_in_clock = 48000000;
    const uint32_t mcspi_out_freq = 10000000;

    const uint32_t base_address = CSL_MCSPI7_CFG_BASE;
    const uint32_t pin_mode = MCSPI_DATA_LINE_COMM_MODE_6;
    uint32_t status = 1;


    // initialize
    McSPIReset(base_address);
    MCSPISysConfigSetup(base_address, MCSPI_CLOCKS_OCP_ON_FUNC_ON,
                        MCSPI_SIDLEMODE_NO, MCSPI_WAKEUP_DISABLE,
                        MCSPI_AUTOIDLE_OFF);
    McSPICSEnable(base_address);
    McSPIMasterModeEnable(base_address);
    status = McSPIMasterModeConfig(base_address,MCSPI_MODULCTRL_SINGLE_SINGLE, MCSPI_TX_ONLY_MODE,pin_mode, channel_number);
    if (0 == status){
        printf ("Weird issue at %d\n", __LINE__);
        exit(3);
    }
    McSPIClkConfig(base_address, mcspi_in_clock, mcspi_out_freq, channel_number, MCSPI_CLK_MODE_0);
    McSPIWordLengthSet(base_address, MCSPI_WORD_LENGTH(32), channel_number);
    McSPICSPolarityConfig(base_address, (MCSPI_CH1CONF_EPOL_ACTIVELOW <<
                        MCSPI_CH1CONF_EPOL_SHIFT), channel_number);
    McSPITxFIFOConfig(base_address, MCSPI_TX_FIFO_ENABLE, channel_number);
    McSPIRxFIFOConfig(base_address, MCSPI_RX_FIFO_DISABLE, channel_number);

    // transfer
    McSPIChannelEnable(base_address, channel_number);
    for(int j = 0; j < 100; j++){

        // For some reason the SPI clock goes crazy without this delay. Sometimes
        // transfers seem to disappear. Like maybe only about 44 out of 100 transfers
        // will go through. The clock would also sometimes be unstable
        for(volatile int i=0; i<200; i++);
        McSPICSAssert(base_address, channel_number);
        
        volatile uint32_t timeout1 = 0xFFFF;
        volatile uint32_t channelStatus = McSPIChannelStatusGet(base_address, channel_number);
        while (0 == (channelStatus & MCSPI_CH0STAT_TXS_MASK))
        {
            channelStatus = 0;
            channelStatus = McSPIChannelStatusGet(base_address, channel_number);
            --timeout1;
            if (0 == timeout1)
            {
                printf("\nMcSPI6 TX Timed out!!");
                while (1) ;
            }
        }
        McSPITransmitData(base_address, 0x55555500 | j, channel_number);
        
        // Without this delay the last 2 bytes from the final transfer will not go through
        for(volatile int i=0; i<200; i++);

        // wait for transfer to finish before CS de-assert (This wait does not seem to help at all)
        channelStatus = McSPIChannelStatusGet(base_address, channel_number);
        while (channelStatus & MCSPI_CH_STAT_EOT && 0 == (channelStatus & CSL_MCSPI_CH0STAT_TXS_MASK))
        {
            channelStatus = 0;
            channelStatus = McSPIChannelStatusGet(base_address, channel_number);
            --timeout1;
            if (0 == timeout1)
            {
                printf("\nMcSPI6 TX Timed out!!");
                while (1) ;
            }
        }
        McSPICSDeAssert(base_address, channel_number);
    }
}

void test_spi_mcspi6_and_mcspi7()
{
    const uint32_t mcspi_in_clock = 48000000;
    //const uint32_t mcspi_in_clock = 128000000;
    const uint32_t mcspi_out_freq = 10000000;


    const uint32_t spi6_channel_number = MCSPI_CHANNEL_1; // CS for SPI6
    const uint32_t spi6_base_address = CSL_MCSPI6_CFG_BASE;
    const uint32_t spi6_pin_mode = MCSPI_DATA_LINE_COMM_MODE_6;
    uint32_t spi6_status = 1;

    const uint32_t spi7_channel_number = MCSPI_CHANNEL_0; // CS for SPI7
    const uint32_t spi7_base_address = CSL_MCSPI7_CFG_BASE;
    const uint32_t spi7_pin_mode = MCSPI_DATA_LINE_COMM_MODE_6;
    uint32_t spi7_status = 1;

    // Initialize spi 6
    McSPIReset(spi6_base_address);
    MCSPISysConfigSetup(spi6_base_address, MCSPI_CLOCKS_OCP_ON_FUNC_ON,
                        MCSPI_SIDLEMODE_NO, MCSPI_WAKEUP_DISABLE,
                        MCSPI_AUTOIDLE_OFF);
    McSPICSEnable(spi6_base_address);
    McSPIMasterModeEnable(spi6_base_address);
    spi6_status = McSPIMasterModeConfig(spi6_base_address,MCSPI_MODULCTRL_SINGLE_SINGLE, MCSPI_TX_ONLY_MODE,spi6_pin_mode, spi6_channel_number);
    if (0 == spi6_status){
        printf ("Weird issue at %d\n", __LINE__);
        exit(3);
    }
    McSPIClkConfig(spi6_base_address, mcspi_in_clock, mcspi_out_freq, spi6_channel_number, MCSPI_CLK_MODE_0);
    McSPIWordLengthSet(spi6_base_address, MCSPI_WORD_LENGTH(32), spi6_channel_number);
    McSPICSPolarityConfig(spi6_base_address, (MCSPI_CH1CONF_EPOL_ACTIVELOW <<
                        MCSPI_CH1CONF_EPOL_SHIFT), spi6_channel_number);
    McSPITxFIFOConfig(spi6_base_address, MCSPI_TX_FIFO_ENABLE, spi6_channel_number);
    McSPIRxFIFOConfig(spi6_base_address, MCSPI_RX_FIFO_DISABLE, spi6_channel_number);


    // initialize spi 7
    McSPIReset(spi7_base_address);
    MCSPISysConfigSetup(spi7_base_address, MCSPI_CLOCKS_OCP_ON_FUNC_ON,
                        MCSPI_SIDLEMODE_NO, MCSPI_WAKEUP_DISABLE,
                        MCSPI_AUTOIDLE_OFF);
    McSPICSEnable(spi7_base_address);
    McSPIMasterModeEnable(spi7_base_address);
    spi7_status = McSPIMasterModeConfig(spi7_base_address,MCSPI_MODULCTRL_SINGLE_SINGLE, MCSPI_TX_ONLY_MODE,spi7_pin_mode, spi7_channel_number);
    if (0 == spi7_status){
        printf ("Weird issue at %d\n", __LINE__);
        exit(3);
    }
    McSPIClkConfig(spi7_base_address, mcspi_in_clock, mcspi_out_freq, spi7_channel_number, MCSPI_CLK_MODE_0);
    McSPIWordLengthSet(spi7_base_address, MCSPI_WORD_LENGTH(32), spi7_channel_number);
    McSPICSPolarityConfig(spi7_base_address, (MCSPI_CH1CONF_EPOL_ACTIVELOW <<
                        MCSPI_CH1CONF_EPOL_SHIFT), spi7_channel_number);
    McSPITxFIFOConfig(spi7_base_address, MCSPI_TX_FIFO_ENABLE, spi7_channel_number);
    McSPIRxFIFOConfig(spi7_base_address, MCSPI_RX_FIFO_DISABLE, spi7_channel_number);

    // transfer
    McSPIChannelEnable(spi6_base_address, spi6_channel_number);
    McSPIChannelEnable(spi7_base_address, spi7_channel_number);
    for(int j = 0; j < 100; j++){

        // For some reason the SPI clock goes crazy without this delay. Sometimes
        // transfers seem to disappear. Like maybe only about 44 out of 100 transfers
        // will go through. The clock would also sometimes be unstable
        for(volatile int i=0; i<200; i++);
        McSPICSAssert(spi6_base_address, spi6_channel_number);
        McSPICSAssert(spi7_base_address, spi7_channel_number);
        
        volatile uint32_t timeout1 = 0xFFFF;
        volatile uint32_t spi6_channelStatus = McSPIChannelStatusGet(spi6_base_address, spi6_channel_number);
        volatile uint32_t spi7_channelStatus = McSPIChannelStatusGet(spi7_base_address, spi7_channel_number);
        while (0 == (spi6_channelStatus & MCSPI_CH0STAT_TXS_MASK) && 0 == (spi7_channelStatus & MCSPI_CH0STAT_TXS_MASK))
        {
            spi6_channelStatus = 0;
            spi7_channelStatus = 0;
            spi6_channelStatus = McSPIChannelStatusGet(spi6_base_address, spi6_channel_number);
            spi7_channelStatus = McSPIChannelStatusGet(spi7_base_address, spi7_channel_number);
            --timeout1;
            if (0 == timeout1)
            {
                printf("\nMcSPI6 TX Timed out!!");
                while (1) ;
            }
        }
        McSPITransmitData(spi6_base_address, 0x55555500 | j, spi6_channel_number);
        McSPITransmitData(spi7_base_address, 0x55555500 | j, spi7_channel_number);
        
        // Without this delay the last 2 bytes from the final transfer will not go through
        for(volatile int i=0; i<200; i++);

        // wait for trasfer to finish before CS de-assert (This wait does not seem to help at all)
        spi6_channelStatus = McSPIChannelStatusGet(spi6_base_address, spi6_channel_number);
        spi7_channelStatus = McSPIChannelStatusGet(spi7_base_address, spi7_channel_number);
        while (spi6_channelStatus & MCSPI_CH_STAT_EOT && 0 == (spi6_channelStatus & CSL_MCSPI_CH0STAT_TXS_MASK) && spi7_channelStatus & MCSPI_CH_STAT_EOT && 0 == (spi7_channelStatus & CSL_MCSPI_CH0STAT_TXS_MASK))
        {
            spi6_channelStatus = 0;
            spi7_channelStatus = 0;
            spi6_channelStatus = McSPIChannelStatusGet(spi6_base_address, spi6_channel_number);
            spi7_channelStatus = McSPIChannelStatusGet(spi7_base_address, spi7_channel_number);
            --timeout1;
            if (0 == timeout1)
            {
                printf("\nMcSPI6 TX Timed out!!");
                while (1) ;
            }
        }
        McSPICSDeAssert(spi6_base_address, spi6_channel_number);
        McSPICSDeAssert(spi7_base_address, spi7_channel_number);
    }
}