/*
 * Copyright (c) 2020-2022 Texas Instruments Incorporated - https://www.ti.com
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <errno.h>
#include <stdio.h>
#include <stdbool.h>
#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#include "rpmsg_char_internal.h"

/* Increment this whenever new SoC data is added */
#define NUM_SOC_FAMILY 10

struct soc_data {
	const char *family_name;
	const struct rproc_map *map;
	int num_rprocs;
};

/* TI K3 AM65x SoCs */
const struct rproc_map am65x_map[] = {
	{ .id = R5F_MCU0_0, .rproc_name = "41000000.r5f", },
	{ .id = R5F_MCU0_1, .rproc_name = "41400000.r5f", },
};

/* TI K3 J721E SoCs */
const struct rproc_map j721e_map[] = {
	{ .id = R5F_MCU0_0,  .rproc_name = "41000000.r5f",   },
	{ .id = R5F_MCU0_1,  .rproc_name = "41400000.r5f",   },
	{ .id = R5F_MAIN0_0, .rproc_name = "5c00000.r5f",    },
	{ .id = R5F_MAIN0_1, .rproc_name = "5d00000.r5f",    },
	{ .id = R5F_MAIN1_0, .rproc_name = "5e00000.r5f",    },
	{ .id = R5F_MAIN1_1, .rproc_name = "5f00000.r5f",    },
	{ .id = DSP_C66_0,   .rproc_name = "4d80800000.dsp", },
	{ .id = DSP_C66_1,   .rproc_name = "4d81800000.dsp", },
	{ .id = DSP_C71_0,   .rproc_name = "64800000.dsp",   },
};

/* TI K3 J7200 SoCs */
const struct rproc_map j7200_map[] = {
	{ .id = R5F_MCU0_0,  .rproc_name = "41000000.r5f", },
	{ .id = R5F_MCU0_1,  .rproc_name = "41400000.r5f", },
	{ .id = R5F_MAIN0_0, .rproc_name = "5c00000.r5f",  },
	{ .id = R5F_MAIN0_1, .rproc_name = "5d00000.r5f",  },
};

/* TI K3 AM64x SoCs */
const struct rproc_map am64x_map[] = {
	{ .id = R5F_MAIN0_0, .rproc_name = "78000000.r5f",  },
	{ .id = R5F_MAIN0_1, .rproc_name = "78200000.r5f",  },
	{ .id = R5F_MAIN1_0, .rproc_name = "78400000.r5f",  },
	{ .id = R5F_MAIN1_1, .rproc_name = "78600000.r5f",  },
	{ .id = M4F_MCU0_0,  .rproc_name = "5000000.m4fss",  },
};

/* TI K3 J721S2 SoCs */
const struct rproc_map j721s2_map[] = {
	{ .id = R5F_MCU0_0,  .rproc_name = "41000000.r5f",   },
	{ .id = R5F_MCU0_1,  .rproc_name = "41400000.r5f",   },
	{ .id = R5F_MAIN0_0, .rproc_name = "5c00000.r5f",    },
	{ .id = R5F_MAIN0_1, .rproc_name = "5d00000.r5f",    },
	{ .id = R5F_MAIN1_0, .rproc_name = "5e00000.r5f",    },
	{ .id = R5F_MAIN1_1, .rproc_name = "5f00000.r5f",    },
	{ .id = DSP_C71_0,   .rproc_name = "64800000.dsp",   },
	{ .id = DSP_C71_1,   .rproc_name = "65800000.dsp",   },
};


/* TI K3 AM62x SoCs */
const struct rproc_map am62x_map[] = {
	{ .id = M4F_MCU0_0,  .rproc_name = "5000000.m4fss",  },
	{ .id = R5F_WKUP0_0, .rproc_name = "78000000.r5f",   },
};

/* TI K3 J784S4 SoCs */
const struct rproc_map j784s4_map[] = {
	{ .id = R5F_MCU0_0,  .rproc_name = "41000000.r5f",   },
	{ .id = R5F_MCU0_1,  .rproc_name = "41400000.r5f",   },
	{ .id = R5F_MAIN0_0, .rproc_name = "5c00000.r5f",    },
	{ .id = R5F_MAIN0_1, .rproc_name = "5d00000.r5f",    },
	{ .id = R5F_MAIN1_0, .rproc_name = "5e00000.r5f",    },
	{ .id = R5F_MAIN1_1, .rproc_name = "5f00000.r5f",    },
	{ .id = R5F_MAIN2_0, .rproc_name = "5900000.r5f",    },
	{ .id = R5F_MAIN2_1, .rproc_name = "5a00000.r5f",    },
	{ .id = DSP_C71_0,   .rproc_name = "64800000.dsp",   },
	{ .id = DSP_C71_1,   .rproc_name = "65800000.dsp",   },
	{ .id = DSP_C71_2,   .rproc_name = "66800000.dsp",   },
	{ .id = DSP_C71_3,   .rproc_name = "67800000.dsp",   },
};

/* TI K3 AM62Ax SoCs */
const struct rproc_map am62ax_map[] = {
	{ .id = R5F_WKUP0_0, .rproc_name = "78000000.r5f",   },
        { .id = R5F_MCU0_0,  .rproc_name = "79000000.r5f",   },
	{ .id = DSP_C71_0,   .rproc_name = "7e000000.dsp",   },
};

/* TI K3 AM62Px SoCs */
const struct rproc_map am62px_map[] = {
	{ .id = R5F_WKUP0_0, .rproc_name = "78000000.r5f",   },
        { .id = R5F_MCU0_0,  .rproc_name = "79000000.r5f",   },
};

/* TI K3 J722S SoCs */
const struct rproc_map j722s_map[] = {
	{ .id = R5F_WKUP0_0, .rproc_name = "78000000.r5f",   },
	{ .id = R5F_MCU0_0,  .rproc_name = "79000000.r5f",   },
	{ .id = R5F_MAIN0_0, .rproc_name = "78400000.r5f",   },
	{ .id = DSP_C71_0,   .rproc_name = "7e000000.dsp",   },
	{ .id = DSP_C71_1,   .rproc_name = "7e200000.dsp",   },
};

const struct soc_data socs[NUM_SOC_FAMILY] = {
	{
		.family_name = "AM65X",
		.map = am65x_map,
		.num_rprocs = (sizeof(am65x_map) / sizeof(struct rproc_map)),
	},
	{
		.family_name = "J721E",
		.map = j721e_map,
		.num_rprocs = (sizeof(j721e_map) / sizeof(struct rproc_map)),
	},
	{
		.family_name = "J7200",
		.map = j7200_map,
		.num_rprocs = (sizeof(j7200_map) / sizeof(struct rproc_map)),
	},
	{
		.family_name = "AM64X",
		.map = am64x_map,
		.num_rprocs = (sizeof(am64x_map) / sizeof(struct rproc_map)),
	},
	{
		.family_name = "J721S2",
		.map = j721s2_map,
		.num_rprocs = (sizeof(j721s2_map) / sizeof(struct rproc_map)),
	},
	{
		.family_name = "AM62X",
		.map = am62x_map,
		.num_rprocs = (sizeof(am62x_map) / sizeof(struct rproc_map)),
	},
	{
		.family_name = "J784S4",
		.map = j784s4_map,
		.num_rprocs = (sizeof(j784s4_map) / sizeof(struct rproc_map)),
	},
	{
		.family_name = "AM62AX",
		.map = am62ax_map,
		.num_rprocs = (sizeof(am62ax_map) / sizeof(struct rproc_map)),
	},
	{
		.family_name = "AM62PX",
		.map = am62px_map,
		.num_rprocs = (sizeof(am62px_map) / sizeof(struct rproc_map)),
	},
	{
		.family_name = "J722S",
		.map = j722s_map,
		.num_rprocs = (sizeof(j722s_map) / sizeof(struct rproc_map)),
	},
};

int _rpmsg_char_find_soc_family(const char *name, struct soc_rprocs *soc)
{
	int ret, i;
	char family_name[32];
	bool found = false;

	if (check_dir("/sys/devices/soc0")) {
		fprintf(stderr, "Kernel doesn't have a soc device, use fallback detection..\n");
		goto fallback;
	}

	ret = file_read_string("/sys/devices/soc0/family", family_name,
				sizeof(family_name));
	if (ret < 0)
		return -ENOENT;

	for (i = 0; i < NUM_SOC_FAMILY; i++) {
		if (!strcmp(family_name, socs[i].family_name)) {
			found = true;
			break;
		}
	}

fallback:
	/* fall back to using passed in SoC family name */
	if (!found && name) {
		for (i = 0; i < NUM_SOC_FAMILY; i++) {
			if (!strcmp(name, socs[i].family_name)) {
				found = true;
				break;
			}
		}
	}

	if (!found) {
		fprintf(stderr, "%s: SoC device family match failed\n", __func__);
		return -ENOENT;
	}

	soc->num_rprocs = socs[i].num_rprocs;
	soc->map = socs[i].map;

	return 0;
}
