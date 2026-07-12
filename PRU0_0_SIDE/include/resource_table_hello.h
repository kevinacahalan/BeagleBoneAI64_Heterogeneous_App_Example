/*
 * resource_table_hello.h — TYPE_TRACE resource table for PRU0_0 hello.
 *
 * Kernel 6.12 copies this into PRU DMEM with ARM64 memcpy. ICSSG device
 * memory can fault on odd-sized copies, so sizeof(resource table) must
 * be a multiple of 16 bytes.
 */

#ifndef RESOURCE_TABLE_HELLO_H
#define RESOURCE_TABLE_HELLO_H

#include <stddef.h>
#include <rsc_types.h>

#define DebugP_MEM_LOG_SIZE 1024

/* PROGBITS (initialized) so the ELF loads the buffer into DMEM */
#pragma DATA_SECTION(gDebugMemLog, ".log_shared_mem")
#pragma RETAIN(gDebugMemLog)
#pragma DATA_ALIGN(gDebugMemLog, 8)
char gDebugMemLog[DebugP_MEM_LOG_SIZE] = { 0 };

/*
 * Layout without pad: resource_table(16) + offset(4) + fw_rsc_trace(48) = 68.
 * Pad 12 bytes → 80 (multiple of 16) for safe rproc_start memcpy.
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
		1,	/* version */
		1,	/* number of entries */
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
		"trace:pru0_0",
	},
	{ 0, 0, 0 },
};

#endif /* RESOURCE_TABLE_HELLO_H */
