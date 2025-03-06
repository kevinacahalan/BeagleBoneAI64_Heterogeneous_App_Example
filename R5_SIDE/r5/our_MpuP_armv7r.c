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


#include <kernel/dpl/our_MpuP_armv7.h>
#include <kernel/dpl/our_CacheP.h>
#include <kernel/dpl/our_HwiP.h>

#define MPU_SECTION __attribute__((section(".text.mpu")))

/* Max possible regions in ARMv7-R CPU */
#define our_MpuP_MAX_REGIONS    (16u)

/* APIs defined in our_MpuP_armv7r_asm.s */
void our_MpuP_disableAsm(void);
void our_MpuP_enableAsm(void);
uint32_t our_MpuP_isEnableAsm(void);
void our_MpuP_disableBRAsm(void);
void our_MpuP_enableBRAsm(void);
void our_MpuP_setRegionAsm(uint32_t regionId, uint32_t regionBaseAddr, 
              uint32_t sizeAndEnble, uint32_t regionAttrs);

/* these are defined as part of SysConfig */
extern our_MpuP_Config gMpuConfig;
extern our_MpuP_RegionConfig gMpuRegionConfig[];


static uint32_t MPU_SECTION our_MpuP_getAttrs(our_MpuP_RegionAttrs *region)
{
    uint32_t regionAttrs = 
          ((uint32_t)(region->isExecuteNever & 0x1) << 12) 
        | ((uint32_t)(region->accessPerm     & 0x7) <<  8)
        | ((uint32_t)(region->tex            & 0x7) <<  3) 
        | ((uint32_t)(region->isSharable     & 0x1) <<  2) 
        | ((uint32_t)(region->isCacheable    & 0x1) <<  1) 
        | ((uint32_t)(region->isBufferable   & 0x1) <<  0); 

    return regionAttrs;
}

void MPU_SECTION our_MpuP_RegionAttrs_init(our_MpuP_RegionAttrs *region)
{
    region->isExecuteNever = 0;
    region->accessPerm     = our_MpuP_AP_S_RW_U_R;
    region->tex            = 0;
    region->isSharable     = 1;
    region->isCacheable    = 0;
    region->isBufferable   = 0;
    region->isEnable       = 0;
    region->subregionDisableMask = 0;
}

void MPU_SECTION our_MpuP_setRegion(uint32_t regionNum, void * addr, uint32_t size, our_MpuP_RegionAttrs *attrs)
{
    uint32_t baseAddress, sizeAndEnable, regionAttrs;
    uint32_t enabled;
    uintptr_t key;

    // DebugP_assertNoLog( regionNum < our_MpuP_MAX_REGIONS);

    /* size 5b field */
    size = (size & 0x1F);

    /* If N is the value in size field, the region size is 2N+1 bytes. */
    sizeAndEnable = ((uint32_t)(attrs->subregionDisableMask & 0xFF) << 8)
                  | ((uint32_t)(size            & 0x1F) << 1) 
                  | ((uint32_t)(attrs->isEnable &  0x1) << 0);

    /* align base address to region size */
    baseAddress = ((uint32_t)addr & ~( (1<<((uint64_t)size+1))-1 ));

    /* get region attribute mask */
    regionAttrs = our_MpuP_getAttrs(attrs);

    enabled = our_MpuP_isEnable();

    /* disable the MPU (if already disabled, does nothing) */
    our_MpuP_disable();

    key = our_HwiP_disable();

    our_MpuP_setRegionAsm(regionNum, baseAddress, sizeAndEnable, regionAttrs);

    our_HwiP_restore(key);

    if (enabled) {
        our_MpuP_enable();
    }
}

void MPU_SECTION our_MpuP_enable()
{
    if(!our_MpuP_isEnable())
    {
        uint32_t type;
        uintptr_t key;

        key = our_HwiP_disable();

        /* get the current enabled bits */
        type = our_CacheP_getEnabled();

        if (type & our_CacheP_TYPE_ALLP) {
            our_CacheP_disable(our_CacheP_TYPE_ALLP);
        }

        our_MpuP_enableAsm();

        /* set cache back to initial settings */
        our_CacheP_enable(type);

        __asm__ (" dsb");
        __asm__ (" isb");

        our_HwiP_restore(key);
    }
}

void MPU_SECTION our_MpuP_disable()
{
    if(our_MpuP_isEnable())
    {
        uint32_t type;
        uintptr_t key;

        key = our_HwiP_disable();

        /* get the current enabled bits */
        type = our_CacheP_getEnabled();

        /* disable all enabled caches */
        our_CacheP_disable(type);

        __asm__ (" dsb");

        our_MpuP_disableAsm();

        /* set cache back to initial settings */
        our_CacheP_enable(type);

        our_HwiP_restore(key);
    }
}

uint32_t MPU_SECTION our_MpuP_isEnable()
{
    return our_MpuP_isEnableAsm();
}

void MPU_SECTION our_MpuP_init()
{
    uint32_t i;

    if (our_MpuP_isEnable()) {
        our_MpuP_disable();
    }

    our_MpuP_disableBRAsm();

    // DebugP_assertNoLog( gMpuConfig.numRegions < our_MpuP_MAX_REGIONS);

    /*
     * Initialize MPU regions
     */
    for (i = 0; i < gMpuConfig.numRegions; i++) 
    {
        our_MpuP_setRegion(i, 
                (void*)gMpuRegionConfig[i].baseAddr,
                gMpuRegionConfig[i].size,
                &gMpuRegionConfig[i].attrs
        );
    }

    if (gMpuConfig.enableBackgroundRegion) {
        our_MpuP_enableBRAsm();
    }

    if (gMpuConfig.enableMpu) {
        our_MpuP_enable();
    }
}
