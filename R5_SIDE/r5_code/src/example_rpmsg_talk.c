#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <ti/osal/osal.h>

#include <example_rpmsg_talk.h>
#include <ai64/bbai64_clocks.h>

#include "../../../SHARED_CODE/include/random_utils.h"

#define TIMEOUT_DEFAULT_S 10U

static uint32_t myEndPt = 0;
static uint32_t remoteEndPt = 0;
static uint32_t remoteProcId = IPC_MPU1_0;
static RPMessage_Handle handle = NULL;
static uint32_t req_id_counter = (uint32_t)-1;
static bool linux_ping_resolved_once = false;

static int send_ping_r5(bool log_ping);

static inline uint32_t get_next_req_id(void)
{
    return req_id_counter--;
}

static void mark_linux_peer_stale(const char *reason, int32_t status, uint32_t failed_remote_endpt)
{
    if (failed_remote_endpt == 0U || remoteEndPt == failed_remote_endpt) {
        remoteEndPt = 0;
    }
    remoteProcId = IPC_MPU1_0;
    linux_ping_resolved_once = false;

    printf("R5: Linux RPMSG peer invalidated (%s, status=%ld); waiting for Linux ping\n",
           reason != NULL ? reason : "unknown", status);
}

static int send_ping_r5(bool log_ping)
{
    MESSAGE ping = {0};
    int32_t status;

    if (handle == NULL || remoteEndPt == 0U) {
        return -1;
    }

    ping.tag = MESSAGE_PING;
    status = RPMessage_send(handle, remoteProcId, remoteEndPt, myEndPt, &ping, sizeof(ping));
    if (status != IPC_SOK) {
        mark_linux_peer_stale("send ping failed", status, remoteEndPt);
        return -1;
    }

    if (log_ping) {
        printf("R5: Sent ping to Linux (remoteEndPt=%lu)\n", remoteEndPt);
    }

    return 0;
}

static int function_a(int a, int b)
{
    return a + b;
}

static float function_b(float a, float b, float c)
{
    return a + b + c;
}

static int handle_request_r5(MESSAGE *req_msg, uint32_t response_remote_endpt, uint32_t response_remote_proc)
{
    request_data_t *req = &req_msg->data.request;
    MESSAGE resp = {0};
    int32_t status;

    resp.tag = MESSAGE_RESPONSE;
    resp.request_id = req_msg->request_id;
    resp.data.response.function_tag = req->function_tag;

    switch (req->function_tag) {
    case FUNCTION_A:
        resp.data.response.result.result_function_a = function_a(req->params.function_a.a, req->params.function_a.b);
        printf("R5: id=%-4ld %s: a=%d, b=%d, result=%d\n",
               resp.request_id,
               function_tag_to_string(req->function_tag),
               req->params.function_a.a,
               req->params.function_a.b,
               resp.data.response.result.result_function_a);
        break;
    case FUNCTION_B:
        resp.data.response.result.result_function_b = function_b(req->params.function_b.a,
                                                                 req->params.function_b.b,
                                                                 req->params.function_b.c);
        printf("R5: id=%-4ld %s: a=%.3f, b=%.3f, c=%.3f, result=%.3f\n",
               resp.request_id,
               function_tag_to_string(req->function_tag),
               req->params.function_b.a,
               req->params.function_b.b,
               req->params.function_b.c,
               resp.data.response.result.result_function_b);
        break;
    default:
        printf("R5: id=%-4ld Unknown request tag %s\n",
               resp.request_id,
               function_tag_to_string(req->function_tag));
        return -1;
    }

    status = RPMessage_send(handle, response_remote_proc, response_remote_endpt, myEndPt, &resp, sizeof(resp));
    if (status != IPC_SOK) {
        printf("R5: id=%-4ld Send failed (%ld); retrying once...\n", resp.request_id, status);
        Osal_delay(500);
        status = RPMessage_send(handle, response_remote_proc, response_remote_endpt, myEndPt, &resp, sizeof(resp));
        if (status != IPC_SOK) {
            printf("R5: id=%-4ld Send response failed (%ld)\n", resp.request_id, status);
            mark_linux_peer_stale("send response failed", status, response_remote_endpt);
            return -1;
        }
    }

    printf("R5: id=%-4ld Sent response for %s\n",
           resp.request_id,
           function_tag_to_string(req->function_tag));
    return 0;
}

static void handle_command_r5(const MESSAGE *cmd_msg)
{
    switch (cmd_msg->data.request.function_tag) {
    case FUNCTION_A:
        function_a(cmd_msg->data.request.params.function_a.a, cmd_msg->data.request.params.function_a.b);
        break;
    default:
        printf("R5: id=%-4ld Unknown command tag %s\n",
               cmd_msg->request_id,
               function_tag_to_string(cmd_msg->data.request.function_tag));
        return;
    }

    printf("R5: id=%-4ld Handled non-blocking command %s\n",
           cmd_msg->request_id,
           function_tag_to_string(cmd_msg->data.request.function_tag));
}

static REMOTE_RETURN send_blocking_request_r5(MESSAGE *request, int timeout_seconds, void *result_out, size_t result_size)
{
    uint32_t send_remote_endpt;
    int32_t status;
    uint64_t start;

    if (!example_rpmsg_peer_ready()) {
        printf("R5: id=%-4ld No Linux RPMSG peer for %s yet\n",
               request->request_id,
               function_tag_to_string(request->data.request.function_tag));
        return (REMOTE_RETURN){ .error = -1, .rt = NULL };
    }

    send_remote_endpt = remoteEndPt;
    status = RPMessage_send(handle, remoteProcId, send_remote_endpt, myEndPt, request, sizeof(*request));
    if (status != IPC_SOK) {
        printf("R5: id=%-4ld Send failed (%ld); retrying once...\n", request->request_id, status);
        Osal_delay(500);
        status = RPMessage_send(handle, remoteProcId, send_remote_endpt, myEndPt, request, sizeof(*request));
        if (status != IPC_SOK) {
            printf("R5: id=%-4ld Send request failed (%ld)\n", request->request_id, status);
            mark_linux_peer_stale("send request failed", status, send_remote_endpt);
            return (REMOTE_RETURN){ .error = -1, .rt = NULL };
        }
    }

    printf("R5: id=%-4ld Sent blocking call for %s\n",
           request->request_id,
           function_tag_to_string(request->data.request.function_tag));

    start = get_gtc_as_u64seconds();
    while ((get_gtc_as_u64seconds() - start) < (uint64_t)timeout_seconds) {
        MESSAGE msg = {0};
        uint16_t len = sizeof(msg);
        uint32_t srcEndPt = remoteEndPt;
        uint32_t srcProc = remoteProcId;

        status = RPMessage_recvNb(handle, &msg, &len, &srcEndPt, &srcProc);
        if (status == IPC_SOK && len == sizeof(msg)) {
            remoteEndPt = srcEndPt;
            remoteProcId = srcProc;

            if (msg.tag == MESSAGE_RESPONSE && msg.request_id == request->request_id) {
                if (msg.data.response.function_tag != request->data.request.function_tag) {
                    printf("R5: id=%-4ld Unexpected response tag %s while waiting for %s\n",
                           msg.request_id,
                           function_tag_to_string(msg.data.response.function_tag),
                           function_tag_to_string(request->data.request.function_tag));
                    return (REMOTE_RETURN){ .error = -1, .rt = NULL };
                }

                memcpy(result_out, &msg.data.response.result, result_size);
                return (REMOTE_RETURN){ .error = 0, .rt = result_out };
            }

            if (msg.tag == MESSAGE_REQUEST) {
                handle_request_r5(&msg, srcEndPt, srcProc);
            } else if (msg.tag == MESSAGE_COMMAND) {
                handle_command_r5(&msg);
            } else if (msg.tag == MESSAGE_RESPONSE) {
                printf("R5: Note: Orphaned response id=%ld (from prior call)\n", msg.request_id);
            } else if (msg.tag == MESSAGE_PING) {
                printf("R5: Received Linux ping, endpoints refreshed (remoteEndPt=%lu)\n", remoteEndPt);
                if (!linux_ping_resolved_once) {
                    linux_ping_resolved_once = true;
                    send_ping_r5(true);
                }
            }
        } else if (status != IPC_ETIMEOUT) {
            printf("R5: id=%-4ld Poll error %ld\n", request->request_id, status);
        }

        Osal_delay(1);
    }

    printf("R5: id=%-4ld Timeout waiting for %s response\n",
           request->request_id,
           function_tag_to_string(request->data.request.function_tag));
    mark_linux_peer_stale("response timeout", IPC_ETIMEOUT, send_remote_endpt);
    return (REMOTE_RETURN){ .error = -1, .rt = NULL };
}

static REMOTE_RETURN send_nonblocking_command_r5(MESSAGE *command)
{
    uint32_t send_remote_endpt;
    int32_t status;

    if (!example_rpmsg_peer_ready()) {
        printf("R5: id=%-4ld No Linux RPMSG peer for %s yet\n",
               command->request_id,
               function_tag_to_string(command->data.request.function_tag));
        return (REMOTE_RETURN){ .error = -1, .rt = NULL };
    }

    send_remote_endpt = remoteEndPt;
    status = RPMessage_send(handle, remoteProcId, send_remote_endpt, myEndPt, command, sizeof(*command));
    if (status != IPC_SOK) {
        printf("R5: id=%-4ld Send failed (%ld); retrying once...\n", command->request_id, status);
        Osal_delay(500);
        status = RPMessage_send(handle, remoteProcId, send_remote_endpt, myEndPt, command, sizeof(*command));
        if (status != IPC_SOK) {
            printf("R5: id=%-4ld Failed to send non-blocking command for %s (%ld)\n",
                   command->request_id,
                   function_tag_to_string(command->data.request.function_tag),
                   status);
            mark_linux_peer_stale("send command failed", status, send_remote_endpt);
            return (REMOTE_RETURN){ .error = -1, .rt = NULL };
        }
    }

    printf("R5: id=%-4ld Sent non-blocking command for %s\n",
           command->request_id,
           function_tag_to_string(command->data.request.function_tag));
    return (REMOTE_RETURN){ .error = 0, .rt = NULL };
}

int setup_r5_example_talk(void)
{
    int32_t ret = setup_ipc(&handle, &myEndPt);

    if (ret != 0) {
        printf("R5: setup_ipc failed with %ld\n", ret);
        return ret;
    }

    remoteEndPt = 0;
    remoteProcId = IPC_MPU1_0;
    linux_ping_resolved_once = false;
    printf("R5: RPMSG setup complete, waiting for Linux ping\n");
    return 0;
}

int example_rpmsg_peer_ready(void)
{
    return handle != NULL && remoteEndPt != 0U;
}

int teardown_r5_example_talk(void)
{
    return 0;
}

int process_linux_messages(int max_messages)
{
    int processed = 0;

    if (handle == NULL) {
        return 0;
    }

    for (;;) {
        MESSAGE msg = {0};
        uint16_t len = sizeof(msg);
        uint32_t srcEndPt = remoteEndPt;
        uint32_t srcProc = remoteProcId;
        int32_t status = RPMessage_recvNb(handle, &msg, &len, &srcEndPt, &srcProc);

        if (status != IPC_SOK) {
            return processed;
        }

        if (len != sizeof(msg)) {
            printf("R5: Bad message size %u\n", len);
            processed++;
            if (max_messages != 0 && processed >= max_messages) {
                return processed;
            }
            continue;
        }

        remoteEndPt = srcEndPt;
        remoteProcId = srcProc;

        if (msg.tag == MESSAGE_REQUEST) {
            handle_request_r5(&msg, srcEndPt, srcProc);
        } else if (msg.tag == MESSAGE_COMMAND) {
            handle_command_r5(&msg);
        } else if (msg.tag == MESSAGE_RESPONSE) {
            printf("R5: Received unexpected response (request_id %lu)\n", msg.request_id);
        } else if (msg.tag == MESSAGE_PING) {
            printf("R5: Received Linux ping, endpoints refreshed (remoteEndPt=%lu)\n", remoteEndPt);
            if (!linux_ping_resolved_once) {
                linux_ping_resolved_once = true;
                send_ping_r5(true);
            }
        } else {
            printf("R5: id=%lu Unknown tag %u\n", msg.request_id, (unsigned int)msg.tag);
        }

        processed++;
        if (max_messages != 0 && processed >= max_messages) {
            return processed;
        }
    }
}

REMOTE_RETURN call_linux_function_x_blocking(int param, float *result_out)
{
    MESSAGE req = {0};

    req.tag = MESSAGE_REQUEST;
    req.request_id = get_next_req_id();
    req.data.request.function_tag = FUNCTION_X;
    req.data.request.params.function_x.param = param;

    return send_blocking_request_r5(&req, TIMEOUT_DEFAULT_S, result_out, sizeof(*result_out));
}

REMOTE_RETURN call_linux_function_y_blocking(float param, int *result_out)
{
    MESSAGE req = {0};

    req.tag = MESSAGE_REQUEST;
    req.request_id = get_next_req_id();
    req.data.request.function_tag = FUNCTION_Y;
    req.data.request.params.function_y.param = param;

    return send_blocking_request_r5(&req, TIMEOUT_DEFAULT_S, result_out, sizeof(*result_out));
}

REMOTE_RETURN send_command_x_nonblocking(int param)
{
    MESSAGE cmd = {0};

    cmd.tag = MESSAGE_COMMAND;
    cmd.request_id = get_next_req_id();
    cmd.data.request.function_tag = FUNCTION_X;
    cmd.data.request.params.function_x.param = param;

    return send_nonblocking_command_r5(&cmd);
}