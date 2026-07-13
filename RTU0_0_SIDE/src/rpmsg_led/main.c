/*
 * RTU0_0 RPMsg LED control — receive blink count from Linux, post to shared
 * mailbox for PRU0_0 led_worker, ACK after PRU finishes.
 *
 * Channel "rpmsg-raw" / port 30 (kernel 6.12 rpmsg_char).
 * Overlay must provide &rtu0_0 vring IRQ <20 4 4> or boot fails -ENXIO.
 * Debug: remoteproc trace0 via TYPE_TRACE / gDebugMemLog.
 */

#include <stdint.h>
#include <string.h>
#include <pru_intc.h>
#include <rsc_types.h>
#include <pru_rpmsg.h>

#include "resource_table_rpmsg.h"
#include "intc_map_0.h"
#include "led_mailbox.h"

volatile register uint32_t __R31;

/* Host-10 interrupt sets bit 30 in R31 for RTU cores */
#define HOST_INT ((uint32_t)1 << 30)

/* RTU0 RPMsg system events (PSSP / device tree convention) */
#define TO_ARM_HOST 20
#define FROM_ARM_HOST 21

#define CHAN_NAME "rpmsg-raw"
#define CHAN_PORT 30

#define VIRTIO_CONFIG_S_DRIVER_OK 4

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

static void mailbox_init(void)
{
	volatile struct led_mailbox *mb = LED_MAILBOX;

	mb->magic = LED_MAILBOX_MAGIC;
	mb->seq = 0;
	mb->blink_count = 0;
	mb->done_seq = 0;
}

static void mailbox_request_and_wait(uint32_t count)
{
	volatile struct led_mailbox *mb = LED_MAILBOX;
	uint32_t next_seq;

	mb->blink_count = count;
	next_seq = mb->seq + 1u;
	mb->seq = next_seq;

	while (mb->done_seq != next_seq) {
		;
	}
}

void main(void)
{
	struct pru_rpmsg_transport transport;
	uint16_t src, dst, len;
	volatile uint8_t *status;

	g_trace_pos = 0;
	gDebugMemLog[0] = '\0';

	mailbox_init();
	trace_puts("RTU0_0 rpmsg_led boot\n");

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
					mailbox_request_and_wait(count);
					trace_puts("pru done\n");
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
