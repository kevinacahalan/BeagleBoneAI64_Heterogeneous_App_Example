#ifndef BBAI64_UART_H
#define BBAI64_UART_H

#include <stdbool.h>
#include <stdint.h>

// These are also defined in ti/csl/soc/j721e/src/cslr_soc_baseaddress.h with the
// CSL_UART[#]_BASE naming scheme.
#define BBAI64_UART0_BASE (0x2800000UL)
#define BBAI64_UART1_BASE (0x2810000UL)
#define BBAI64_UART2_BASE (0x2820000UL)
#define BBAI64_UART3_BASE (0x2830000UL)
#define BBAI64_UART4_BASE (0x2840000UL)
#define BBAI64_UART5_BASE (0x2850000UL)
#define BBAI64_UART6_BASE (0x2860000UL)
#define BBAI64_UART7_BASE (0x2870000UL)
#define BBAI64_UART8_BASE (0x2880000UL)
#define BBAI64_UART9_BASE (0x2890000UL)

#define BBAI64_UART_MODULE_CLOCK_HZ   48000000U
#define BBAI64_UART_DEFAULT_BAUD_RATE 115200U

typedef enum
{
    BBAI64_UART_MODULE_0 = 0,
    BBAI64_UART_MODULE_1,
    BBAI64_UART_MODULE_2,
    BBAI64_UART_MODULE_3,
    BBAI64_UART_MODULE_4,
    BBAI64_UART_MODULE_5,
    BBAI64_UART_MODULE_6,
    BBAI64_UART_MODULE_7,
    BBAI64_UART_MODULE_8,
    BBAI64_UART_MODULE_9,
    BBAI64_UART_MODULE_INVALID = 255,
} bbai64_uart_module_t;

typedef enum
{
    BBAI64_UART_PARITY_NONE = 0,
    BBAI64_UART_PARITY_ODD,
    BBAI64_UART_PARITY_EVEN,
} bbai64_uart_parity_t;

typedef struct
{
    uint32_t base_addr;
    uint32_t baud_rate;
    uint32_t module_input_clock_hz;
    uint8_t data_bits;
    uint8_t stop_bits;
    bbai64_uart_parity_t parity;
} bbai64_uart_config_t;

typedef struct
{
    bbai64_uart_config_t config;
    bool is_initialized;
} bbai64_uart_t;

#define BBAI64_UART_FRAME_MAX_MARKER_BYTES 8U

typedef enum
{
    BBAI64_UART_FRAME_LENGTH_U8 = 0,
    BBAI64_UART_FRAME_LENGTH_U16_LE,
    BBAI64_UART_FRAME_LENGTH_U16_BE,
} bbai64_uart_frame_length_t;

typedef enum
{
    BBAI64_UART_FRAME_CHECKSUM_NONE = 0,
    BBAI64_UART_FRAME_CHECKSUM_XOR8,
    BBAI64_UART_FRAME_CHECKSUM_SUM8,
    BBAI64_UART_FRAME_CHECKSUM_CRC16_IBM,
    BBAI64_UART_FRAME_CHECKSUM_CRC16_CCITT_FALSE,
} bbai64_uart_frame_checksum_t;

enum
{
    BBAI64_UART_FRAME_CHECKSUM_ON_HEADER = (1U << 0),
    BBAI64_UART_FRAME_CHECKSUM_ON_LENGTH = (1U << 1),
    BBAI64_UART_FRAME_CHECKSUM_ON_PAYLOAD = (1U << 2),
};

typedef struct
{
    uint8_t start_bytes[BBAI64_UART_FRAME_MAX_MARKER_BYTES];
    uint8_t start_bytes_length;
    uint8_t end_bytes[BBAI64_UART_FRAME_MAX_MARKER_BYTES];
    uint8_t end_bytes_length;
    bbai64_uart_frame_length_t length_type;
    int16_t length_adjustment;
    bbai64_uart_frame_checksum_t checksum_type;
    uint8_t checksum_scope_flags;
    bool checksum_big_endian;
} bbai64_uart_frame_config_t;

typedef enum
{
    BBAI64_UART_FRAME_READ_OK = 0,
    BBAI64_UART_FRAME_READ_TIMEOUT,
    BBAI64_UART_FRAME_READ_INVALID_ARGUMENT,
    BBAI64_UART_FRAME_READ_INVALID_CONFIG,
    BBAI64_UART_FRAME_READ_LENGTH_OUT_OF_RANGE,
    BBAI64_UART_FRAME_READ_BUFFER_TOO_SMALL,
    BBAI64_UART_FRAME_READ_END_MARKER_MISMATCH,
    BBAI64_UART_FRAME_READ_CHECKSUM_MISMATCH,
} bbai64_uart_frame_read_status_t;

/*
 * Typical flow:
 * 1. Call bbai64_uart_config_init() or bbai64_uart_config_init_from_base_addr().
 * 2. Override baud_rate or line settings as needed.
 * 3. Call bbai64_uart_init().
 * 4. Use bbai64_uart_write(), bbai64_uart_write_cstr(),
 *    bbai64_uart_read_available(), or bbai64_uart_read_with_timeout().
 * 5. For packetized UART protocols, use bbai64_uart_frame_config_init(),
 *    bbai64_uart_write_frame(), and bbai64_uart_read_frame().
 */
uint32_t bbai64_uart_module_to_base_addr(bbai64_uart_module_t module);
bbai64_uart_module_t bbai64_uart_base_addr_to_module(uint32_t base_addr);
const char *bbai64_uart_module_name(bbai64_uart_module_t module);

void bbai64_uart_config_init(bbai64_uart_config_t *config, bbai64_uart_module_t module);
bool bbai64_uart_config_init_from_base_addr(bbai64_uart_config_t *config, uint32_t base_addr);

bool bbai64_uart_init(bbai64_uart_t *uart, const bbai64_uart_config_t *config);
void bbai64_uart_deinit(bbai64_uart_t *uart);
bool bbai64_uart_is_initialized(const bbai64_uart_t *uart);

bool bbai64_uart_tx_ready(const bbai64_uart_t *uart);
bool bbai64_uart_rx_ready(const bbai64_uart_t *uart);
void bbai64_uart_drain_rx(const bbai64_uart_t *uart);

bool bbai64_uart_write_byte(const bbai64_uart_t *uart, uint8_t byte);
bool bbai64_uart_write(const bbai64_uart_t *uart, const uint8_t *data, uint32_t length);
bool bbai64_uart_write_cstr(const bbai64_uart_t *uart, const char *text);

bool bbai64_uart_read_byte_nonblocking(const bbai64_uart_t *uart, uint8_t *byte);
uint32_t bbai64_uart_read_available(const bbai64_uart_t *uart, uint8_t *buffer, uint32_t buffer_length);
uint32_t bbai64_uart_read_blocking(const bbai64_uart_t *uart, uint8_t *buffer, uint32_t length);
uint32_t bbai64_uart_read_with_timeout(const bbai64_uart_t *uart, uint8_t *buffer, uint32_t length, uint64_t timeout_us);

void bbai64_uart_frame_config_init(bbai64_uart_frame_config_t *config);
uint32_t bbai64_uart_frame_encoded_size(const bbai64_uart_frame_config_t *config, uint32_t payload_length);
bool bbai64_uart_write_frame(const bbai64_uart_t *uart, const bbai64_uart_frame_config_t *frame_config,
                             const uint8_t *payload, uint32_t payload_length);
bbai64_uart_frame_read_status_t bbai64_uart_read_frame(const bbai64_uart_t *uart,
                                                       const bbai64_uart_frame_config_t *frame_config,
                                                       uint8_t *payload,
                                                       uint32_t payload_buffer_length,
                                                       uint32_t *payload_length,
                                                       uint64_t timeout_us);
const char *bbai64_uart_frame_read_status_name(bbai64_uart_frame_read_status_t status);

#endif