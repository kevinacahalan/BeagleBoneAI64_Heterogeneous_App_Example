/*
 * resource_table_rpmsg.h — VirtIO RPMsg + TYPE_TRACE for RTU0_0.
 * Adapted from TI PSSP examples/j721e/RTU_RPMsg_Echo_Interrupt0.
 *
 * Trace buffer is small (256 B): RTU0_DMEM_0 is only 2 KB and also holds
 * the RPMsg payload. Log lives in .log_shared_mem on RTU0_DMEM_1.
 */

#ifndef RESOURCE_TABLE_RPMSG_H
#define RESOURCE_TABLE_RPMSG_H

#include <stddef.h>
#include <rsc_types.h>
#include <pru_virtio_ids.h>

#define PRU_RPMSG_VQ0_SIZE 16
#define PRU_RPMSG_VQ1_SIZE 16

#define VIRTIO_RPMSG_F_NS 0
#define RPMSG_PRU_C0_FEATURES (1 << VIRTIO_RPMSG_F_NS)

#define DebugP_MEM_LOG_SIZE 256

#pragma DATA_SECTION(gDebugMemLog, ".log_shared_mem")
#pragma RETAIN(gDebugMemLog)
#pragma DATA_ALIGN(gDebugMemLog, 8)
char gDebugMemLog[DebugP_MEM_LOG_SIZE] = { 0 };

/*
 * Without pad: base(16) + offset[2](8) + vdev+vrings(68) + trace(48) = 140.
 * Pad 4 → 144 (multiple of 16) for rproc_start memcpy.
 */
struct my_resource_table {
	struct resource_table base;
	uint32_t offset[2];
	struct fw_rsc_vdev rpmsg_vdev;
	struct fw_rsc_vdev_vring rpmsg_vring0;
	struct fw_rsc_vdev_vring rpmsg_vring1;
	struct fw_rsc_trace trace;
	uint32_t pad[1];
};

#pragma DATA_SECTION(resourceTable, ".resource_table")
#pragma RETAIN(resourceTable)
#pragma DATA_ALIGN(resourceTable, 8)
struct my_resource_table resourceTable = {
	1,
	2,
	0, 0,
	{
		offsetof(struct my_resource_table, rpmsg_vdev),
		offsetof(struct my_resource_table, trace),
	},
	{
		(uint32_t)TYPE_VDEV,
		(uint32_t)VIRTIO_ID_RPMSG,
		(uint32_t)0,
		(uint32_t)RPMSG_PRU_C0_FEATURES,
		(uint32_t)0,
		(uint32_t)0,
		(uint8_t)0,
		(uint8_t)2,
		{ (uint8_t)0, (uint8_t)0 },
	},
	{
		FW_RSC_ADDR_ANY,
		16,
		PRU_RPMSG_VQ0_SIZE,
		0,
		0,
	},
	{
		FW_RSC_ADDR_ANY,
		16,
		PRU_RPMSG_VQ1_SIZE,
		0,
		0,
	},
	{
		TYPE_TRACE,
		(uint32_t)gDebugMemLog,
		DebugP_MEM_LOG_SIZE,
		0,
		"trace:rtu0_0",
	},
	{ 0 },
};

#endif /* RESOURCE_TABLE_RPMSG_H */
