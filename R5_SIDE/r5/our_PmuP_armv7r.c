/*
 *  Copyright (C) 2018-2021 Texas Instruments Incorporated
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *    Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 *    Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the
 *    distribution.
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
 */


#include <kernel/dpl/CycleCounterP.h>

/* NOTE: CycleCounterP_getCount32 is implmented in our_PmuP_armv7r_asm.S */

#define PMU_SECTION __attribute__((section(".text.pmu")))

#define our_PmuP_SETUP_FLAG_CYCLE_COUNTER_DIV64     (1u<<3u)
#define our_PmuP_SETUP_FLAG_CYCLE_COUNTER_RESET     (1u<<2u)
#define our_PmuP_SETUP_FLAG_EVENT_COUNTER_RESET     (1u<<1u)
#define our_PmuP_SETUP_FLAG_ENABLE_ALL_COUNTERS     (1u<<0u)

#define our_PmuP_COUNTER_MASK_CYCLE_COUNTER         (1u<<31u)
#define our_PmuP_COUNTER_MASK_ALL_COUNTERS          (0xFFFFFFFFu)

void our_PmuP_enableCounters(uint32_t counterMask);
void our_PmuP_disableCounters(uint32_t counterMask);
void our_PmuP_clearOverflowStatus(uint32_t counterMask);
void our_PmuP_setup(uint32_t setupFlags);

void PMU_SECTION CycleCounterP_reset()
{
    uint32_t setupFlags = 0;

    setupFlags |= our_PmuP_SETUP_FLAG_CYCLE_COUNTER_RESET;
    setupFlags |= our_PmuP_SETUP_FLAG_EVENT_COUNTER_RESET;
    setupFlags |= our_PmuP_SETUP_FLAG_ENABLE_ALL_COUNTERS;

    our_PmuP_disableCounters(our_PmuP_COUNTER_MASK_ALL_COUNTERS); /* disable all counters */
    our_PmuP_clearOverflowStatus(our_PmuP_COUNTER_MASK_ALL_COUNTERS); /* clear all overflow flags */
    our_PmuP_setup(setupFlags); /* setup counters */
    our_PmuP_enableCounters(our_PmuP_COUNTER_MASK_CYCLE_COUNTER); /* enable cycle counter only */
}

