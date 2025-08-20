#include <sys/select.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <stdint.h>
#include <stddef.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <stdbool.h>
#include <linux/rpmsg.h>

#include "../../../SHARED_CODE/include/random_utils.h"
#include <ti_rpmsg_char.h>
#include <rpmsg_char_example.h>
#include "../../../SHARED_CODE/include/shared_rpmsg.h"

// Helpers for send/recv
static int send_msg(int fd, void *msg, int len) {
    int ret = write(fd, msg, len);
    if (ret < 0) {
        perror("Can't write to rpmsg endpt device");
    }
    return ret;
}

static int recv_msg(int fd, void *buf, int len, int *recv_len) {
    int ret = read(fd, buf, len);
    if (ret < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            *recv_len = 0;  // No data available
            return 0;
        }
        perror("Can't read from rpmsg endpt device");
        return -1;
    }
    *recv_len = ret;
    return 0;
}

// Cleanup
static int rpmsg_cleanup(rpmsg_char_dev_t *rcdev) {
    return rpmsg_char_close(rcdev);
}

// Example Linux functions callable from R5
static float linux_function_x(int param) {
    float rt = param * 1.5f;
    printf("linux_function_x: Got %d from R5, returning %f\n", param, rt);
    return rt;
}

static int linux_function_y(float param) {
    int rt = (int)(param * 2.0f);
    printf("linux_function_y: Got %f from R5, returning %d\n", param, rt);
    return rt;
}

// Handle incoming request from R5
static void handle_request_linux(MESSAGE *req_msg, rpmsg_char_dev_t *rcdev) {
    request_data_t *req = &req_msg->data.request;
    MESSAGE resp = {0};
    resp.tag = MESSAGE_RESPONSE;
    resp.request_id = req_msg->request_id;
    resp.data.response.function_tag = req->function_tag;

    switch (req->function_tag) {
        case FUNCTION_X:
            resp.data.response.result.result_function_x = linux_function_x(req->params.function_x.param);
            break;
        case FUNCTION_Y:
            resp.data.response.result.result_function_y = linux_function_y(req->params.function_y.param);
            break;
        default:
            fprintf(stderr, "Linux: Unknown function tag %s\n", function_tag_to_string(req->function_tag));
            return;
    }

    burn_time_pretending_to_do_stuff(800, 1200);  // Simulate work
    send_msg(rcdev->fd, &resp, sizeof(MESSAGE));
    printf("Linux: Sent response for tag %s\n", function_tag_to_string(req->function_tag));
}

// Handle incoming command (non-blocking, no response)
static void handle_command_linux(MESSAGE *cmd_msg) {
    // Example: Just log or perform action without response
    printf("Linux: Received non-blocking command with tag %s\n", function_tag_to_string(cmd_msg->data.request.function_tag));
    // Add logic for command-specific actions here
}

// Blocking call to R5 function A
static int call_function_a_blocking(rpmsg_char_dev_t *rcdev, int a, int b) {
    static uint32_t req_id_counter = 1;
    uint32_t req_id = req_id_counter++;

    MESSAGE req = {0};
    req.tag = MESSAGE_REQUEST;
    req.request_id = req_id;
    req.data.request.function_tag = FUNCTION_A;
    req.data.request.params.function_a.a = a;
    req.data.request.params.function_a.b = b;

    if (send_msg(rcdev->fd, &req, sizeof(MESSAGE)) < 0) {
        return -1;
    }

    time_t start = time(NULL);
    while (time(NULL) - start < 5) {  // 5s timeout
        MESSAGE msg;
        int len = 0;
        if (recv_msg(rcdev->fd, &msg, sizeof(MESSAGE), &len) == 0 && len == sizeof(MESSAGE)) {
            if (msg.tag == MESSAGE_RESPONSE && msg.request_id == req_id) {
                if (msg.data.response.function_tag == FUNCTION_A) {
                    printf("Linux: Got response for FUNCTION_A: %d\n", msg.data.response.result.result_function_a);
                    return msg.data.response.result.result_function_a;
                }
            } else if (msg.tag == MESSAGE_REQUEST) {
                handle_request_linux(&msg, rcdev);  // Handle concurrent request
            } else if (msg.tag == MESSAGE_COMMAND) {
                handle_command_linux(&msg);  // Handle concurrent command
            } else if (msg.tag == MESSAGE_RESPONSE) {
                // printf("R5: Received unexpected response (request_id %lu)\n", msg.request_id);  // Comment out or make verbose-only
                // Optional: Add a queue for pending responses if you want to match later, but for now, log quietly
                printf("R5: Note: Orphaned response id %u (from prior call)\n", msg.request_id);
            }
        }
        usleep(10000);  // Small sleep to avoid CPU spin
    }
    fprintf(stderr, "Linux: Timeout waiting for FUNCTION_A response\n");
    return -1;
}

// Blocking call to R5 function B (fixed bug)
static float call_function_b_blocking(rpmsg_char_dev_t *rcdev, float a, float b, float c) {
    static uint32_t req_id_counter = 1;
    uint32_t req_id = req_id_counter++;

    MESSAGE req = {0};
    req.tag = MESSAGE_REQUEST;
    req.request_id = req_id;
    req.data.request.function_tag = FUNCTION_B;
    req.data.request.params.function_b.a = a;
    req.data.request.params.function_b.b = b;
    req.data.request.params.function_b.c = c;

    if (send_msg(rcdev->fd, &req, sizeof(MESSAGE)) < 0) {
        return -1.0f;
    }

    time_t start = time(NULL);
    while (time(NULL) - start < 5) {
        MESSAGE msg;
        int len = 0;
        if (recv_msg(rcdev->fd, &msg, sizeof(MESSAGE), &len) == 0 && len == sizeof(MESSAGE)) {
            if (msg.tag == MESSAGE_RESPONSE && msg.request_id == req_id) {
                if (msg.data.response.function_tag == FUNCTION_B) {
                    printf("Linux: Got response for FUNCTION_B: %f\n", msg.data.response.result.result_function_b);
                    return msg.data.response.result.result_function_b;
                }
            } else if (msg.tag == MESSAGE_REQUEST) {
                handle_request_linux(&msg, rcdev);
            } else if (msg.tag == MESSAGE_COMMAND) {
                handle_command_linux(&msg);
            }
        }
        usleep(10000);
    }
    fprintf(stderr, "Linux: Timeout waiting for FUNCTION_B response\n");
    return -1.0f;
}

// Non-blocking command to R5 (e.g., trigger FUNCTION_A without return)
static void send_command_a_nonblocking(rpmsg_char_dev_t *rcdev, int a, int b) {
    MESSAGE cmd = {0};
    cmd.tag = MESSAGE_COMMAND;
    cmd.data.request.function_tag = FUNCTION_A;  // Reuse structure
    cmd.data.request.params.function_a.a = a;
    cmd.data.request.params.function_a.b = b;

    send_msg(rcdev->fd, &cmd, sizeof(MESSAGE));
    printf("Linux: Sent non-blocking command for FUNCTION_A\n");
}

// Main RPC loop
static int rpmsg_char_rpc(int rproc_id, char *dev_name, unsigned int local_endpt, unsigned int remote_endpt) {
    char eptdev_name[64];
    snprintf(eptdev_name, sizeof(eptdev_name), "eptdev_name_rpmsg%d-%d", rproc_id, getpid());

    rpmsg_char_dev_t *rcdev = rpmsg_char_open(rproc_id, dev_name, local_endpt, remote_endpt, eptdev_name, 0);
    while (!rcdev) {
        sleep(2);
        rcdev = rpmsg_char_open(rproc_id, dev_name, local_endpt, remote_endpt, eptdev_name, 0);
    }
    printf("Linux: Created endpoint %s, fd=%d, port=%d\n", eptdev_name, rcdev->fd, rcdev->endpt);

    // Set non-blocking
    int flags = fcntl(rcdev->fd, F_GETFL, 0);
    fcntl(rcdev->fd, F_SETFL, flags | O_NONBLOCK);

    MESSAGE ping = {0};
    ping.tag = MESSAGE_PING;
    send_msg(rcdev->fd, &ping, sizeof(MESSAGE));
    printf("Linux: Sent startup ping\n");

    // Main loop: Poll for messages, make example calls
    while (1) {
        MESSAGE msg;
        int len = 0;
        if (recv_msg(rcdev->fd, &msg, sizeof(MESSAGE), &len) == 0 && len == sizeof(MESSAGE)) {
            if (msg.tag == MESSAGE_REQUEST) {
                handle_request_linux(&msg, rcdev);
            } else if (msg.tag == MESSAGE_COMMAND) {
                handle_command_linux(&msg);
            } else if (msg.tag == MESSAGE_RESPONSE) {
                printf("Linux: Received unexpected response (request_id %u)\n", msg.request_id);
            }
        }

        // Example blocking calls
        call_function_a_blocking(rcdev, 5, 3);
        call_function_b_blocking(rcdev, 1.1f, 2.2f, 3.3f);

        // Example non-blocking command
        send_command_a_nonblocking(rcdev, 10, 20);

        burn_time_pretending_to_do_stuff(800, 1200);
    }

    return rpmsg_cleanup(rcdev);
}

int rpmsg_char_example_main(void) {
    int ret = rpmsg_char_init(NULL);
    if (ret) return ret;

    int rproc_id = R5F_MAIN0_0;
    unsigned int local_endpt = RPMSG_ADDR_ANY;
    unsigned int remote_endpt = RPMSG_CHAR_ENDPOINT;
    char *dev_name = RPMSG_CHAR_DEVICE_NAME;

    rpmsg_char_rpc(rproc_id, dev_name, local_endpt, remote_endpt);
    rpmsg_char_exit();
    return 0;
}