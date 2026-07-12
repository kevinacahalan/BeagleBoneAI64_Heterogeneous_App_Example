/*
 * PRU0_0 hello world — writes a string into the remoteproc trace buffer.
 *
 * Load on j7-pru0_0, then:
 *   sudo cat /sys/kernel/debug/remoteproc/remoteprocN/trace0
 */

#include <stdint.h>
#include <string.h>

#include "resource_table_hello.h"

void main(void)
{
	strcpy(gDebugMemLog, "Hello world, I am PRU0_0!\n");
	__halt();
}
