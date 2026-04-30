#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include <ti/csl/csl_uart.h>

#include <ai64/bbai64_clocks.h>
#include <ai64/bbai64_uart.h>

typedef struct
{
    bbai64_uart_frame_checksum_t type;
    uint16_t value;
} bbai64_uart_checksum_state_t;

static uint32_t bbai64_uart_select_operating_mode(uint32_t baud_rate)
{
    if (baud_rate >= 921600U) {
        return UART13x_OPER_MODE;
    }

    return UART16x_OPER_MODE;
}

static bool bbai64_uart_build_line_config(const bbai64_uart_config_t *config,
                                          uint32_t *word_length_stop_bits,
                                          uint32_t *parity_flag)
{
    if ((config == NULL) || (word_length_stop_bits == NULL) || (parity_flag == NULL)) {
        return false;
    }

    switch (config->data_bits) {
        case 5U:
            *word_length_stop_bits = UART_FRAME_WORD_LENGTH_5;
            break;
        case 6U:
            *word_length_stop_bits = UART_FRAME_WORD_LENGTH_6;
            break;
        case 7U:
            *word_length_stop_bits = UART_FRAME_WORD_LENGTH_7;
            break;
        case 8U:
            *word_length_stop_bits = UART_FRAME_WORD_LENGTH_8;
            break;
        default:
            return false;
    }

    if (config->stop_bits == 2U) {
        *word_length_stop_bits |= UART_FRAME_NUM_STB_1_5_2;
    } else if (config->stop_bits == 1U) {
        *word_length_stop_bits |= UART_FRAME_NUM_STB_1;
    } else {
        return false;
    }

    switch (config->parity) {
        case BBAI64_UART_PARITY_NONE:
            *parity_flag = UART_PARITY_NONE;
            break;
        case BBAI64_UART_PARITY_ODD:
            *parity_flag = UART_ODD_PARITY;
            break;
        case BBAI64_UART_PARITY_EVEN:
            *parity_flag = UART_EVEN_PARITY;
            break;
        default:
            return false;
    }

    return true;
}

static bool bbai64_uart_config_is_valid(const bbai64_uart_config_t *config)
{
    uint32_t word_length_stop_bits = 0;
    uint32_t parity_flag = 0;

    if (config == NULL) {
        return false;
    }

    if (bbai64_uart_base_addr_to_module(config->base_addr) == BBAI64_UART_MODULE_INVALID) {
        return false;
    }

    if ((config->baud_rate == 0U) || (config->module_input_clock_hz == 0U)) {
        return false;
    }

    return bbai64_uart_build_line_config(config, &word_length_stop_bits, &parity_flag);
}

static bool bbai64_uart_handle_is_valid(const bbai64_uart_t *uart)
{
    if ((uart == NULL) || !uart->is_initialized) {
        return false;
    }

    return bbai64_uart_config_is_valid(&uart->config);
}

/*
 * Advance a streaming marker match by one byte while preserving any suffix
 * that is also a prefix of the marker. This lets the receiver resync on
 * overlapping patterns such as 0xAA 0xAA 0x55 without rewinding or buffering
 * the entire incoming stream.
 */
static uint8_t bbai64_uart_next_marker_match_length(const uint8_t *marker,
                                                    uint8_t marker_length,
                                                    uint8_t matched_length,
                                                    uint8_t byte)
{
    uint8_t max_candidate;
    uint8_t sequence_length;

    if ((marker == NULL) || (marker_length == 0U)) {
        return 0U;
    }

    if (matched_length > marker_length) {
        matched_length = marker_length;
    }

    sequence_length = matched_length + 1U;
    max_candidate = (sequence_length < marker_length) ? sequence_length : marker_length;

    for (uint8_t candidate = max_candidate; candidate > 0U; candidate--) {
        const uint8_t suffix_start = sequence_length - candidate;
        bool matches = true;

        for (uint8_t index = 0U; index < candidate; index++) {
            const uint8_t sequence_index = suffix_start + index;
            const uint8_t sequence_byte = (sequence_index < matched_length) ?
                                          marker[sequence_index] :
                                          byte;

            if (sequence_byte != marker[index]) {
                matches = false;
                break;
            }
        }

        if (matches) {
            return candidate;
        }
    }

    return 0U;
}

static uint8_t bbai64_uart_frame_length_size_bytes(bbai64_uart_frame_length_t length_type)
{
    switch (length_type) {
        case BBAI64_UART_FRAME_LENGTH_U8:
            return 1U;
        case BBAI64_UART_FRAME_LENGTH_U16_LE:
        case BBAI64_UART_FRAME_LENGTH_U16_BE:
            return 2U;
        default:
            return 0U;
    }
}

static uint8_t bbai64_uart_frame_checksum_size_bytes(bbai64_uart_frame_checksum_t checksum_type)
{
    switch (checksum_type) {
        case BBAI64_UART_FRAME_CHECKSUM_NONE:
            return 0U;
        case BBAI64_UART_FRAME_CHECKSUM_XOR8:
        case BBAI64_UART_FRAME_CHECKSUM_SUM8:
            return 1U;
        case BBAI64_UART_FRAME_CHECKSUM_CRC16_IBM:
        case BBAI64_UART_FRAME_CHECKSUM_CRC16_CCITT_FALSE:
            return 2U;
        default:
            return 0U;
    }
}

static bool bbai64_uart_frame_config_is_valid(const bbai64_uart_frame_config_t *config)
{
    const uint8_t valid_scope_mask = BBAI64_UART_FRAME_CHECKSUM_ON_HEADER |
                                     BBAI64_UART_FRAME_CHECKSUM_ON_LENGTH |
                                     BBAI64_UART_FRAME_CHECKSUM_ON_PAYLOAD;

    if (config == NULL) {
        return false;
    }

    if ((config->start_bytes_length > BBAI64_UART_FRAME_MAX_MARKER_BYTES) ||
        (config->end_bytes_length > BBAI64_UART_FRAME_MAX_MARKER_BYTES)) {
        return false;
    }

    if (bbai64_uart_frame_length_size_bytes(config->length_type) == 0U) {
        return false;
    }

    if ((config->checksum_scope_flags & ~valid_scope_mask) != 0U) {
        return false;
    }

    if ((config->checksum_type == BBAI64_UART_FRAME_CHECKSUM_NONE) &&
        (config->checksum_scope_flags != 0U)) {
        return false;
    }

    if ((config->checksum_type != BBAI64_UART_FRAME_CHECKSUM_NONE) &&
        (config->checksum_scope_flags == 0U)) {
        return false;
    }

    return (bbai64_uart_frame_checksum_size_bytes(config->checksum_type) != 0U) ||
           (config->checksum_type == BBAI64_UART_FRAME_CHECKSUM_NONE);
}

/*
 * Convert API payload length into the protocol's on-wire length field.
 * length_adjustment covers protocols whose length includes extra bytes
 * beyond the payload, or excludes bytes the caller still wants treated as
 * payload by the rest of this API.
 */
static bool bbai64_uart_frame_encode_length(const bbai64_uart_frame_config_t *config,
                                            uint32_t payload_length,
                                            uint8_t *length_bytes,
                                            uint8_t *length_size,
                                            uint32_t *encoded_length)
{
    int64_t adjusted_length;

    if ((config == NULL) || (length_bytes == NULL) || (length_size == NULL) ||
        (encoded_length == NULL) || !bbai64_uart_frame_config_is_valid(config)) {
        return false;
    }

    adjusted_length = (int64_t)payload_length + (int64_t)config->length_adjustment;
    if (adjusted_length < 0) {
        return false;
    }

    *length_size = bbai64_uart_frame_length_size_bytes(config->length_type);
    *encoded_length = (uint32_t)adjusted_length;

    switch (config->length_type) {
        case BBAI64_UART_FRAME_LENGTH_U8:
            if (adjusted_length > 0xFF) {
                return false;
            }
            length_bytes[0] = (uint8_t)adjusted_length;
            return true;
        case BBAI64_UART_FRAME_LENGTH_U16_LE:
            if (adjusted_length > 0xFFFF) {
                return false;
            }
            length_bytes[0] = (uint8_t)(adjusted_length & 0xFF);
            length_bytes[1] = (uint8_t)((adjusted_length >> 8) & 0xFF);
            return true;
        case BBAI64_UART_FRAME_LENGTH_U16_BE:
            if (adjusted_length > 0xFFFF) {
                return false;
            }
            length_bytes[0] = (uint8_t)((adjusted_length >> 8) & 0xFF);
            length_bytes[1] = (uint8_t)(adjusted_length & 0xFF);
            return true;
        default:
            return false;
    }
}

/*
 * Inverse of bbai64_uart_frame_encode_length(). Once the wire length has
 * been parsed, translate it back into the payload byte count expected by
 * the rest of the read path.
 */
static bool bbai64_uart_frame_decode_payload_length(const bbai64_uart_frame_config_t *config,
                                                    const uint8_t *length_bytes,
                                                    uint32_t *payload_length)
{
    int64_t decoded_payload_length;
    uint32_t encoded_length = 0U;

    if ((config == NULL) || (length_bytes == NULL) || (payload_length == NULL) ||
        !bbai64_uart_frame_config_is_valid(config)) {
        return false;
    }

    switch (config->length_type) {
        case BBAI64_UART_FRAME_LENGTH_U8:
            encoded_length = length_bytes[0];
            break;
        case BBAI64_UART_FRAME_LENGTH_U16_LE:
            encoded_length = (uint32_t)length_bytes[0] | ((uint32_t)length_bytes[1] << 8);
            break;
        case BBAI64_UART_FRAME_LENGTH_U16_BE:
            encoded_length = ((uint32_t)length_bytes[0] << 8) | (uint32_t)length_bytes[1];
            break;
        default:
            return false;
    }

    decoded_payload_length = (int64_t)encoded_length - (int64_t)config->length_adjustment;
    if (decoded_payload_length < 0) {
        return false;
    }

    *payload_length = (uint32_t)decoded_payload_length;
    return true;
}

static void bbai64_uart_checksum_state_init(bbai64_uart_checksum_state_t *state,
                                            bbai64_uart_frame_checksum_t type)
{
    if (state == NULL) {
        return;
    }

    state->type = type;
    state->value = 0U;

    if (type == BBAI64_UART_FRAME_CHECKSUM_CRC16_CCITT_FALSE) {
        state->value = 0xFFFFU;
    }
}

/*
 * Feed bytes through the selected checksum engine in wire order. The framing
 * layer decides which sections participate; this helper only owns the
 * algorithm-specific state update.
 */
static void bbai64_uart_checksum_state_update(bbai64_uart_checksum_state_t *state,
                                              const uint8_t *data,
                                              uint32_t length)
{
    if ((state == NULL) || (data == NULL)) {
        return;
    }

    for (uint32_t index = 0; index < length; index++) {
        switch (state->type) {
            case BBAI64_UART_FRAME_CHECKSUM_NONE:
                return;
            case BBAI64_UART_FRAME_CHECKSUM_XOR8:
                state->value = (uint8_t)(state->value ^ data[index]);
                break;
            case BBAI64_UART_FRAME_CHECKSUM_SUM8:
                state->value = (uint8_t)(state->value + data[index]);
                break;
            case BBAI64_UART_FRAME_CHECKSUM_CRC16_IBM:
                state->value ^= data[index];
                for (uint8_t bit = 0; bit < 8U; bit++) {
                    if ((state->value & 0x0001U) != 0U) {
                        state->value = (uint16_t)((state->value >> 1) ^ 0xA001U);
                    } else {
                        state->value >>= 1;
                    }
                }
                break;
            case BBAI64_UART_FRAME_CHECKSUM_CRC16_CCITT_FALSE:
                state->value ^= (uint16_t)((uint16_t)data[index] << 8);
                for (uint8_t bit = 0; bit < 8U; bit++) {
                    if ((state->value & 0x8000U) != 0U) {
                        state->value = (uint16_t)((state->value << 1) ^ 0x1021U);
                    } else {
                        state->value <<= 1;
                    }
                }
                break;
            default:
                return;
        }
    }
}

/*
 * Build the checksum exactly as the peer sees the frame on the wire.
 * Scope flags let protocols include any combination of header, length, and
 * payload without duplicating checksum logic in the read and write paths.
 */
static void bbai64_uart_frame_compute_checksum(const bbai64_uart_frame_config_t *config,
                                               const uint8_t *length_bytes,
                                               uint8_t length_size,
                                               const uint8_t *payload,
                                               uint32_t payload_length,
                                               uint8_t *checksum_bytes)
{
    bbai64_uart_checksum_state_t checksum_state;
    uint16_t checksum_value;

    if ((config == NULL) || (checksum_bytes == NULL)) {
        return;
    }

    checksum_bytes[0] = 0U;
    checksum_bytes[1] = 0U;

    bbai64_uart_checksum_state_init(&checksum_state, config->checksum_type);

    if ((config->checksum_scope_flags & BBAI64_UART_FRAME_CHECKSUM_ON_HEADER) != 0U) {
        bbai64_uart_checksum_state_update(&checksum_state,
                                          config->start_bytes,
                                          config->start_bytes_length);
    }

    if ((config->checksum_scope_flags & BBAI64_UART_FRAME_CHECKSUM_ON_LENGTH) != 0U) {
        bbai64_uart_checksum_state_update(&checksum_state, length_bytes, length_size);
    }

    if (((config->checksum_scope_flags & BBAI64_UART_FRAME_CHECKSUM_ON_PAYLOAD) != 0U) &&
        (payload_length != 0U)) {
        bbai64_uart_checksum_state_update(&checksum_state, payload, payload_length);
    }

    checksum_value = checksum_state.value;
    switch (config->checksum_type) {
        case BBAI64_UART_FRAME_CHECKSUM_NONE:
            break;
        case BBAI64_UART_FRAME_CHECKSUM_XOR8:
        case BBAI64_UART_FRAME_CHECKSUM_SUM8:
            checksum_bytes[0] = (uint8_t)checksum_value;
            break;
        case BBAI64_UART_FRAME_CHECKSUM_CRC16_IBM:
        case BBAI64_UART_FRAME_CHECKSUM_CRC16_CCITT_FALSE:
            if (config->checksum_big_endian) {
                checksum_bytes[0] = (uint8_t)((checksum_value >> 8) & 0xFF);
                checksum_bytes[1] = (uint8_t)(checksum_value & 0xFF);
            } else {
                checksum_bytes[0] = (uint8_t)(checksum_value & 0xFF);
                checksum_bytes[1] = (uint8_t)((checksum_value >> 8) & 0xFF);
            }
            break;
        default:
            break;
    }
}

static bool bbai64_uart_read_byte_before_deadline(const bbai64_uart_t *uart,
                                                  uint8_t *byte,
                                                  uint64_t deadline_ticks)
{
    if ((byte == NULL) || !bbai64_uart_handle_is_valid(uart)) {
        return false;
    }

    while (get_current_ticks() < deadline_ticks) {
        if (!bbai64_uart_rx_ready(uart)) {
            continue;
        }

        *byte = (uint8_t)UARTCharGet(uart->config.base_addr);
        return true;
    }

    return false;
}

static bool bbai64_uart_read_bytes_before_deadline(const bbai64_uart_t *uart,
                                                   uint8_t *buffer,
                                                   uint32_t length,
                                                   uint64_t deadline_ticks)
{
    if ((buffer == NULL) && (length != 0U)) {
        return false;
    }

    for (uint32_t index = 0; index < length; index++) {
        if (!bbai64_uart_read_byte_before_deadline(uart, &buffer[index], deadline_ticks)) {
            return false;
        }
    }

    return true;
}

static bool bbai64_uart_discard_bytes_before_deadline(const bbai64_uart_t *uart,
                                                      uint32_t length,
                                                      uint64_t deadline_ticks)
{
    uint8_t throwaway;

    for (uint32_t index = 0; index < length; index++) {
        if (!bbai64_uart_read_byte_before_deadline(uart, &throwaway, deadline_ticks)) {
            return false;
        }
    }

    return true;
}

/*
 * Scan until the full start marker appears or the deadline expires.
 * Non-matching bytes are treated as noise or partial frames and discarded so
 * the parser can recover even when it starts mid-stream.
 */
static bool bbai64_uart_read_start_marker_before_deadline(const bbai64_uart_t *uart,
                                                          const bbai64_uart_frame_config_t *config,
                                                          uint64_t deadline_ticks)
{
    uint8_t matched = 0U;
    uint8_t byte;

    if (config->start_bytes_length == 0U) {
        return true;
    }

    while (matched < config->start_bytes_length) {
        if (!bbai64_uart_read_byte_before_deadline(uart, &byte, deadline_ticks)) {
            return false;
        }

        matched = bbai64_uart_next_marker_match_length(config->start_bytes,
                                                       config->start_bytes_length,
                                                       matched,
                                                       byte);
    }

    return true;
}

uint32_t bbai64_uart_module_to_base_addr(bbai64_uart_module_t module)
{
    switch (module) {
        case BBAI64_UART_MODULE_0:
            return BBAI64_UART0_BASE;
        case BBAI64_UART_MODULE_1:
            return BBAI64_UART1_BASE;
        case BBAI64_UART_MODULE_2:
            return BBAI64_UART2_BASE;
        case BBAI64_UART_MODULE_3:
            return BBAI64_UART3_BASE;
        case BBAI64_UART_MODULE_4:
            return BBAI64_UART4_BASE;
        case BBAI64_UART_MODULE_5:
            return BBAI64_UART5_BASE;
        case BBAI64_UART_MODULE_6:
            return BBAI64_UART6_BASE;
        case BBAI64_UART_MODULE_7:
            return BBAI64_UART7_BASE;
        case BBAI64_UART_MODULE_8:
            return BBAI64_UART8_BASE;
        case BBAI64_UART_MODULE_9:
            return BBAI64_UART9_BASE;
        default:
            return 0U;
    }
}

bbai64_uart_module_t bbai64_uart_base_addr_to_module(uint32_t base_addr)
{
    switch (base_addr) {
        case BBAI64_UART0_BASE:
            return BBAI64_UART_MODULE_0;
        case BBAI64_UART1_BASE:
            return BBAI64_UART_MODULE_1;
        case BBAI64_UART2_BASE:
            return BBAI64_UART_MODULE_2;
        case BBAI64_UART3_BASE:
            return BBAI64_UART_MODULE_3;
        case BBAI64_UART4_BASE:
            return BBAI64_UART_MODULE_4;
        case BBAI64_UART5_BASE:
            return BBAI64_UART_MODULE_5;
        case BBAI64_UART6_BASE:
            return BBAI64_UART_MODULE_6;
        case BBAI64_UART7_BASE:
            return BBAI64_UART_MODULE_7;
        case BBAI64_UART8_BASE:
            return BBAI64_UART_MODULE_8;
        case BBAI64_UART9_BASE:
            return BBAI64_UART_MODULE_9;
        default:
            return BBAI64_UART_MODULE_INVALID;
    }
}

const char *bbai64_uart_module_name(bbai64_uart_module_t module)
{
    switch (module) {
        case BBAI64_UART_MODULE_0:
            return "UART0";
        case BBAI64_UART_MODULE_1:
            return "UART1";
        case BBAI64_UART_MODULE_2:
            return "UART2";
        case BBAI64_UART_MODULE_3:
            return "UART3";
        case BBAI64_UART_MODULE_4:
            return "UART4";
        case BBAI64_UART_MODULE_5:
            return "UART5";
        case BBAI64_UART_MODULE_6:
            return "UART6";
        case BBAI64_UART_MODULE_7:
            return "UART7";
        case BBAI64_UART_MODULE_8:
            return "UART8";
        case BBAI64_UART_MODULE_9:
            return "UART9";
        default:
            return "UART_INVALID";
    }
}

void bbai64_uart_config_init(bbai64_uart_config_t *config, bbai64_uart_module_t module)
{
    if (config == NULL) {
        return;
    }

    memset(config, 0, sizeof(*config));
    config->base_addr = bbai64_uart_module_to_base_addr(module);
    config->baud_rate = BBAI64_UART_DEFAULT_BAUD_RATE;
    config->module_input_clock_hz = BBAI64_UART_MODULE_CLOCK_HZ;
    config->data_bits = 8U;
    config->stop_bits = 1U;
    config->parity = BBAI64_UART_PARITY_NONE;
}

bool bbai64_uart_config_init_from_base_addr(bbai64_uart_config_t *config, uint32_t base_addr)
{
    bbai64_uart_module_t module = bbai64_uart_base_addr_to_module(base_addr);

    if (config == NULL) {
        return false;
    }

    if (module == BBAI64_UART_MODULE_INVALID) {
        return false;
    }

    bbai64_uart_config_init(config, module);
    config->base_addr = base_addr;
    return true;
}

bool bbai64_uart_init(bbai64_uart_t *uart, const bbai64_uart_config_t *config)
{
    uint32_t divisor_value;
    uint32_t word_length_stop_bits = 0;
    uint32_t parity_flag = 0;
    uint32_t fifo_config;
    uint32_t operating_mode;

    if (uart == NULL) {
        return false;
    }

    bbai64_uart_deinit(uart);

    if (!bbai64_uart_config_is_valid(config)) {
        return false;
    }

    operating_mode = bbai64_uart_select_operating_mode(config->baud_rate);
    divisor_value = UARTDivisorValCompute(config->module_input_clock_hz,
                                          config->baud_rate,
                                          operating_mode,
                                          0U);

    if (!bbai64_uart_build_line_config(config, &word_length_stop_bits, &parity_flag)) {
        return false;
    }

    UARTModuleReset(config->base_addr);
    UARTOperatingModeSelect(config->base_addr, operating_mode);
    UARTDivisorLatchEnable(config->base_addr);
    UARTDivisorLatchWrite(config->base_addr, divisor_value);
    UARTDivisorLatchDisable(config->base_addr);
    UARTLineCharacConfig(config->base_addr, word_length_stop_bits, parity_flag);

    fifo_config = UART_FIFO_CONFIG(
        UART_TRIG_LVL_GRANULARITY_1,
        UART_TRIG_LVL_GRANULARITY_1,
        0,
        0,
        0,
        0,
        UART_DMA_EN_PATH_SCR,
        UART_DMA_MODE_0_ENABLE
    );
    UARTFIFOConfig(config->base_addr, fifo_config);
    UARTOperatingModeSelect(config->base_addr, operating_mode);

    uart->config = *config;
    uart->is_initialized = true;
    return true;
}

void bbai64_uart_deinit(bbai64_uart_t *uart)
{
    if (uart == NULL) {
        return;
    }

    memset(&uart->config, 0, sizeof(uart->config));
    uart->is_initialized = false;
}

bool bbai64_uart_is_initialized(const bbai64_uart_t *uart)
{
    return bbai64_uart_handle_is_valid(uart);
}

bool bbai64_uart_tx_ready(const bbai64_uart_t *uart)
{
    if (!bbai64_uart_handle_is_valid(uart)) {
        return false;
    }

    return (UARTSpaceAvail(uart->config.base_addr) != 0U);
}

bool bbai64_uart_rx_ready(const bbai64_uart_t *uart)
{
    if (!bbai64_uart_handle_is_valid(uart)) {
        return false;
    }

    return (UARTCharsAvail(uart->config.base_addr) != 0U);
}

void bbai64_uart_drain_rx(const bbai64_uart_t *uart)
{
    if (!bbai64_uart_handle_is_valid(uart)) {
        return;
    }

    while (bbai64_uart_rx_ready(uart)) {
        (void)UARTCharGet(uart->config.base_addr);
    }
}

bool bbai64_uart_write_byte(const bbai64_uart_t *uart, uint8_t byte)
{
    if (!bbai64_uart_handle_is_valid(uart)) {
        return false;
    }

    while (!bbai64_uart_tx_ready(uart));
    UARTCharPut(uart->config.base_addr, byte);
    return true;
}

bool bbai64_uart_write(const bbai64_uart_t *uart, const uint8_t *data, uint32_t length)
{
    if (!bbai64_uart_handle_is_valid(uart)) {
        return false;
    }

    if ((data == NULL) && (length != 0U)) {
        return false;
    }

    for (uint32_t index = 0; index < length; index++) {
        if (!bbai64_uart_write_byte(uart, data[index])) {
            return false;
        }
    }

    return true;
}

bool bbai64_uart_write_cstr(const bbai64_uart_t *uart, const char *text)
{
    if (text == NULL) {
        return false;
    }

    return bbai64_uart_write(uart, (const uint8_t *)text, (uint32_t)strlen(text));
}

bool bbai64_uart_read_byte_nonblocking(const bbai64_uart_t *uart, uint8_t *byte)
{
    if (!bbai64_uart_handle_is_valid(uart) || (byte == NULL) || !bbai64_uart_rx_ready(uart)) {
        return false;
    }

    *byte = (uint8_t)UARTCharGet(uart->config.base_addr);
    return true;
}

uint32_t bbai64_uart_read_available(const bbai64_uart_t *uart, uint8_t *buffer, uint32_t buffer_length)
{
    uint32_t count = 0;

    if (!bbai64_uart_handle_is_valid(uart)) {
        return 0U;
    }

    if ((buffer == NULL) && (buffer_length != 0U)) {
        return 0U;
    }

    while ((count < buffer_length) && bbai64_uart_read_byte_nonblocking(uart, &buffer[count])) {
        count++;
    }

    return count;
}

uint32_t bbai64_uart_read_blocking(const bbai64_uart_t *uart, uint8_t *buffer, uint32_t length)
{
    uint32_t count = 0;

    if (!bbai64_uart_handle_is_valid(uart)) {
        return 0U;
    }

    if ((buffer == NULL) && (length != 0U)) {
        return 0U;
    }

    while (count < length) {
        while (!bbai64_uart_rx_ready(uart));
        buffer[count++] = (uint8_t)UARTCharGet(uart->config.base_addr);
    }

    return count;
}

uint32_t bbai64_uart_read_with_timeout(const bbai64_uart_t *uart, uint8_t *buffer, uint32_t length, uint64_t timeout_us)
{
    uint32_t count = 0;
    const uint64_t deadline_ticks = get_current_ticks() + dmicroseconds_to_ticks(timeout_us);

    if (!bbai64_uart_handle_is_valid(uart)) {
        return 0U;
    }

    if ((buffer == NULL) && (length != 0U)) {
        return 0U;
    }

    while ((count < length) && (get_current_ticks() < deadline_ticks)) {
        if (!bbai64_uart_rx_ready(uart)) {
            continue;
        }

        buffer[count++] = (uint8_t)UARTCharGet(uart->config.base_addr);
    }

    return count;
}

void bbai64_uart_frame_config_init(bbai64_uart_frame_config_t *config)
{
    if (config == NULL) {
        return;
    }

    memset(config, 0, sizeof(*config));
    config->start_bytes[0] = 0xAAU;
    config->start_bytes[1] = 0x55U;
    config->start_bytes_length = 2U;
    config->length_type = BBAI64_UART_FRAME_LENGTH_U16_LE;
    config->checksum_type = BBAI64_UART_FRAME_CHECKSUM_NONE;
    config->checksum_scope_flags = 0U;
    config->checksum_big_endian = false;
    config->length_adjustment = 0;
}

/*
 * Return total bytes placed on the wire for a payload of this size.
 * The adjusted length only changes the numeric value written into the length
 * field. The frame still carries the original payload bytes, so the byte count
 * here continues to use payload_length.
 */
uint32_t bbai64_uart_frame_encoded_size(const bbai64_uart_frame_config_t *config, uint32_t payload_length)
{
    uint8_t length_bytes[2];
    uint8_t length_size;
    uint32_t encoded_length;

    if (!bbai64_uart_frame_encode_length(config,
                                         payload_length,
                                         length_bytes,
                                         &length_size,
                                         &encoded_length)) {
        return 0U;
    }

    (void)length_bytes;
    (void)encoded_length;

    return (uint32_t)config->start_bytes_length +
           (uint32_t)length_size +
           payload_length +
           (uint32_t)bbai64_uart_frame_checksum_size_bytes(config->checksum_type) +
           (uint32_t)config->end_bytes_length;
}

/*
 * Serialize the frame directly to UART instead of building a temporary
 * contiguous packet buffer. That keeps stack usage predictable on the R5 while
 * still supporting configurable framing layouts.
 */
bool bbai64_uart_write_frame(const bbai64_uart_t *uart, const bbai64_uart_frame_config_t *frame_config,
                             const uint8_t *payload, uint32_t payload_length)
{
    uint8_t length_bytes[2];
    uint8_t checksum_bytes[2] = {0U, 0U};
    uint8_t length_size;
    uint32_t encoded_length;

    if (!bbai64_uart_handle_is_valid(uart) || !bbai64_uart_frame_config_is_valid(frame_config)) {
        return false;
    }

    if ((payload == NULL) && (payload_length != 0U)) {
        return false;
    }

    if (!bbai64_uart_frame_encode_length(frame_config,
                                         payload_length,
                                         length_bytes,
                                         &length_size,
                                         &encoded_length)) {
        return false;
    }

    (void)encoded_length;
    bbai64_uart_frame_compute_checksum(frame_config,
                                       length_bytes,
                                       length_size,
                                       payload,
                                       payload_length,
                                       checksum_bytes);

    return bbai64_uart_write(uart, frame_config->start_bytes, frame_config->start_bytes_length) &&
           bbai64_uart_write(uart, length_bytes, length_size) &&
           bbai64_uart_write(uart, payload, payload_length) &&
           bbai64_uart_write(uart,
                             checksum_bytes,
                             bbai64_uart_frame_checksum_size_bytes(frame_config->checksum_type)) &&
           bbai64_uart_write(uart, frame_config->end_bytes, frame_config->end_bytes_length);
}

/*
 * Parse exactly one framed packet from the byte stream. The routine is
 * intentionally strict: it resynchronizes on the start marker, validates the
 * encoded length, drains oversized frames to preserve alignment, and then
 * checks both the end marker and checksum before reporting success.
 */
bbai64_uart_frame_read_status_t bbai64_uart_read_frame(const bbai64_uart_t *uart,
                                                       const bbai64_uart_frame_config_t *frame_config,
                                                       uint8_t *payload,
                                                       uint32_t payload_buffer_length,
                                                       uint32_t *payload_length,
                                                       uint64_t timeout_us)
{
    uint8_t length_bytes[2];
    uint8_t checksum_bytes[2] = {0U, 0U};
    uint8_t expected_checksum_bytes[2] = {0U, 0U};
    uint8_t end_bytes[BBAI64_UART_FRAME_MAX_MARKER_BYTES];
    uint8_t length_size;
    uint8_t checksum_size;
    uint32_t frame_payload_length;
    uint64_t deadline_ticks;

    if (payload_length != NULL) {
        *payload_length = 0U;
    }

    if (!bbai64_uart_handle_is_valid(uart) || (payload_length == NULL)) {
        return BBAI64_UART_FRAME_READ_INVALID_ARGUMENT;
    }

    if (!bbai64_uart_frame_config_is_valid(frame_config)) {
        return BBAI64_UART_FRAME_READ_INVALID_CONFIG;
    }

    deadline_ticks = get_current_ticks() + dmicroseconds_to_ticks(timeout_us);
    length_size = bbai64_uart_frame_length_size_bytes(frame_config->length_type);
    checksum_size = bbai64_uart_frame_checksum_size_bytes(frame_config->checksum_type);

    if (!bbai64_uart_read_start_marker_before_deadline(uart, frame_config, deadline_ticks)) {
        return BBAI64_UART_FRAME_READ_TIMEOUT;
    }

    if (!bbai64_uart_read_bytes_before_deadline(uart, length_bytes, length_size, deadline_ticks)) {
        return BBAI64_UART_FRAME_READ_TIMEOUT;
    }

    if (!bbai64_uart_frame_decode_payload_length(frame_config, length_bytes, &frame_payload_length)) {
        return BBAI64_UART_FRAME_READ_LENGTH_OUT_OF_RANGE;
    }

    if ((frame_payload_length > 0U) && ((payload == NULL) || (frame_payload_length > payload_buffer_length))) {
        *payload_length = frame_payload_length;

        /*
         * Consume the rest of the current frame before returning so the next
         * read starts at the following frame boundary instead of in the middle
         * of this oversized payload.
         */
        if (!bbai64_uart_discard_bytes_before_deadline(uart,
                                                       frame_payload_length + (uint32_t)checksum_size +
                                                           (uint32_t)frame_config->end_bytes_length,
                                                       deadline_ticks)) {
            return BBAI64_UART_FRAME_READ_TIMEOUT;
        }

        return BBAI64_UART_FRAME_READ_BUFFER_TOO_SMALL;
    }

    if (!bbai64_uart_read_bytes_before_deadline(uart, payload, frame_payload_length, deadline_ticks)) {
        return BBAI64_UART_FRAME_READ_TIMEOUT;
    }

    if (!bbai64_uart_read_bytes_before_deadline(uart, checksum_bytes, checksum_size, deadline_ticks)) {
        return BBAI64_UART_FRAME_READ_TIMEOUT;
    }

    if (!bbai64_uart_read_bytes_before_deadline(uart,
                                                end_bytes,
                                                frame_config->end_bytes_length,
                                                deadline_ticks)) {
        return BBAI64_UART_FRAME_READ_TIMEOUT;
    }

    if ((frame_config->end_bytes_length != 0U) &&
        (memcmp(end_bytes, frame_config->end_bytes, frame_config->end_bytes_length) != 0)) {
        return BBAI64_UART_FRAME_READ_END_MARKER_MISMATCH;
    }

    bbai64_uart_frame_compute_checksum(frame_config,
                                       length_bytes,
                                       length_size,
                                       payload,
                                       frame_payload_length,
                                       expected_checksum_bytes);
    if ((checksum_size != 0U) && (memcmp(checksum_bytes, expected_checksum_bytes, checksum_size) != 0)) {
        return BBAI64_UART_FRAME_READ_CHECKSUM_MISMATCH;
    }

    *payload_length = frame_payload_length;
    return BBAI64_UART_FRAME_READ_OK;
}

const char *bbai64_uart_frame_read_status_name(bbai64_uart_frame_read_status_t status)
{
    switch (status) {
        case BBAI64_UART_FRAME_READ_OK:
            return "FRAME_READ_OK";
        case BBAI64_UART_FRAME_READ_TIMEOUT:
            return "FRAME_READ_TIMEOUT";
        case BBAI64_UART_FRAME_READ_INVALID_ARGUMENT:
            return "FRAME_READ_INVALID_ARGUMENT";
        case BBAI64_UART_FRAME_READ_INVALID_CONFIG:
            return "FRAME_READ_INVALID_CONFIG";
        case BBAI64_UART_FRAME_READ_LENGTH_OUT_OF_RANGE:
            return "FRAME_READ_LENGTH_OUT_OF_RANGE";
        case BBAI64_UART_FRAME_READ_BUFFER_TOO_SMALL:
            return "FRAME_READ_BUFFER_TOO_SMALL";
        case BBAI64_UART_FRAME_READ_END_MARKER_MISMATCH:
            return "FRAME_READ_END_MARKER_MISMATCH";
        case BBAI64_UART_FRAME_READ_CHECKSUM_MISMATCH:
            return "FRAME_READ_CHECKSUM_MISMATCH";
        default:
            return "FRAME_READ_UNKNOWN";
    }
}