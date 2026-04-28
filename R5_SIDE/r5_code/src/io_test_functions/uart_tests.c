#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ti/csl/csl_uart.h>
#include <ti/csl/soc.h>
#include <ai64/bbai64_clocks.h>

#include <io_test_functions/uart_tests.h>

#define UART_MODULE_CLOCK_HZ 48000000U
#define UART_TEST_BAUD_RATE  19200U
#define UART_TEST_DATA_BITS  8U
#define UART_TEST_STOP_BITS  1U
#define UART_TEST_PARITY     0U
#define UART_RX_TIMEOUT_US   500000U

static uint32_t UARTSelectOperatingMode(uint32_t baudRate)
{
    if (baudRate >= 921600U) {
        return UART13x_OPER_MODE;
    }

    return UART16x_OPER_MODE;
}

static void UARTInit(uint32_t baseAddr, uint32_t moduleClk, uint32_t baudRate,
                     uint32_t dataBits, uint32_t stopBits, uint32_t parity)
{
    uint32_t divisorValue;
    uint32_t wLenStbFlag = 0;
    uint32_t parityFlag = 0;
    const uint32_t operatingMode = UARTSelectOperatingMode(baudRate);

    UARTModuleReset(baseAddr);

    UARTOperatingModeSelect(baseAddr, operatingMode);
    divisorValue = UARTDivisorValCompute(moduleClk, baudRate, operatingMode, 0);
    UARTDivisorLatchEnable(baseAddr);
    UARTDivisorLatchWrite(baseAddr, divisorValue);
    UARTDivisorLatchDisable(baseAddr);

    switch (dataBits) {
        case 5:
            wLenStbFlag = UART_FRAME_WORD_LENGTH_5;
            break;
        case 6:
            wLenStbFlag = UART_FRAME_WORD_LENGTH_6;
            break;
        case 7:
            wLenStbFlag = UART_FRAME_WORD_LENGTH_7;
            break;
        case 8:
        default:
            wLenStbFlag = UART_FRAME_WORD_LENGTH_8;
            break;
    }

    if (stopBits == 2) {
        wLenStbFlag |= UART_FRAME_NUM_STB_1_5_2;
    } else {
        wLenStbFlag |= UART_FRAME_NUM_STB_1;
    }

    switch (parity) {
        case 0:
            parityFlag = UART_PARITY_NONE;
            break;
        case 1:
            parityFlag = UART_ODD_PARITY;
            break;
        case 2:
            parityFlag = UART_EVEN_PARITY;
            break;
        default:
            parityFlag = UART_PARITY_NONE;
            break;
    }

    UARTLineCharacConfig(baseAddr, wLenStbFlag, parityFlag);

    const uint32_t fifoConfig = UART_FIFO_CONFIG(
        UART_TRIG_LVL_GRANULARITY_1,
        UART_TRIG_LVL_GRANULARITY_1,
        0,
        0,
        0,
        0,
        UART_DMA_EN_PATH_SCR,
        UART_DMA_MODE_0_ENABLE
    );

    UARTFIFOConfig(baseAddr, fifoConfig);
    UARTOperatingModeSelect(baseAddr, operatingMode);
}

static void UARTSend(uint32_t baseAddr, const uint8_t *data, uint32_t length)
{
    for (uint32_t i = 0; i < length; i++) {
        while (!UARTSpaceAvail(baseAddr));
        UARTCharPut(baseAddr, data[i]);
    }
}

static void UARTDrainRx(uint32_t baseAddr)
{
    while (UARTCharsAvail(baseAddr)) {
        (void)UARTCharGet(baseAddr);
    }
}

static uint32_t UARTReceiveWithTimeout(uint32_t baseAddr, uint8_t *buffer,
                                       uint32_t length, uint64_t timeoutUs)
{
    uint32_t count = 0;
    const uint64_t deadlineTicks = get_current_ticks() + dmicroseconds_to_ticks(timeoutUs);

    while ((count < length) && (get_current_ticks() < deadlineTicks)) {
        if (!UARTCharsAvail(baseAddr)) {
            continue;
        }

        buffer[count++] = (uint8_t)UARTCharGet(baseAddr);
    }

    return count;
}

void test_uart(uint32_t txBaseAddr, uint32_t rxBaseAddr)
{
    static const uint8_t txBuffer[] = "Hello UART polling!\n";
    uint8_t rxBuffer[sizeof(txBuffer)];
    uint32_t receivedCount;

    memset(rxBuffer, 0, sizeof(rxBuffer));

    printf("UART polling test: connect P9_16 (UART6_TX) to P8_28 (UART8_RX)\n");
    printf("UART polling test: tx=0x%08lx rx=0x%08lx baud=%lu\n",
           (unsigned long)txBaseAddr,
           (unsigned long)rxBaseAddr,
           (unsigned long)UART_TEST_BAUD_RATE);

    UARTInit(txBaseAddr, UART_MODULE_CLOCK_HZ, UART_TEST_BAUD_RATE,
             UART_TEST_DATA_BITS, UART_TEST_STOP_BITS, UART_TEST_PARITY);
    if (rxBaseAddr != txBaseAddr) {
        UARTInit(rxBaseAddr, UART_MODULE_CLOCK_HZ, UART_TEST_BAUD_RATE,
                 UART_TEST_DATA_BITS, UART_TEST_STOP_BITS, UART_TEST_PARITY);
    }

    UARTDrainRx(rxBaseAddr);
    UARTSend(txBaseAddr, txBuffer, sizeof(txBuffer) - 1U);

    receivedCount = UARTReceiveWithTimeout(rxBaseAddr, rxBuffer,
                                           sizeof(txBuffer) - 1U,
                                           UART_RX_TIMEOUT_US);
    rxBuffer[receivedCount] = '\0';

    printf("UART polling RX %lu/%lu bytes: \"%s\"\n",
           (unsigned long)receivedCount,
           (unsigned long)(sizeof(txBuffer) - 1U),
           (char *)rxBuffer);

    if ((receivedCount == (sizeof(txBuffer) - 1U)) &&
        (memcmp(txBuffer, rxBuffer, sizeof(txBuffer) - 1U) == 0)) {
        printf("UART polling RX test passed\n");
        return;
    }

    printf("UART polling RX test failed\n");
}
