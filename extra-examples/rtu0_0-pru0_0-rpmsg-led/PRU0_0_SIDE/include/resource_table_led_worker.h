/*
 * resource_table_led_worker.h — TYPE_TRACE resource table for PRU0_0 led_worker.
 *
 * No virtio/RPMsg (RTU owns that). Size padded to a multiple of 16 for
 * kernel 6.12 ARM64 remoteproc memcpy into ICSSG memory.
 */

#ifndef RESOURCE_TABLE_LED_WORKER_H
#define RESOURCE_TABLE_LED_WORKER_H

#include <stddef.h>
#include <rsc_types.h>

#define DebugP_MEM_LOG_SIZE 512

#pragma DATA_SECTION(gDebugMemLog, ".log_shared_mem")
#pragma RETAIN(gDebugMemLog)
#pragma DATA_ALIGN(gDebugMemLog, 8)
char gDebugMemLog[DebugP_MEM_LOG_SIZE] = { 0 };

/*
 * Layout without pad: resource_table(16) + offset(4) + fw_rsc_trace(48) = 68.
 * Pad 12 bytes → 80 (multiple of 16).
 */
struct my_resource_table {
	struct resource_table base;
	uint32_t offset[1];
	struct fw_rsc_trace trace;
	uint32_t pad[3];
};

#pragma DATA_SECTION(pru_remoteproc_ResourceTable, ".resource_table")
#pragma RETAIN(pru_remoteproc_ResourceTable)
#pragma DATA_ALIGN(pru_remoteproc_ResourceTable, 8)
struct my_resource_table pru_remoteproc_ResourceTable = {
	{
		1,
		1,
		{ 0U, 0U },
	},
	{
		offsetof(struct my_resource_table, trace),
	},
	{
		TYPE_TRACE,
		(uint32_t)gDebugMemLog,
		DebugP_MEM_LOG_SIZE,
		0,
		"trace:pru0_0_led",
	},
	{ 0, 0, 0 },
};

#endif /* RESOURCE_TABLE_LED_WORKER_H */
