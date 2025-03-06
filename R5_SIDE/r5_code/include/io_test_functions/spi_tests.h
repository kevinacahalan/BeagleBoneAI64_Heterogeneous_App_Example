#ifndef SPI_TESTS_H
#define SPI_TESTS_H

#include <stdint.h>

void test_spi3(void);
void test_spi_mcpsi6(const uint32_t channel_number);
void test_spi_mcspi7(const uint32_t channel_number);
void test_spi_mcspi6_and_mcspi7();

#endif // SPI_TESTS_H
