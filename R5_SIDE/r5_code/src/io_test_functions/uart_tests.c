#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ti/csl/csl_uart.h>
#include <ti/csl/soc.h>

// #define UART_BASE_ADDR CSL_UART2_BASE


void UARTInit(uint32_t baseAddr, uint32_t moduleClk, uint32_t baudRate,
              uint32_t dataBits, uint32_t stopBits, uint32_t parity) {
    uint32_t divisorValue;
    uint32_t wLenStbFlag = 0;
    uint32_t parityFlag = 0;

    // Reset UART module
    UARTModuleReset(baseAddr);

    // Select UART16x operating mode
    UARTOperatingModeSelect(baseAddr, UART16x_OPER_MODE);

    // Compute divisor value for desired baud rate
    divisorValue = UARTDivisorValCompute(moduleClk, baudRate, UART16x_OPER_MODE, 0);

    // Enable access to Divisor Latches
    UARTDivisorLatchEnable(baseAddr);

    // Write divisor value
    UARTDivisorLatchWrite(baseAddr, divisorValue);

    // Disable access to Divisor Latches
    UARTDivisorLatchDisable(baseAddr);

    // Configure Line Characteristics
    // Set word length
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

    // Set number of stop bits
    if (stopBits == 2) {
        wLenStbFlag |= UART_FRAME_NUM_STB_1_5_2;
    } else {
        wLenStbFlag |= UART_FRAME_NUM_STB_1;
    }

    // Set parity
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

    // Apply line characteristics settings
    UARTLineCharacConfig(baseAddr, wLenStbFlag, parityFlag);

    // Configure FIFO settings
    uint32_t fifoConfig = UART_FIFO_CONFIG(
        UART_TRIG_LVL_GRANULARITY_1,  // txGra
        UART_TRIG_LVL_GRANULARITY_1,  // rxGra
        1,                            // txTrig
        1,                            // rxTrig
        1,                            // txClr
        1,                            // rxClr
        UART_DMA_EN_PATH_SCR,         // dmaEnPath
        UART_DMA_MODE_0_ENABLE        // dmaMode
    );

    UARTFIFOConfig(baseAddr, fifoConfig);

    // Re-enable UART in UART16x mode
    UARTOperatingModeSelect(baseAddr, UART16x_OPER_MODE);
}

void UARTSend(uint32_t baseAddr, const uint8_t *data, uint32_t length) {
    for (uint32_t i = 0; i < length; i++) {
        // Wait until space is available in the FIFO
        while (!UARTSpaceAvail(baseAddr));
        UARTCharPut(baseAddr, data[i]);
    }
}

uint32_t UARTReceive(uint32_t baseAddr, uint8_t *buffer, uint32_t length) {
    uint32_t count = 0;
    while (count < length) {
        // Wait until data is available
        while (!UARTCharsAvail(baseAddr));
        buffer[count++] = UARTCharGet(baseAddr);
    }
    return count;
}

void test_uart(uint32_t baseAddr) {
    uint8_t txBuffer[] = "Hello UART!\n";
    uint8_t rxBuffer[100];
    uint32_t baudRate = 19200;
    uint32_t dataBits = 8;
    uint32_t stopBits = 1;
    uint32_t parity = 0;  // 0: None, 1: Odd, 2: Even
    uint32_t moduleClk = 48000000U;
    
    // stop warning for now
    (void)rxBuffer;

    printf("UART TEST...");

    // Initialize UART
    UARTInit(baseAddr, moduleClk, baudRate, dataBits, stopBits, parity);

    // Send data over UART
    UARTSend(baseAddr, txBuffer, sizeof(txBuffer) - 1);
    printf("done!\n");
}
