#ifndef UART_TESTS_H
#define UART_TESTS_H


// These are also defined ti/csl/soc/j721e/src/cslr_soc_baseaddress.h with the CSL_UART[#]_BASE naming scheme
#define BBAI64_UART0_BASE                                                                             (0x2800000UL)
#define BBAI64_UART1_BASE                                                                             (0x2810000UL)
#define BBAI64_UART2_BASE                                                                             (0x2820000UL)
#define BBAI64_UART3_BASE                                                                             (0x2830000UL)
#define BBAI64_UART4_BASE                                                                             (0x2840000UL)
#define BBAI64_UART5_BASE                                                                             (0x2850000UL)
#define BBAI64_UART6_BASE                                                                             (0x2860000UL)
#define BBAI64_UART7_BASE                                                                             (0x2870000UL)
#define BBAI64_UART8_BASE                                                                             (0x2880000UL)
#define BBAI64_UART9_BASE                                                                             (0x2890000UL)

void test_uart(uint32_t baseAddr);

#endif // UART_TESTS_H
