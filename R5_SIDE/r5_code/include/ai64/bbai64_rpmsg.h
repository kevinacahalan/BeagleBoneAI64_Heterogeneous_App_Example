#ifndef IPC_TESTSETUP_BAREMETAL_H
#define IPC_TESTSETUP_BAREMETAL_H

// Your declarations and definitions go here

#include <stdint.h>
#include <ti/drv/ipc/ipc.h>
#include "../../../SHARED_CODE/include/shared_rpmsg.h"

int32_t setup_ipc(RPMessage_Handle *handle_chrdev, uint32_t *myEndPt);
// int process_one_rproc_message(RPMessage_Handle *handle_chrdev, uint32_t *myEndPt, uint32_t *remoteEndPt, uint32_t *remoteProcId);

#endif // IPC_TESTSETUP_BAREMETAL_H
