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

#ifndef our_DebugP_H
#define our_DebugP_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/**
 * \defgroup KERNEL_DPL_DEBUG APIs for Debug log's and assert's
 * \ingroup KERNEL_DPL
 *
 * For more details and example usage, see \ref KERNEL_DPL_DEBUG_PAGE
 *
 * @{
 */

/**
 * \anchor our_DebugP_LOG_ZONE
 * \name Debug log zones
 * @{
 */
/**
 * \brief Flag for always on zone, enabled by default and recommend to not switch off
 */
#define our_DebugP_LOG_ZONE_ALWAYS_ON       (0x0001U)
/**
 * \brief Flag for error zone.
 */
#define our_DebugP_LOG_ZONE_ERROR           (0x0002U)
/**
 * \brief Flag for warning zone.
 */
#define our_DebugP_LOG_ZONE_WARN            (0x0004U)
/**
 * \brief Flag for info zone.
 */
#define our_DebugP_LOG_ZONE_INFO            (0x0008U)

/** @} */

/**
 * \brief size of shared memory log for a CPU
 */
#define our_DebugP_SHM_LOG_SIZE         ( (2*1024U) - 16U)

/**
 * \brief size of memory log for a CPU
 */
#define our_DebugP_MEM_LOG_SIZE         ( 4*1024U )


/**
 * \brief Flag to indicate shared memory buffer is valid
 *
 * Written by the writer after it has initialized and cleared by the reader
 * after reader has initialized
 */
#define our_DebugP_SHM_LOG_IS_VALID     (0x12345678U)

/**
 * \brief Data structure describing log in shared memory
 */
typedef struct {

    uint32_t isValid;
    uint32_t rdIndex;
    uint32_t wrIndex;
    uint32_t rsv;
    uint8_t  buffer[our_DebugP_SHM_LOG_SIZE];

} our_DebugP_ShmLog;

/**
 * \name Compile time log and assert enable, disable
 * @{
 */

#ifndef our_DebugP_ASSERT_ENABLED
/**
 * \brief Pre-processor define to enable or disable our_DebugP assert's
 *
 * Set to 0 to disable assert checks and recompile all code where this file is included.
 */
#define our_DebugP_ASSERT_ENABLED 1
#endif

#ifndef our_DebugP_LOG_ENABLED
/**
 * \brief Pre-processor define to enable or disable our_DebugP log's
 *
 * Set to 0 to disable logging and recompile all code where this file is included.
 */
#define our_DebugP_LOG_ENABLED    1
#endif

/** @} */

#if our_DebugP_ASSERT_ENABLED
/**
 * \brief Actual function that is called for assert's by \ref our_DebugP_assert
 */
void _our_DebugP_assert(int expression, const char *file, const char *function, int line, const char *expressionString);

/**
 * \brief Actual function that is called for assert's by \ref our_DebugP_assertNoLog
 */
void _our_DebugP_assertNoLog(int expression);

/**
 * \name Debug assert APIs
 * @{
 */

/**
 * \brief Function to call for assert check
 *
 * If expresion evaluates to 0 then the function disable's interrupt and loops forever.
 * It logs the file name and line number before looping forever.
 * User should fix their code and run again.
 *
 * This API should not be called within ISR context.
 *
 * \param expression [in] expression to check for.
 */
#define our_DebugP_assert(expression)  \
    do { \
        _our_DebugP_assert(expression, \
            __FILE__, __FUNCTION__, __LINE__, #expression); \
    } while(0)

/**
 * \brief Function to call for assert check, no logs are printed
 *
 * Same as \ref our_DebugP_assert except no logs are printed.
 * This can be used in very early initialization code and in ISRs.
 *
 * During very early initialization and inside ISRs asserts with log will not work.
 *
 * \param expression [in] expression to check for.
 */
#define our_DebugP_assertNoLog(expression) (_our_DebugP_assertNoLog(expression))

/** @} */

#else
#define our_DebugP_assert(expression)
#define our_DebugP_assertNoLog(expression)
#endif

#if our_DebugP_LOG_ENABLED

/**
 * \brief Function to log a string to the enabled console for a given zone
 *
 * This API should not be used directly, instead \ref our_DebugP_log,
 * \ref our_DebugP_logError, \ref our_DebugP_logWarn, \ref our_DebugP_logInfo
 * should be used.
 *
 * \param logZone [in] Value from \ref our_DebugP_LOG_ZONE
 * \param format [in] String ot log
 */
void _our_DebugP_logZone(uint32_t logZone, char *format, ...);

/**
 * \name Debug log APIs
 * @{
 */

/**
 *
 * \brief Function to log a string to the enabled console
 *
 * This API should not be called within ISR context.
 *
 * \param format [in] String to log
 */
#define our_DebugP_log(format, ...)     \
    do { \
        _our_DebugP_logZone(our_DebugP_LOG_ZONE_ALWAYS_ON, format, ##__VA_ARGS__); \
    } while(0)

/**
 * \brief Function to log a string to the enabled console, for error zone.
 *
 * This API should not be called within ISR context.
 *
 * \param format [in] String to log
 */
#define our_DebugP_logError(format, ...)     \
    do { \
        _our_DebugP_logZone(our_DebugP_LOG_ZONE_ERROR, "ERROR: %s:%d: " format, __FUNCTION__, __LINE__, ##__VA_ARGS__); \
    } while(0)

/**
 * \brief Function to log a string to the enabled console, for warning zone.
 *
 * This API should not be called within ISR context.
 *
 * \param format [in] String to log
 */
#define our_DebugP_logWarn(format, ...)     \
    do { \
        _our_DebugP_logZone(our_DebugP_LOG_ZONE_WARN, "WARNING: %s:%d: " format, __FUNCTION__, __LINE__, ##__VA_ARGS__); \
    } while(0)

/**
 * \brief Function to log a string to the enabled console, for info zone.
 *
 * This API should not be called within ISR context.
 *
 * \param format [in] String to log
 */
#define our_DebugP_logInfo(format, ...)     \
    do { \
        _our_DebugP_logZone(our_DebugP_LOG_ZONE_INFO, "INFO: %s:%d: " format, __FUNCTION__, __LINE__, ##__VA_ARGS__); \
    } while(0)

 /** @} */

#else
#define our_DebugP_log(format, ...)
#define our_DebugP_logError(format, ...)
#define our_DebugP_logWarn(format, ...)
#define our_DebugP_logInfo(format, ...)
#endif

/**
 * \brief Enable log zones
 *
 * \param logZoneMask [in] Mask of values from \ref our_DebugP_LOG_ZONE
 *
 * \return old value of zone mask, can be used to restore original state using \ref our_DebugP_logZoneRestore
 */
uint32_t our_DebugP_logZoneEnable(uint32_t logZoneMask);

/**
 * \brief Disable log zones
 *
 * \param logZoneMask [in] Mask of values from \ref our_DebugP_LOG_ZONE
 *
 * \return old value of zone mask, can be used to restore original state using \ref our_DebugP_logZoneRestore
 */
uint32_t our_DebugP_logZoneDisable(uint32_t logZoneMask);

/**
 * \brief Restire zone mask returned from \ref our_DebugP_logZoneDisable or \ref our_DebugP_logZoneEnable
 *
 * \param logZoneMask [in] Mask of values from \ref our_DebugP_LOG_ZONE
 */
void our_DebugP_logZoneRestore(uint32_t logZoneMask);

/**
 * \brief Initialize shared memory log writer for this core
 *
 * \param shmLog     [in] Address of shared memory where the writer should write logs to
 * \param selfCoreId [in] ID of core on which this API is called,
 *                        this is used to add a core name prefix string to each log line, see \ref our_CSL_CoreID
 */
void our_DebugP_shmLogWriterInit(our_DebugP_ShmLog *shmLog, uint16_t selfCoreId);

/**
 * \brief Write a character to shared memory log
 *
 * If shared memory log buffer is full, nothing is written and character gets "dropped"
 *
 * Internally, the charaxter is stored in a local line buffer and
 * line buffer is flushed to shared memory only when a '\\n' character
 * is put.
 *
 * \param character  [in] character to write
 */
void our_DebugP_shmLogWriterPutChar(char character);


/**
 * \brief Write a character to UART terminal
 *
 *        Make sure the UART to use is set via our_DebugP_uartSetDrvIndex().
 *        When using SysConfig this is done when UART debug log is enabled.
 *
 * \param character  [in] character to write
 */
void our_DebugP_uartLogWriterPutChar(char character);

/**
 * \brief Initialize log reader to read from shared memory and log to console via our_DebugP_log
 *
 * The parameter `shmLog` is a array and is indexed using core ID as defined by \ref our_CSL_CoreID
 *
 * \param shmLog    [in] Array of addresses of shared memory where the reader should read from.
 * \param numCores  [in] Number of entries in the shmLog array. Typically \ref our_CSL_CORE_ID_MAX
 */
void our_DebugP_shmLogReaderInit(our_DebugP_ShmLog *shmLog, uint16_t numCores);


/**
 * \brief Initialize log write to write to memory trace buffer.
 *
 * Used when IPC with Linux is enabled
 * OR
 * ROV based logging is enabled
 *
 * \param selfCoreId [in] ID of core on which this API is called,
 *                        this is used to add a core name prefix string to each log line, see \ref our_CSL_CoreID
 */
void our_DebugP_memLogWriterInit(uint16_t selfCoreId);

/**
 * \brief Write a character to trace buffer.
 *
 * Used when IPC with Linux is enabled
 * OR
 * ROV based logging is enabled
 *
 * Internally, the charaxter is stored in a local line buffer and
 * line buffer is flushed to UART only when a '\\n' character
 * is put.
 *
 * \param character  [in] character to write
 */
void our_DebugP_memLogWriterPutChar(char character);

/**
 * \brief Set UART driver index to use for character read and write form UART
 *
 *        Make sure the UART to use is set via our_DebugP_uartSetDrvIndex().
 *        When using SysConfig this is done when UART debug log is enabled.
 *
 * \param uartDrvIndex  [in] UART driver instance index to use
 */
void our_DebugP_uartSetDrvIndex(uint32_t uartDrvIndex);

/**
 * \brief Read a formatted string from the selected UART driver.
 *
 *        This function returns when a "new line" or "enter" is input on the console.
 *
 *        Make sure the UART to use is set via our_DebugP_uartSetDrvIndex().
 *        When using SysConfig this is done when UART debug log is enabled.
 *
 * \return our_SystemP_SUCCESS on sucessful read
 * \return our_SystemP_FAILURE on failure
 */
int32_t our_DebugP_scanf(char *format, ...);

/**
 * \brief Read a string from the selected UART driver
 *
 *        This function returns when a "new line" or "enter" is input on the console.
 *
 *        Make sure the UART to use for reading is set via our_DebugP_uartSetDrvIndex.
 *        When using SysConfig this is done when UART debug log is enabled.
 *
 *        A '\0' is always put at the end.
 *
 * \param lineBuf   [in] Buffer into which the string is read
 * \param bufSize   [in] Size of the buffer in which to read, if buffer is not enough, input is truncated.
 *
 * \return our_SystemP_SUCCESS on sucessful read
 * \return our_SystemP_FAILURE on failure
 */
int32_t our_DebugP_readLine(char *lineBuf, uint32_t bufSize);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* our_DebugP_H */
