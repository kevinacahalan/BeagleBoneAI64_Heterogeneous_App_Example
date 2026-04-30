#ifndef UART_TESTS_H
#define UART_TESTS_H

#include <stdint.h>

#include <ai64/bbai64_uart.h>

#define UART_TEST_TX_BASE BBAI64_UART6_BASE
#define UART_TEST_RX_BASE BBAI64_UART8_BASE

void test_uart(uint32_t txBaseAddr, uint32_t rxBaseAddr);

#endif // UART_TESTS_H
