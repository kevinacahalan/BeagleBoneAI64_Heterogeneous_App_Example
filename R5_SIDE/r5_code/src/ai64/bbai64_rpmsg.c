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
 *  \file ipc_testsetup_baremetal.c
 *
 *  \brief IPC baremetal example code
 *
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
/* SCI Client */
#include <ti/drv/sciclient/sciclient.h>

#include <ti/drv/ipc/ipc.h>

// #include <ti/drv/ipc/examples/common/src/ipc_rsctable.h> // We are using our own from setup.c
#include "../include/setup.h" // For ipc rsctable



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
#define VRING_BASE_ADDRESS      0xAA000000U
#endif



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
#define SERVICE_CHRDEV "rpmsg_chrdev"
/* End point number to be used for chrdev end point */
#define ENDPT_CHRDEV 14U

typedef struct Ipc_TestParams_s
{
    uint32_t endPt;
    char name[32];
} Ipc_TestParams;

typedef enum
{
    UNKNOWN = 0,
    FUNCTION_A = 1,
    FUNCTION_B = 2,
} FUNCTION_TAG;

typedef struct
{
    FUNCTION_TAG tag;
    union
    {
        struct
        {
            int a;
            int b;
        } function_a; // function_a(int a, int b)
        struct
        {
            float a;
            float b;
            float c;
        } function_b; // function_b(float a, float b, float c)
    };
} request_tagged_union_t;

Ipc_TestParams service_chrdev = {ENDPT_CHRDEV, SERVICE_CHRDEV};

uint32_t gRecvTaskBufIdx = 0;

uint32_t rpmsgDataSize = RPMSG_DATA_SIZE;

volatile uint32_t gMessagesReceived = 0;

// #define DEBUG_PRINT

int function_a(int a, int b) {
    printf("%s: Adding %d + %d inside R5 called from linux\n", Ipc_mpGetSelfName(), a, b);
    return a + b;
}

float function_b(float a, float b, float c) {
    printf("%s: Adding %f + %f + %f inside R5 called from linux\n", Ipc_mpGetSelfName(), a, b, c);
    return a + b + c;
}

// returns IPC_ETIMEOUT on no message, otherwise returns other stuff
int process_one_rproc_message(RPMessage_Handle *handle_chrdev, uint32_t *myEndPt, uint32_t *remoteEndPt, uint32_t *remoteProcId) {
    int32_t status = 0;
    uint16_t rxLen = 0;
    request_tagged_union_t req;
    /* Wait for a new message */

    /* Expect the incoming message to be an request_tagged_union_t */
    rxLen = sizeof(request_tagged_union_t);
    // If I make this a RPMessage_recv with a timeout of 0, some messages are double processed...
    status = RPMessage_recvNb(*handle_chrdev, (Ptr)&req, &rxLen, remoteEndPt, remoteProcId);
    if (status != IPC_SOK)
    {
        // Will return here if there is no message
        return status; // much of the time returns IPC_ETIMEOUT..(error -4 for no message)
    }

    if (rxLen != sizeof(request_tagged_union_t))
    {
        printf("%s: Unexpected message size: %u bytes\n", Ipc_mpGetSelfName(), rxLen);
        return 1333;
    }

    // Figure out which function to call
    switch (req.tag)
    {
    case FUNCTION_A:
        int result = function_a(req.function_a.a, req.function_a.b);
        status = RPMessage_send(*handle_chrdev, *remoteProcId, *remoteEndPt, *myEndPt, &result, sizeof(result));
        break;
    case FUNCTION_B:
        float f_result = function_b(req.function_b.a, req.function_b.b, req.function_b.c);
        status = RPMessage_send(*handle_chrdev, *remoteProcId, *remoteEndPt, *myEndPt, &f_result, sizeof(f_result));
        break;

    default:
        printf("Unknown req.tag %d\b", req.tag);
        return 55;
        break;
    }

    if (status != IPC_SOK)
    {
        printf("%s: Failed to send result, error: %ld\n", Ipc_mpGetSelfName(), status);
    }

    return 0;
}

/*
 * This function is the callback function the ipc lld library calls when a
 * message is received.
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

int32_t setup_ipc(RPMessage_Handle *handle_chrdev, uint32_t *myEndPt)
{
    /* Step1 : Initialize the multiproc */
    Ipc_InitPrms initPrms;
    if (IPC_SOK == Ipc_mpSetConfig(selfProcId, gNumRemoteProc, pRemoteProcArray))
    {
        printf("IPC_echo_test (core : %s) .....\r\n", Ipc_mpGetSelfName());

        /* Initialize params with defaults */
        IpcInitPrms_init(0U, &initPrms);

        initPrms.newMsgFxn = &IpcTestBaremetalNewMsgCb;
        initPrms.virtToPhyFxn = &Ipc_exampleVirtToPhyFxn;
        initPrms.phyToVirtFxn = &Ipc_examplePhyToVirtFxn;
        initPrms.printFxn = &IpcTestPrint;

        if (IPC_SOK != Ipc_init(&initPrms))
        {
            return -1;
        }
    }
#ifdef DEBUG_PRINT
    printf("Required Local memory for Virtio_Object = %ld\r\n",
           numProc * Ipc_getVqObjMemoryRequiredPerCore());
#endif

    Ipc_loadResourceTable((void *)&ti_ipc_remoteproc_ResourceTable);

    /* Wait for Linux VDev ready... */
    for (uint32_t t = 0; t < gNumRemoteProc; t++)
    {
        while (!Ipc_isRemoteReady(pRemoteProcArray[t]))
        {
            // Task_sleep(100);
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

    RPMessage_Params params;

    int32_t status = 0;
    void *buf = NULL;
    uint32_t bufSize = rpmsgDataSize;

    /* Allocate a buffer for receiving the message */
    buf = &pRecvTaskBuf[gRecvTaskBufIdx++ * rpmsgDataSize];
    if (buf == NULL)
    {
        printf("%s: buffer allocation failed\n", Ipc_mpGetSelfName());
        return -1;
    }

    RPMessageParams_init(&params);
    params.requestedEndpt = ENDPT_CHRDEV;
    params.buf = buf;
    params.bufSize = bufSize;

    *handle_chrdev = RPMessage_create(&params, myEndPt);
    if (!(*handle_chrdev))
    {
        printf("%s: Failed to create chrdev endpoint\n", Ipc_mpGetSelfName());
        return -1;
    }

    status = RPMessage_announce(RPMESSAGE_ALL, *myEndPt, SERVICE_CHRDEV);
    if (status != IPC_SOK)
    {
        printf("%s: RPMessage_announce() for %s failed\n", Ipc_mpGetSelfName(), SERVICE_CHRDEV);
        return -1;
    }

    return 0;
}

