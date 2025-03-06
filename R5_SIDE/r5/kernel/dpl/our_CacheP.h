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

#ifndef our_CacheP_H
#define our_CacheP_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <kernel/dpl/our_SystemP.h>
#if defined(_TMS320C6X)
#include <kernel/dpl/CacheP_c6x.h>
#endif

/**
 * \defgroup KERNEL_DPL_CACHE APIs for Cache
 * \ingroup KERNEL_DPL
 *
 * For more details and example usage, see \ref KERNEL_DPL_CACHE_PAGE
 *
 * @{
 */

/**
 * \brief  Cache line size for alignment of buffers.
 * Actual CPU defined cache line can be smaller that this value, this define
 * is a utility macro to keep application portable across different CPU's.
 */
#define our_CacheP_CACHELINE_ALIGNMENT   (128U)

/**
 * \brief Cache type
 */
typedef enum our_CacheP_Type_ {
    our_CacheP_TYPE_L1P  = (0x0001u), /**< L1 program cache */
    our_CacheP_TYPE_L1D  = (0x0002u), /**< L1 data cache */
    our_CacheP_TYPE_L2P  = (0x0004u), /**< L2 program cache */
    our_CacheP_TYPE_L2D  = (0x0008u), /**< L2 data cache */
    our_CacheP_TYPE_L1   = (our_CacheP_TYPE_L1P|our_CacheP_TYPE_L1D), /**< All L1 cache's */
    our_CacheP_TYPE_L2   = (our_CacheP_TYPE_L2P|our_CacheP_TYPE_L2D), /**< All L2 cache's */
    our_CacheP_TYPE_ALLP = (our_CacheP_TYPE_L1P|our_CacheP_TYPE_L2P), /**< All program cache's */
    our_CacheP_TYPE_ALLD = (our_CacheP_TYPE_L1D|our_CacheP_TYPE_L2D), /**< All data cache's */
    our_CacheP_TYPE_ALL  = (our_CacheP_TYPE_L1|our_CacheP_TYPE_L2)    /**< All cache's */
} our_CacheP_Type;

/**
 * \brief Cache config structure, this used by SysConfig and not to be used by end-users directly
 */
typedef struct our_CacheP_Config_ {

    uint32_t enable;    /**< 0: cache disabled, 1: cache enabled */
    uint32_t enableForceWrThru; /**< 0: force write through disabled, 1: force write through enabled */

} our_CacheP_Config;

/** \brief Externally defined Cache configuration */
extern our_CacheP_Config    gCacheConfig;

/**
 * \brief Cache enable
 *
 * \param type [in] cache type's to enable
 */
void our_CacheP_enable(uint32_t type);

/**
 * \brief Cache disable
 *
 * \param type [in] cache type's to disable
 */
void our_CacheP_disable(uint32_t type);

/**
 * \brief Get cache enabled bits
 *
 * \return cache type's that are enabled
 */
uint32_t our_CacheP_getEnabled();

/**
 * \brief Cache writeback for full cache
 *
 * \param type [in] cache type's to writeback
 */
void our_CacheP_wbAll(uint32_t type);

/**
 * \brief Cache writeback and invalidate for full cache
 *
 * \param type [in] cache type's to writeback and invalidate
 */
void our_CacheP_wbInvAll(uint32_t type);

/**
 * \brief Cache writeback for a specified region
 *
 * \param addr [in] region address. Recommend to specify address that is cache line aligned
 * \param size [in] region size in bytes. Recommend to specify size that is multiple of cache line size
 * \param type [in] cache type's to writeback
 */
void our_CacheP_wb(void *addr, uint32_t size, uint32_t type);

/**
 * \brief Cache invalidate for a specified region
 *
 * \param addr [in] region address. Recommend to specify address that is cache line aligned
 * \param size [in] region size in bytes. Recommend to specify size that is multiple of cache line size
 * \param type [in] cache type's to invalidate
 */
void our_CacheP_inv(void *addr, uint32_t size, uint32_t type);

/**
 * \brief Cache writeback and invalidate for a specified region
 *
 * \param addr [in] region address. Recommend to specify address that is cache line aligned
 * \param size [in] region size in bytes. Recommend to specify size that is multiple of cache line size
 * \param type [in] cache type's to writeback and invalidate
 */
void our_CacheP_wbInv(void *addr, uint32_t size, uint32_t type);

/**
 * \brief Initialize Cache sub-system, called by SysConfig, not to be called by end users
 *
 */
void our_CacheP_init();

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* our_CacheP_H */

