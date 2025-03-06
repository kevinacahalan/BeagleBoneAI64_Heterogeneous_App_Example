/*
 * Copyright (c) 2020-2022 Texas Instruments Incorporated - https://www.ti.com
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *    Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 *    Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 *    Neither the name of Texas Instruments Incorporated nor the names of
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

#ifndef __RPROC_ID_H__
#define __RPROC_ID_H__

#if defined(__cplusplus)
extern "C" {
#endif

/*!
 * @brief Enum values identifying a remote processor uniquely across
 *        all SoCs
 *
 *        The enum defines a unique remoteproc id and is limited by
 *        RPROC_ID_MAX value, and is agnostic of the SoC family.
 *
 *	  NOTE:
 *	  1. These enum values are to be used as identifiers only.
 *	     Do not assume a specific value associated with a particular
 *	     value.
 *	  2. The RPROC_ID_MAX again is subject to change as new remoteprocs
 *	     are added.
 *
 *        Any new remote processor will need to have an id added to
 *        this enum.
 */
enum rproc_id {
	R5F_MCU0_0 = 0,
	R5F_MCU0_1 = 1,
	R5F_MAIN0_0 = 2,
	R5F_MAIN0_1 = 3,
	R5F_MAIN1_0 = 4,
	R5F_MAIN1_1 = 5,
	DSP_C66_0 = 6,
	DSP_C66_1 = 7,
	DSP_C71_0 = 8,
	M4F_MCU0_0 = 9,
	DSP_C71_1 = 10,
	R5F_MAIN2_0 = 11,
	R5F_MAIN2_1 = 12,
	DSP_C71_2 = 13,
	DSP_C71_3 = 14,
	R5F_WKUP0_0 = 15,
	RPROC_ID_MAX,
};

#if defined(__cplusplus)
}
#endif

#endif /* __RPROC_ID_H__ */
