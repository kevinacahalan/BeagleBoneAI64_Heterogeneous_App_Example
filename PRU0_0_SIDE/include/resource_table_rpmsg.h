/*
 * resource_table_rpmsg.h — VirtIO RPMsg resource table for PRU0_0.
 * Adapted from TI PSSP examples/j721e/PRU_RPMsg_Echo_Interrupt0.
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

struct my_resource_table {
	struct resource_table base;
	uint32_t offset[1];
	struct fw_rsc_vdev rpmsg_vdev;
	struct fw_rsc_vdev_vring rpmsg_vring0;
	struct fw_rsc_vdev_vring rpmsg_vring1;
	/* 88-byte payload → pad to 96 (multiple of 16) for rproc_start memcpy */
	uint32_t pad[2];
};

#pragma DATA_SECTION(resourceTable, ".resource_table")
#pragma RETAIN(resourceTable)
#pragma DATA_ALIGN(resourceTable, 8)
struct my_resource_table resourceTable = {
	1,
	1,
	0, 0,
	{
		offsetof(struct my_resource_table, rpmsg_vdev),
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
	{ 0, 0 },
};

#endif /* RESOURCE_TABLE_RPMSG_H */
