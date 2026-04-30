#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

#include <ai64/bbai64_uart.h>
#include <io_test_functions/uart_tests.h>

#define UART_TEST_BAUD_RATE  19200U
#define UART_RX_TIMEOUT_US   500000U
#define UART_FRAME_TEST_RX_BUFFER_SIZE 64U

static const char *test_frame_checksum_name(bbai64_uart_frame_checksum_t checksumType)
{
    switch (checksumType) {
        case BBAI64_UART_FRAME_CHECKSUM_NONE:
            return "NONE";
        case BBAI64_UART_FRAME_CHECKSUM_XOR8:
            return "XOR8";
        case BBAI64_UART_FRAME_CHECKSUM_SUM8:
            return "SUM8";
        case BBAI64_UART_FRAME_CHECKSUM_CRC16_IBM:
            return "CRC16_IBM";
        case BBAI64_UART_FRAME_CHECKSUM_CRC16_CCITT_FALSE:
            return "CRC16_CCITT_FALSE";
        default:
            return "UNKNOWN_CHECKSUM";
    }
}

static const char *test_frame_length_name(bbai64_uart_frame_length_t lengthType)
{
    switch (lengthType) {
        case BBAI64_UART_FRAME_LENGTH_U8:
            return "U8";
        case BBAI64_UART_FRAME_LENGTH_U16_LE:
            return "U16_LE";
        case BBAI64_UART_FRAME_LENGTH_U16_BE:
            return "U16_BE";
        default:
            return "UNKNOWN_LENGTH";
    }
}

static bool run_framed_packet_case(const bbai64_uart_t *txUart,
                                   const bbai64_uart_t *rxUart,
                                   const char *testName,
                                   const bbai64_uart_frame_config_t *frameConfig,
                                   const uint8_t *txPayload,
                                   uint32_t txPayloadLength)
{
    uint8_t rxPayload[UART_FRAME_TEST_RX_BUFFER_SIZE] = {0};
    uint32_t receivedLength = 0U;
    uint32_t encodedSize;
    bbai64_uart_frame_read_status_t readStatus;

    encodedSize = bbai64_uart_frame_encoded_size(frameConfig, txPayloadLength);
    printf("UART framed packet test: %s\n", testName);
    printf("  start_bytes=%u end_bytes=%u length=%s checksum=%s encoded_bytes=%lu\n",
           frameConfig->start_bytes_length,
           frameConfig->end_bytes_length,
           test_frame_length_name(frameConfig->length_type),
           test_frame_checksum_name(frameConfig->checksum_type),
           (unsigned long)encodedSize);

    bbai64_uart_drain_rx(rxUart);
    if (!bbai64_uart_write_frame(txUart, frameConfig, txPayload, txPayloadLength)) {
        printf("  write_frame failed\n");
        return false;
    }

    readStatus = bbai64_uart_read_frame(rxUart,
                                        frameConfig,
                                        rxPayload,
                                        sizeof(rxPayload),
                                        &receivedLength,
                                        UART_RX_TIMEOUT_US);
    printf("  read_frame status=%s payload_length=%lu\n",
           bbai64_uart_frame_read_status_name(readStatus),
           (unsigned long)receivedLength);

    if ((readStatus != BBAI64_UART_FRAME_READ_OK) ||
        (receivedLength != txPayloadLength) ||
        (memcmp(txPayload, rxPayload, txPayloadLength) != 0)) {
        printf("  framed packet test failed\n");
        return false;
    }

    printf("  framed packet test passed\n");
    return true;
}

static void test_uart_framed_packets(const bbai64_uart_t *txUart, const bbai64_uart_t *rxUart)
{
    uint32_t passedCount = 0U;
    uint32_t totalCount = 0U;
    bbai64_uart_frame_config_t frameConfig;

    printf("UART framed packet capability tests\n");

    /*
     * Test 1: default framed transport.
     *
     * This is now the simplest default: 0xAA 0x55 start marker, 16-bit
     * little-endian payload length, and no checksum at all.
     */
    {
        static const uint8_t txPayload[] = {0x01U, 0x03U, 0x20U, 0xAAU, 0x55U};

        bbai64_uart_frame_config_init(&frameConfig);
        totalCount++;
        if (run_framed_packet_case(txUart,
                                   rxUart,
                                   "default no-checksum frame",
                                   &frameConfig,
                                   txPayload,
                                   sizeof(txPayload))) {
            passedCount++;
        }
    }

    /*
     * Test 2: small-packet format with an 8-bit XOR checksum.
     *
     * This mirrors simple UART peripherals where the frame is short, the
     * length is a single byte, and integrity is checked only across the payload.
     */
    {
        static const uint8_t txPayload[] = {0x10U, 0x20U, 0x30U, 0x40U};

        bbai64_uart_frame_config_init(&frameConfig);
        frameConfig.start_bytes[0] = 0x5AU;
        frameConfig.start_bytes[1] = 0xA5U;
        frameConfig.start_bytes_length = 2U;
        frameConfig.length_type = BBAI64_UART_FRAME_LENGTH_U8;
        frameConfig.checksum_type = BBAI64_UART_FRAME_CHECKSUM_XOR8;
        frameConfig.checksum_scope_flags = BBAI64_UART_FRAME_CHECKSUM_ON_PAYLOAD;

        totalCount++;
        if (run_framed_packet_case(txUart,
                                   rxUart,
                                   "u8 length + XOR8 payload checksum",
                                   &frameConfig,
                                   txPayload,
                                   sizeof(txPayload))) {
            passedCount++;
        }
    }

    /*
     * Test 3: SUM8 checksum plus an end marker.
     *
     * This demonstrates a protocol that uses a single-byte length, a trailing
     * terminator, and a lightweight checksum over the length and payload.
     */
    {
        static const uint8_t txPayload[] = {'O', 'K', 0x10U, 0x20U};

        bbai64_uart_frame_config_init(&frameConfig);
        frameConfig.start_bytes[0] = 0x7EU;
        frameConfig.start_bytes_length = 1U;
        frameConfig.end_bytes[0] = 0x0DU;
        frameConfig.end_bytes[1] = 0x0AU;
        frameConfig.end_bytes_length = 2U;
        frameConfig.length_type = BBAI64_UART_FRAME_LENGTH_U8;
        frameConfig.checksum_type = BBAI64_UART_FRAME_CHECKSUM_SUM8;
        frameConfig.checksum_scope_flags = BBAI64_UART_FRAME_CHECKSUM_ON_LENGTH |
                                           BBAI64_UART_FRAME_CHECKSUM_ON_PAYLOAD;

        totalCount++;
        if (run_framed_packet_case(txUart,
                                   rxUart,
                                   "u8 length + SUM8 + end marker",
                                   &frameConfig,
                                   txPayload,
                                   sizeof(txPayload))) {
            passedCount++;
        }
    }

    /*
     * Test 4: extended binary header with CRC16-IBM.
     *
     * This shows a more general binary protocol: larger fixed header, 16-bit
     * little-endian length, CRC16-IBM, and a non-zero length adjustment so the
     * on-wire length does not have to equal the payload length exactly.
     */
    {
        static const uint8_t txPayload[] = {0x01U, 0x03U, 0x20U, 0x00U};

        bbai64_uart_frame_config_init(&frameConfig);
        frameConfig.start_bytes[0] = 0xFFU;
        frameConfig.start_bytes[1] = 0xFFU;
        frameConfig.start_bytes[2] = 0xFDU;
        frameConfig.start_bytes[3] = 0x00U;
        frameConfig.start_bytes_length = 4U;
        frameConfig.length_type = BBAI64_UART_FRAME_LENGTH_U16_LE;
        frameConfig.length_adjustment = 1;
        frameConfig.checksum_type = BBAI64_UART_FRAME_CHECKSUM_CRC16_IBM;
        frameConfig.checksum_scope_flags = BBAI64_UART_FRAME_CHECKSUM_ON_HEADER |
                                           BBAI64_UART_FRAME_CHECKSUM_ON_LENGTH |
                                           BBAI64_UART_FRAME_CHECKSUM_ON_PAYLOAD;

        totalCount++;
        if (run_framed_packet_case(txUart,
                                   rxUart,
                                   "extended binary header + CRC16-IBM frame",
                                   &frameConfig,
                                   txPayload,
                                   sizeof(txPayload))) {
            passedCount++;
        }
    }

    /*
     * Test 5: big-endian length and big-endian CRC16-CCITT-FALSE.
     *
     * This is useful for devices that prefer network-order framing and a CRC16
     * variant that starts from 0xFFFF instead of zero.
     */
    {
        static const uint8_t txPayload[] = {0xBEU, 0xEFU, 0x55U, 0x66U, 0x77U};

        bbai64_uart_frame_config_init(&frameConfig);
        frameConfig.start_bytes[0] = 0xC0U;
        frameConfig.start_bytes[1] = 0xDEU;
        frameConfig.start_bytes_length = 2U;
        frameConfig.length_type = BBAI64_UART_FRAME_LENGTH_U16_BE;
        frameConfig.checksum_type = BBAI64_UART_FRAME_CHECKSUM_CRC16_CCITT_FALSE;
        frameConfig.checksum_scope_flags = BBAI64_UART_FRAME_CHECKSUM_ON_HEADER |
                                           BBAI64_UART_FRAME_CHECKSUM_ON_LENGTH |
                                           BBAI64_UART_FRAME_CHECKSUM_ON_PAYLOAD;
        frameConfig.checksum_big_endian = true;

        totalCount++;
        if (run_framed_packet_case(txUart,
                                   rxUart,
                                   "u16 big-endian + CRC16-CCITT-FALSE",
                                   &frameConfig,
                                   txPayload,
                                   sizeof(txPayload))) {
            passedCount++;
        }
    }

    printf("UART framed packet summary: %lu/%lu tests passed\n",
           (unsigned long)passedCount,
           (unsigned long)totalCount);
}

void test_uart(uint32_t txBaseAddr, uint32_t rxBaseAddr)
{
    static const uint8_t txBuffer[] = "Hello UART polling!\n";
    uint8_t rxBuffer[sizeof(txBuffer)];
    uint32_t receivedCount;
    bbai64_uart_config_t txConfig;
    bbai64_uart_config_t rxConfig;
    bbai64_uart_t txUart;
    bbai64_uart_t rxUart;
    bbai64_uart_module_t txModule;
    bbai64_uart_module_t rxModule;

    memset(rxBuffer, 0, sizeof(rxBuffer));
    memset(&txUart, 0, sizeof(txUart));
    memset(&rxUart, 0, sizeof(rxUart));

    txModule = bbai64_uart_base_addr_to_module(txBaseAddr);
    rxModule = bbai64_uart_base_addr_to_module(rxBaseAddr);

    if (!bbai64_uart_config_init_from_base_addr(&txConfig, txBaseAddr) ||
        !bbai64_uart_config_init_from_base_addr(&rxConfig, rxBaseAddr)) {
        printf("UART polling test: unsupported UART base address\n");
        return;
    }

    txConfig.baud_rate = UART_TEST_BAUD_RATE;
    rxConfig.baud_rate = UART_TEST_BAUD_RATE;

    printf("UART polling test: connect P9_16 (UART6_TX) to P8_28 (UART8_RX)\n");
    printf("UART polling test: %s tx=0x%08lx %s rx=0x%08lx baud=%lu\n",
           bbai64_uart_module_name(txModule),
           (unsigned long)txConfig.base_addr,
           bbai64_uart_module_name(rxModule),
           (unsigned long)rxConfig.base_addr,
           (unsigned long)UART_TEST_BAUD_RATE);

    if (!bbai64_uart_init(&txUart, &txConfig)) {
        printf("UART polling test: failed to init TX UART\n");
        return;
    }

    if (rxBaseAddr != txBaseAddr) {
        if (!bbai64_uart_init(&rxUart, &rxConfig)) {
            printf("UART polling test: failed to init RX UART\n");
            return;
        }
    } else {
        rxUart = txUart;
    }

    bbai64_uart_drain_rx(&rxUart);
    if (!bbai64_uart_write(&txUart, txBuffer, sizeof(txBuffer) - 1U)) {
        printf("UART polling test: failed to write TX bytes\n");
        return;
    }

    receivedCount = bbai64_uart_read_with_timeout(&rxUart, rxBuffer,
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
        test_uart_framed_packets(&txUart, &rxUart);
        return;
    }

    printf("UART polling RX test failed\n");
}
