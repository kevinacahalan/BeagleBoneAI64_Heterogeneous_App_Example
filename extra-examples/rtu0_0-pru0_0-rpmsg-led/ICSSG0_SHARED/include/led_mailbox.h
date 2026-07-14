/*
 * Lock-free LED blink mailbox in ICSSG0 PRU_SHAREDMEM.
 *
 * Local address 0x10000 is visible to both PRU0 and RTU0 in the same slice.
 * Protocol: RTU writes blink_count then bumps seq; PRU blinks and sets
 * done_seq = seq; RTU waits for done_seq before RPMsg ACK.
 */

#ifndef LED_MAILBOX_H
#define LED_MAILBOX_H

#include <stdint.h>

#define LED_MAILBOX_ADDR 0x10000u
#define LED_MAILBOX_MAGIC 0x4C454442u /* 'LEDB' */

struct led_mailbox {
	uint32_t magic;
	uint32_t seq;
	uint32_t blink_count;
	uint32_t done_seq;
};

#define LED_MAILBOX ((volatile struct led_mailbox *)LED_MAILBOX_ADDR)

#endif /* LED_MAILBOX_H */
