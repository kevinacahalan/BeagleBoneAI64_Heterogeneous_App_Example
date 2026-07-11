/*
 *  Copyright (c) Texas Instruments Incorporated 2020
 *  All rights reserved.
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

/**
 *  \file bbai64_rpmsg.c
 *
 *  \brief IPC baremetal example code (derived from TI ipc_testsetup_baremetal.c)
 *         plus Linux 6.12 remoteproc graceful shutdown.
 *
 *  Bring-up path (setup_ipc):
 *    multiproc → Ipc_init (with mailbox shutdown callback) → resource table →
 *    wait for Linux virtio → VirtIO → RPMessage → chrdev endpoint announce.
 *
 *  Shutdown path (Linux 6.12 k3 remoteproc):
 *    echo stop → kernel sends RP_MBOX_SHUTDOWN over the mailbox →
 *    IpcRpMboxCallback sets gbShutdown → main loop exits →
 *    finalize_ipc_shutdown() sends RP_MBOX_SHUTDOWN_ACK, disables IRQs, WFI.
 *
 *  Without the ACK + WFI, you get:
 *    k3_r5_rproc_stop: timeout waiting for rproc completion event
 *    can't stop rproc: -16
 *    and remoteproc state stays "running" so firmware cannot be replaced.
 */

/* ========================================================================== */
/*                             Include Files                                  */
/* ========================================================================== */

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include <ti/drv/ipc/examples/common/src/ipc_setup.h>

#include <ti/osal/HwiP.h>
#include <ti/osal/osal.h>
/* SCI Client — must init at boot if we Sciclient_deinit() on shutdown */
#include <ti/drv/sciclient/sciclient.h>

#include <ti/drv/ipc/ipc.h>
#include <ti/csl/arch/csl_arch.h>
#include <ti/csl/arch/r5/interrupt.h>

/* We use our own resource table from setup.c, not TI's ipc_rsctable.h */
// #include <ti/drv/ipc/examples/common/src/ipc_rsctable.h>
#include "../include/setup.h" /* For ti_ipc_remoteproc_ResourceTable */

#include "../../../SHARED_CODE/include/shared_rpmsg.h"


#if defined (SOC_J721E)
#define CORE_IN_TEST            10
#else
#error "Invalid SOC"
#endif


/*** TAKEN FROM ti/drv/ipc/examples/common/src/ipc_setup.h ***/
/* this should be >= RPMessage_getObjMemRequired() */
#define IPC_RPMESSAGE_OBJ_SIZE  256U

/* this should be >= RPMessage_getMessageBufferSize() */
#define IPC_RPMESSAGE_MSG_BUFFER_SIZE  (496U + 32U)

#define RPMSG_DATA_SIZE         (256U*IPC_RPMESSAGE_MSG_BUFFER_SIZE + IPC_RPMESSAGE_OBJ_SIZE)
#define VQ_BUF_SIZE             2048U

/* Vring start address for each device */
#ifdef SOC_AM65XX
#define VRING_BASE_ADDRESS      0xA2000000U
#elif defined (SOC_J7200)
#define VRING_BASE_ADDRESS      0xA4000000U
#elif defined (SOC_AM64X)
#define VRING_BASE_ADDRESS      0xA5000000U
#elif defined (SOC_J721S2)
#define VRING_BASE_ADDRESS      0xA8000000U
#elif defined (SOC_J784S4) || defined (SOC_J742S2)
#define VRING_BASE_ADDRESS      0xAC000000U
#else
/* J721E / BeagleBone AI-64 default */
#define VRING_BASE_ADDRESS      0xAA000000U
#endif

/*
 * Sentinel for "no shutdown remotecore recorded yet".
 *
 * IMPORTANT: IPC_MPU1_0 (Linux A72) is defined as 0 in the TI IPC headers.
 * A common bug is to treat remoteCoreId == 0 as "unset" and skip SHUTDOWN_ACK
 * when Linux is the requester — which is exactly the case for remoteproc stop.
 * Never gate ACK on (remoteCoreId != 0). Use this sentinel + gbShutdown instead.
 */
#define SHUTDOWN_REMOTE_UNSET   (0xFFFFFFFFU)



uint8_t  gCntrlBuf[RPMSG_DATA_SIZE] __attribute__ ((section("ipc_data_buffer"), aligned (8)));
uint8_t  sysVqBuf[VQ_BUF_SIZE]  __attribute__ ((section ("ipc_data_buffer"), aligned (8)));
uint8_t  g_sendBuf[RPMSG_DATA_SIZE * CORE_IN_TEST]  __attribute__ ((section ("ipc_data_buffer"), aligned (8)));
uint8_t  g_rspBuf[RPMSG_DATA_SIZE]  __attribute__ ((section ("ipc_data_buffer"), aligned (8)));

uint8_t *pCntrlBuf = gCntrlBuf;
uint8_t *pSendTaskBuf = g_sendBuf;
uint8_t *pRecvTaskBuf = g_rspBuf;
uint8_t *pSysVqBuf = sysVqBuf;


#ifdef BUILD_MCU2_0
uint32_t selfProcId = IPC_MCU2_0;
uint32_t remoteProc[] =
{
    IPC_MPU1_0, IPC_MCU1_0, IPC_MCU1_1, IPC_MCU2_1, IPC_MCU3_0, IPC_MCU3_1, IPC_C66X_1, IPC_C66X_2, IPC_C7X_1
};
#endif

#ifdef BUILD_MCU2_1
uint32_t selfProcId = IPC_MCU2_1;
uint32_t remoteProc[] =
{
    IPC_MPU1_0, IPC_MCU1_0, IPC_MCU1_1, IPC_MCU2_0, IPC_MCU3_0, IPC_MCU3_1, IPC_C66X_1, IPC_C66X_2, IPC_C7X_1
};
#endif

#ifdef BUILD_MCU3_0
uint32_t selfProcId = IPC_MCU3_0;
uint32_t remoteProc[] =
{
    IPC_MPU1_0, IPC_MCU1_0, IPC_MCU1_1, IPC_MCU2_0, IPC_MCU2_1, IPC_MCU3_1, IPC_C66X_1, IPC_C66X_2, IPC_C7X_1
};
#endif

#ifdef BUILD_MCU3_1
uint32_t selfProcId = IPC_MCU3_1;
uint32_t remoteProc[] =
{
    IPC_MPU1_0, IPC_MCU1_0, IPC_MCU1_1, IPC_MCU2_0, IPC_MCU2_1, IPC_MCU3_0, IPC_C66X_1, IPC_C66X_2, IPC_C7X_1
};
#endif


uint32_t *pRemoteProcArray = remoteProc;
uint32_t  gNumRemoteProc = sizeof(remoteProc)/sizeof(uint32_t);

RPMessage_Handle gHandleArray[CORE_IN_TEST];
uint32_t         gEndptArray[CORE_IN_TEST];



RPMessage_Handle *pHandleArray = gHandleArray;
uint32_t *pEndptArray = gEndptArray;



/* Size of message */
#define MSGSIZE 256U

/* Service name to be registered for chrdev end point */
#define SERVICE_CHRDEV RPMSG_CHAR_DEVICE_NAME
/* End point number to be used for chrdev end point */
#define ENDPT_CHRDEV RPMSG_CHAR_ENDPOINT

uint32_t gRecvTaskBufIdx = 0;

uint32_t rpmsgDataSize = RPMSG_DATA_SIZE;

volatile uint32_t gMessagesReceived = 0;

/* Set by IpcRpMboxCallback when Linux (or another core) requests remoteproc stop. */
volatile uint32_t gbShutdown = 0U;
/* Core that sent RP_MBOX_SHUTDOWN — where we must send SHUTDOWN_ACK. */
volatile uint32_t gbShutdownRemotecoreID = SHUTDOWN_REMOTE_UNSET;

/*
 * Cached chrdev RPMessage handle so the mailbox ISR callback can unblock any
 * blocking recv without touching printf or doing heavy work in IRQ context.
 */
static RPMessage_Handle gChrdevHandle = NULL;

/* Non-zero after a successful Sciclient_init(); gates Sciclient_deinit(). */
static int gSciclientReady = 0;

// #define DEBUG_PRINT

/*
 * Ipc_mailboxSend lives in ipc_mailbox.h / the IPC library. It is not declared
 * in the public ipc.h, so TI's own examples (ipc_testsetup.c) use an extern.
 * We need it to send RP_MBOX_SHUTDOWN_ACK back to Linux.
 */
extern int32_t Ipc_mailboxSend(uint32_t selfId, uint32_t remoteProcId, uint32_t val,
                               uint32_t timeoutCnt);

/*
 * Sciclient talks to the Device Manager / SYSFW. On shutdown we call
 * Sciclient_deinit() (matching TI's graceful-shutdown examples). If Sciclient
 * was never initialized, that deinit can hang *before* we get to send
 * SHUTDOWN_ACK — which looks exactly like the remoteproc -16 timeout.
 * So init early, even if we barely use Sciclient during normal run.
 */
static void ipc_init_sciclient(void)
{
    Sciclient_ConfigPrms_t config;

    /* Now reinitialize it as default parameter (same pattern as TI main_baremetal.c) */
    Sciclient_configPrmsInit(&config);

    if (Sciclient_init(&config) == CSL_PASS) {
        gSciclientReady = 1;
        printf("R5: Sciclient_init OK\n");
    } else {
        gSciclientReady = 0;
        printf("R5: Sciclient_init failed (deinit on shutdown may hang)\n");
    }
}

/*
 * Mailbox / remoteproc control-message callback registered via
 * initPrms.rpMboxMsgFxn.
 *
 * The IPC LLD invokes this from mailbox interrupt context when it sees a
 * special RP_MBOX_* value (as opposed to a normal virtio kick).
 *
 * Rules for this callback:
 *   - Set flags / wake waiters only. Do NOT printf here (IRQ context; can
 *     re-enter or spam the UART while Linux is tearing the bus down).
 *   - Do NOT send SHUTDOWN_ACK from here — that belongs on the main-thread
 *     finalize path after best-effort cleanup, so we never skip ACK because
 *     some HW deinit failed.
 *
 * On Linux 6.12, `echo stop` into remoteproc state causes the kernel to send
 * IPC_RP_MBOX_SHUTDOWN and then wait for IPC_RP_MBOX_SHUTDOWN_ACK.
 */
static void IpcRpMboxCallback(uint32_t remoteCoreId, uint32_t msgVal)
{
    if (msgVal == IPC_RP_MBOX_SHUTDOWN) {
        /*
         * Do not gate on remoteCoreId != 0.
         * Linux is IPC_MPU1_0 == 0; skipping ACK for that id is the silent bug
         * that leaves remoteproc stuck in "running".
         */
        gbShutdown = 1U;
        gbShutdownRemotecoreID = remoteCoreId;

        /* Wake anything blocked in RPMessage_recv so the main loop can exit. */
        if (gChrdevHandle != NULL) {
            RPMessage_unblock(gChrdevHandle);
        }
    }
}

/*
 * This function is the callback function the ipc lld library calls when a
 * message is received (normal RPMSG / virtio path — not mailbox SHUTDOWN).
 */
static void IpcTestBaremetalNewMsgCb(uint32_t srcEndPt, uint32_t procId)
{
    (void)srcEndPt;
    (void)procId;
    /* Add code here to take action on any incoming messages */
    gMessagesReceived++;
    return;
}

static void IpcTestPrint(const char *str)
{
    printf("%s", str);

    return;
}

uint32_t Ipc_exampleVirtToPhyFxn(const void *virtAddr)
{
    return ((uint32_t)virtAddr);
}

void *Ipc_examplePhyToVirtFxn(uint32_t phyAddr)
{
    return ((void *)phyAddr);
}

/* Polled by main.c / example_rpmsg_talk.c so the app loop can exit cleanly. */
int ipc_shutdown_requested(void)
{
    return (gbShutdown != 0U) ? 1 : 0;
}

/*
 * Main-thread finalize after gbShutdown is set. Order matters:
 *
 *   1. Send SHUTDOWN_ACK to whoever requested stop (usually Linux MPU).
 *   2. Sciclient_deinit (only if we inited).
 *   3. Disable mailbox + VIM IRQs, then HwiP_disable.
 *   4. WFI forever — Linux considers the core stopped once ACK is seen and
 *      the core is idle in WFI.
 *
 * Never return early before ACK. This mirrors TI ipc_testsetup.c's shutdown
 * finalize, adapted for baremetal, with ACK intentionally *before*
 * Sciclient_deinit so a hung deinit cannot prevent the ACK.
 *
 * Does not return.
 */
void finalize_ipc_shutdown(void)
{
    uint32_t loopCnt;
    int32_t ackStatus = IPC_EFAIL;

    printf("R5: remoteproc shutdown finalize (remoteCoreId=%lu)\n",
           (unsigned long)gbShutdownRemotecoreID);

    if (gbShutdownRemotecoreID != SHUTDOWN_REMOTE_UNSET) {
        /* ACK the suspend / stop message — this is what clears the -16 timeout. */
        ackStatus = Ipc_mailboxSend(selfProcId, gbShutdownRemotecoreID,
                                    IPC_RP_MBOX_SHUTDOWN_ACK, 1U);
        printf("R5: SHUTDOWN_ACK status=%ld\n", (long)ackStatus);
    } else {
        printf("R5: SHUTDOWN_ACK skipped (no remotecore id)\n");
    }

    if (gSciclientReady) {
        (void)Sciclient_deinit();
        gSciclientReady = 0;
    }

    if (gbShutdownRemotecoreID != SHUTDOWN_REMOTE_UNSET) {
        Ipc_mailboxDisableNewMsgInt((uint16_t)selfProcId,
                                    (uint16_t)gbShutdownRemotecoreID);
    }

    /* Disable/Clear pending Interrupts in VIM (same idea as Reset_Handler bring-up). */
    for (loopCnt = 0U; loopCnt < R5_VIM_INTR_NUM; loopCnt++) {
        CSL_vimSetIntrEnable((CSL_vimRegs *)(uintptr_t)CSL_MAIN_DOMAIN_VIM_BASE_ADDR,
                             loopCnt, false);
        CSL_vimClrIntrPending((CSL_vimRegs *)(uintptr_t)CSL_MAIN_DOMAIN_VIM_BASE_ADDR,
                              loopCnt);
    }

    (void)HwiP_disable();

    /* For ARM R cores — park here until Linux resets / reloads the core. */
    for (;;) {
        __asm__ __volatile__("wfi" ::: "memory");
    }
}

int32_t setup_ipc(RPMessage_Handle *handle_chrdev, uint32_t *myEndPt)
{
    /* Step1 : Initialize the multiproc */
    Ipc_InitPrms initPrms;

    /* Required before any Sciclient_deinit() on the shutdown path. */
    ipc_init_sciclient();

    if (IPC_SOK == Ipc_mpSetConfig(selfProcId, gNumRemoteProc, pRemoteProcArray))
    {
        printf("IPC_echo_test (core : %s) .....\r\n", Ipc_mpGetSelfName());

        /* Initialize params with defaults */
        IpcInitPrms_init(0U, &initPrms);

        initPrms.newMsgFxn = &IpcTestBaremetalNewMsgCb;
        initPrms.virtToPhyFxn = &Ipc_exampleVirtToPhyFxn;
        initPrms.phyToVirtFxn = &Ipc_examplePhyToVirtFxn;
        initPrms.printFxn = &IpcTestPrint;
        /*
         * Without rpMboxMsgFxn, RP_MBOX_SHUTDOWN from Linux is ignored and
         * remoteproc stop times out (-16) on kernel 6.12+.
         */
        initPrms.rpMboxMsgFxn = &IpcRpMboxCallback;

        if (IPC_SOK != Ipc_init(&initPrms))
        {
            return -1;
        }
    }else{
        printf("NOT GOOD: %s:%d\r\n", __FILE__, __LINE__);
        return -1;
    }
#ifdef DEBUG_PRINT
    printf("Required Local memory for Virtio_Object = %ld\r\n",
           gNumRemoteProc * Ipc_getVqObjMemoryRequiredPerCore());
#endif

    Ipc_loadResourceTable((void *)&ti_ipc_remoteproc_ResourceTable);

    /* Wait for Linux VDev ready... */
    for (uint32_t t = 0; t < gNumRemoteProc; t++)
    {
        while (!Ipc_isRemoteReady(pRemoteProcArray[t]))
        {
            // Task_sleep(100);
            /* Allow remoteproc stop even if we are still waiting on Linux. */
            if (ipc_shutdown_requested()) {
                return -1;
            }
        }
    }
    printf("Linux VDEV ready now .....\n");

    /* Step2 : Initialize Virtio */
    Ipc_VirtIoParams vqParam;
    vqParam.vqObjBaseAddr = (void *)pSysVqBuf;
    vqParam.vqBufSize = gNumRemoteProc * Ipc_getVqObjMemoryRequiredPerCore();
    vqParam.vringBaseAddr = (void *)VRING_BASE_ADDRESS;
    vqParam.vringBufSize = IPC_VRING_BUFFER_SIZE;
    vqParam.timeoutCnt = 100; /* Wait for counts */
    Ipc_initVirtIO(&vqParam);

    /* Step 3: Initialize RPMessage */
    RPMessage_Params cntrlParam;

#ifdef DEBUG_PRINT
    printf("Required Local memory for RPMessage Object = %ld\n",
           RPMessage_getObjMemRequired());
#endif

    /* Initialize the param */
    RPMessageParams_init(&cntrlParam);

    /* Set memory for HeapMemory for control task */
    cntrlParam.buf = pCntrlBuf;
    cntrlParam.bufSize = rpmsgDataSize;
    cntrlParam.stackBuffer = NULL;
    cntrlParam.stackSize = 0U;
    RPMessage_init(&cntrlParam);

    // EVERYTHING BELOW HERE SHOULD MAYBE ME IN IT"S OWN FUNCTION

    

    int32_t status = 0;

    /* Allocate a buffer for receiving the message */
    void *buf = NULL;
    buf = &pRecvTaskBuf[gRecvTaskBufIdx++ * rpmsgDataSize];
    if (buf == NULL)
    {
        printf("%s: buffer allocation failed\n", Ipc_mpGetSelfName());
        return -1;
    }

    RPMessage_Params params;
    RPMessageParams_init(&params);
    params.requestedEndpt = ENDPT_CHRDEV;
    params.buf = buf;
    params.bufSize = rpmsgDataSize;

    *handle_chrdev = RPMessage_create(&params, myEndPt);
    if (!(*handle_chrdev))
    {
        printf("%s: Failed to create chrdev endpoint\n", Ipc_mpGetSelfName());
        return -1;
    }

    /* Stash for IpcRpMboxCallback → RPMessage_unblock on SHUTDOWN. */
    gChrdevHandle = *handle_chrdev;

    status = RPMessage_announce(RPMESSAGE_ALL, *myEndPt, SERVICE_CHRDEV);
    if (status != IPC_SOK)
    {
        printf("%s: RPMessage_announce() for %s failed\n", Ipc_mpGetSelfName(), SERVICE_CHRDEV);
        return -1;
    }else{
        printf("%s: RPMessage_announce() for %s WORKED!\n", Ipc_mpGetSelfName(), SERVICE_CHRDEV);
    }

    return 0;
}
