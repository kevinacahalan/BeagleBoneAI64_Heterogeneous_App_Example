/*
 * PRU0_0 RPMsg LED blink — receive blink count from Linux, toggle P8_11.
 *
 * Channel name "rpmsg-raw" probes rpmsg_char (kernel 6.12; no rpmsg_pru).
 * Port 30 → /dev/rpmsgN (see host/blink_count.py).
 * P8_11 = PRG0_PRU0_GPO17 = __R30 bit 17 (overlay must mux mode 0).
 * Overlay must provide &pru0_0 vring IRQ or boot fails with -ENXIO.
 * Debug: remoteproc trace0 via TYPE_TRACE / gDebugMemLog.
 */

#include <stdint.h>
#include <string.h>
#include <am65x/pru_cfg.h>
#include <pru_intc.h>
#include <rsc_types.h>
#include <pru_rpmsg.h>

#include "resource_table_rpmsg.h"
#include "intc_map_0.h"

volatile register uint32_t __R30;
volatile register uint32_t __R31;

/* Host-0 interrupt sets bit 30 in R31 for PRU cores */
#define HOST_INT ((uint32_t)1 << 30)

/* PRU0 RPMsg system events (device tree / PSSP convention) */
#define TO_ARM_HOST 16
#define FROM_ARM_HOST 17

#define CHAN_NAME "rpmsg-raw"
#define CHAN_PORT 30

#define VIRTIO_CONFIG_S_DRIVER_OK 4

/* P8_11 → PRG0_PRU0_GPO17 */
#define LED_BIT 17
#define LED_MASK ((uint32_t)1 << LED_BIT)

/*
 * Half-period delay for ~2 Hz blink at ~250 MHz ICSSG PRU clock.
 * __delay_cycles(N) burns N PRU cycles.
 */
#define BLINK_HALF_CYCLES 62500000u

uint8_t payload[RPMSG_MESSAGE_SIZE];

static uint16_t g_trace_pos;

static void trace_putc(char c)
{
	if (g_trace_pos < (DebugP_MEM_LOG_SIZE - 1u)) {
		gDebugMemLog[g_trace_pos++] = c;
		gDebugMemLog[g_trace_pos] = '\0';
	}
}

static void trace_puts(const char *s)
{
	while (*s != '\0') {
		trace_putc(*s);
		s++;
	}
}

static void trace_u32(uint32_t val)
{
	char temp[16];
	uint16_t i = 0;
	uint16_t j;

	if (val == 0u) {
		trace_putc('0');
		return;
	}
	while (val > 0u) {
		temp[i++] = (char)('0' + (val % 10u));
		val /= 10u;
	}
	for (j = 0; j < i; j++) {
		trace_putc(temp[i - 1u - j]);
	}
}

static uint32_t parse_uint32(const char *str, uint16_t max_len)
{
	uint32_t val = 0;
	uint8_t digit_found = 0;
	uint16_t i;

	for (i = 0; i < max_len && str[i] != '\0'; i++) {
		char c = str[i];
		if (c >= '0' && c <= '9') {
			val = val * 10u + (uint32_t)(c - '0');
			digit_found = 1;
		} else if (c == '\n' || c == '\r' || c == ' ' || c == '\t') {
			if (digit_found) {
				break;
			}
		} else {
			break;
		}
	}
	return digit_found ? val : 0;
}

static uint16_t write_uint32_str(char *buf, uint32_t val)
{
	char temp[16];
	uint16_t i = 0;
	uint16_t j;

	if (val == 0) {
		buf[0] = '0';
		buf[1] = '\0';
		return 1;
	}
	while (val > 0) {
		temp[i++] = (char)('0' + (val % 10u));
		val /= 10u;
	}
	for (j = 0; j < i; j++) {
		buf[j] = temp[i - 1u - j];
	}
	buf[i] = '\0';
	return i;
}

static void blink_led(uint32_t count)
{
	uint32_t n;

	for (n = 0; n < count; n++) {
		__R30 |= LED_MASK;
		__delay_cycles(BLINK_HALF_CYCLES);
		__R30 &= ~LED_MASK;
		__delay_cycles(BLINK_HALF_CYCLES);
	}
}

void main(void)
{
	struct pru_rpmsg_transport transport;
	uint16_t src, dst, len;
	volatile uint8_t *status;

	g_trace_pos = 0;
	gDebugMemLog[0] = '\0';

	/* Route __R30 to external GPO pins (same as am65x/j721e ICSSG) */
	CT_CFG.gpcfg0_reg = 0;
	trace_puts("PRU0_0 rpmsg_led boot\n");

	CT_INTC.STATUS_CLR_INDEX_REG_bit.STATUS_CLR_INDEX = FROM_ARM_HOST;

	status = &resourceTable.rpmsg_vdev.status;
	while (!(*status & VIRTIO_CONFIG_S_DRIVER_OK)) {
		;
	}
	trace_puts("virtio DRIVER_OK\n");

	pru_rpmsg_init(&transport, &resourceTable.rpmsg_vring0,
		       &resourceTable.rpmsg_vring1, TO_ARM_HOST, FROM_ARM_HOST);

	while (pru_rpmsg_channel(RPMSG_NS_CREATE, &transport, CHAN_NAME, CHAN_PORT) !=
	       PRU_RPMSG_SUCCESS) {
		;
	}
	trace_puts("channel ready port=30\n");

	while (1) {
		if (__R31 & HOST_INT) {
			CT_INTC.STATUS_CLR_INDEX_REG_bit.STATUS_CLR_INDEX = FROM_ARM_HOST;

			while (pru_rpmsg_receive(&transport, &src, &dst, payload, &len) ==
			       PRU_RPMSG_SUCCESS) {
				uint32_t count;
				char response[64];
				uint16_t num_len;
				const char *prefix = "Blinked ";
				const char *suffix = " times\n";
				uint16_t i;
				uint16_t pos = 0;

				if (len < RPMSG_MESSAGE_SIZE) {
					payload[len] = '\0';
				} else {
					payload[RPMSG_MESSAGE_SIZE - 1] = '\0';
				}

				count = parse_uint32((char *)payload, len);
				trace_puts("rx count=");
				trace_u32(count);
				trace_puts("\n");

				if (count > 0) {
					blink_led(count);
					trace_puts("blink done\n");
				}

				for (i = 0; prefix[i] != '\0'; i++) {
					response[pos++] = prefix[i];
				}
				num_len = write_uint32_str(response + pos, count);
				pos = (uint16_t)(pos + num_len);
				for (i = 0; suffix[i] != '\0'; i++) {
					response[pos++] = suffix[i];
				}
				response[pos] = '\0';

				pru_rpmsg_send(&transport, dst, src, response, (uint16_t)(pos + 1u));
				trace_puts("ack sent\n");
			}
		}
	}
}
