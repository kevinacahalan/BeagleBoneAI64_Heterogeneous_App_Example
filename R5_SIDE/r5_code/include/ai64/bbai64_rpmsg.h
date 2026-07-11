#ifndef IPC_TESTSETUP_BAREMETAL_H
#define IPC_TESTSETUP_BAREMETAL_H

#include <stdint.h>
#include <ti/drv/ipc/ipc.h>
#include "../../../SHARED_CODE/include/shared_rpmsg.h"

int32_t setup_ipc(RPMessage_Handle *handle_chrdev, uint32_t *myEndPt);

/* Non-zero once Linux remoteproc has requested RP_MBOX_SHUTDOWN. */
int ipc_shutdown_requested(void);

/*
 * Best-effort HW/IPC teardown, publish OFFLINE, send SHUTDOWN_ACK, then WFI.
 * Does not return.
 */
void finalize_ipc_shutdown(void);

#endif // IPC_TESTSETUP_BAREMETAL_H
