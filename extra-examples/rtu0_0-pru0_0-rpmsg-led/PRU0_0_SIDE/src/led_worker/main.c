/*
 * PRU0_0 LED worker — blink P8_11 from shared DMEM mailbox (no RPMsg).
 *
 * Pair with RTU0_0 rpmsg_led. P8_11 = PRG0_PRU0_GPO17 = __R30 bit 17.
 * Debug: remoteproc trace0 via TYPE_TRACE / gDebugMemLog.
 */

#include <stdint.h>
#include <am65x/pru_cfg.h>

#include "resource_table_led_worker.h"
#include "led_mailbox.h"

volatile register uint32_t __R30;

#define LED_BIT 17
#define LED_MASK ((uint32_t)1 << LED_BIT)

/* Half-period delay for ~2 Hz blink at ~250 MHz ICSSG PRU clock. */
#define BLINK_HALF_CYCLES 62500000u

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
	volatile struct led_mailbox *mb = LED_MAILBOX;
	uint32_t last_seq = 0;

	g_trace_pos = 0;
	gDebugMemLog[0] = '\0';

	CT_CFG.gpcfg0_reg = 0;
	__R30 &= ~LED_MASK;

	mb->magic = LED_MAILBOX_MAGIC;
	mb->done_seq = 0;
	last_seq = mb->seq;

	trace_puts("PRU0_0 led_worker ready\n");

	while (1) {
		uint32_t seq = mb->seq;

		if (seq != last_seq) {
			uint32_t count = mb->blink_count;

			last_seq = seq;
			trace_puts("blink ");
			trace_u32(count);
			trace_puts(" seq=");
			trace_u32(seq);
			trace_puts("\n");

			if (count > 0) {
				blink_led(count);
			}
			mb->done_seq = seq;
			trace_puts("done\n");
		}
	}
}
