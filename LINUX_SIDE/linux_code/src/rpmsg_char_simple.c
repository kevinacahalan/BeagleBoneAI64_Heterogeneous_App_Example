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
#include <pthread.h>
#include <time.h>
#include <stdbool.h>
#include <semaphore.h>
#include <linux/rpmsg.h>


#include <ti_rpmsg_char.h>
#include <rpmsg_char_simple.h>

#include "../../../SHARED_CODE/include/shared_rpmsg.h"

// Really should at some point move this function to it's own file
#include <time.h>
static inline void burn_time_pretending_to_do_stuff(uint32_t min_ms, uint32_t max_ms) {
    // 1) Sanity check
    if (max_ms <= min_ms) {
        struct timespec ts = {
            .tv_sec  = min_ms / 1000,
            .tv_nsec = (min_ms % 1000) * 1000000U
        };
        nanosleep(&ts, NULL);
        return;
    }

    // 2) One‑time seed
    static int seeded = 0;
    if (!seeded) {
        srandom((unsigned)time(NULL) ^ (unsigned)getpid());
        seeded = 1;
    }

    // 3) Compute span and pick a random offset
    uint32_t span     = max_ms - min_ms + 1;
    uint32_t offs     = (uint32_t)random() % span;  // modulo‐bias is negligible for small spans
    uint32_t delay_ms = min_ms + offs;

    // 4) Sleep
    struct timespec req = {
        .tv_sec  = delay_ms / 1000,
        .tv_nsec = (delay_ms % 1000) * 1000000U
    };
    nanosleep(&req, NULL);
}

static int rpmsg_cleanup(rpmsg_char_dev_t *rcdev) {
    int ret = rpmsg_char_close(rcdev);
    if (ret < 0)
    {
        perror("Error closing the rpmsg endpoint device");
    }
    return ret;
}

static float linux_function_x(int param) {
    float rt = param * 1.5f;
    printf("linux_function_x: Got value %d from R5, sending back value %f\n", param, rt);
    return rt;  // Example: multiply by 1.5
}

static int linux_function_y(float param) {
    int rt = (int)(param * 2.0f);
    printf("linux_function_y: Got value %f from R5, sending back value %d\n", param, rt);
    return (int)(param * 2.0f);  // Example: multiply by 2 and convert to int
}

static void handle_request_linux(MESSAGE *req_msg, rpmsg_char_dev_t *rcdev) {
    request_data_t *req = &req_msg->data.request;
    MESSAGE response_msg = {0};
    response_msg.tag = MESSAGE_RESPONSE;
    response_msg.request_id = req_msg->request_id;
    response_msg.data.response.function_tag = req->function_tag;

    switch (req->function_tag) {
        case FUNCTION_X:
            response_msg.data.response.result.result_function_x = 
                linux_function_x(req->params.function_x.param);
            break;
        case FUNCTION_Y:
            response_msg.data.response.result.result_function_y = 
                linux_function_y(req->params.function_y.param);
            break;
        default:
            fprintf(stderr, "Linux: Unknown function tag %d\n", req->function_tag);
            return;
    }
    // This delay may potentially kickoff lockups if code elsewhere is not robust enough
    burn_time_pretending_to_do_stuff(800, 1200);

    int ret = write(rcdev->fd, &response_msg, sizeof(MESSAGE));

    if (req->function_tag == FUNCTION_X){
        printf("Sent response with rt=%f to R5 for tag %d, FUNCTION_X \n", response_msg.data.response.result.result_function_x, (int)(req->function_tag));
    } else {
        printf("Sent response to R5 for tag %d\n", (int)(req->function_tag));
    }


    if (ret < 0) {
        perror("Linux: Failed to send response");
    }
}

static int call_function_a_blocking(rpmsg_char_dev_t *rcdev, int a, int b) {
    printf("Inside call_function_a_blocking\n");
    static uint32_t request_id_counter = 1;
    uint32_t request_id = request_id_counter++;

    // Send request
    MESSAGE req_msg = {0};
    req_msg.tag = MESSAGE_REQUEST;
    req_msg.request_id = request_id;
    req_msg.data.request.function_tag = FUNCTION_A;
    req_msg.data.request.params.function_a.a = a;
    req_msg.data.request.params.function_a.b = b;

    int ret = write(rcdev->fd, &req_msg, sizeof(MESSAGE));
    if (ret < 0) {
        perror("Linux: Failed to send request");
        return 0;
    }

    // Wait for response, processing other requests
    while (1) {
        MESSAGE resp_msg;
        ret = read(rcdev->fd, &resp_msg, sizeof(MESSAGE));
        if (ret == sizeof(MESSAGE)) {
            if (resp_msg.tag == MESSAGE_RESPONSE && resp_msg.request_id == request_id) {
                if (resp_msg.data.response.function_tag == FUNCTION_A) {
                    return resp_msg.data.response.result.result_function_a;
                } else {
                    fprintf(stderr, "Linux: Mismatched function tag %d\n", resp_msg.data.response.function_tag);
                }
            } else if (resp_msg.tag == MESSAGE_REQUEST) {
                handle_request_linux(&resp_msg, rcdev);
            }
        } else if (ret < 0) {
            perror("Linux: Read error");
            break;
        }
        // Optional: Use select() or poll() for efficiency
    }
    return 0;  // Unreachable with infinite loop; add timeout in production
}

static int rpmsg_char_rpc(int rproc_id, char *dev_name,
                          unsigned int local_endpt, unsigned int remote_endpt)
{
    char eptdev_name[64] = {0};
    rpmsg_char_dev_t *rcdev = NULL;
    int flags = 0;

    /* Create a unique endpoint name */
    snprintf(eptdev_name, sizeof(eptdev_name), "eptdev_name_rpmsg-char-%d-%d", rproc_id, getpid());

    /* Open the rpmsg-char device */
    rcdev = rpmsg_char_open(rproc_id, dev_name, local_endpt, remote_endpt, eptdev_name, flags);
    if (!rcdev)
    {
        perror("Unable to create rpmsg endpoint device");
        // return -EPERM;
    }

    for (int i = 0 ; !rcdev ; i++) {
        sleep(2);
        printf("ATTEMPT #%d\n", i);
        rcdev = rpmsg_char_open(rproc_id, dev_name, local_endpt, remote_endpt, eptdev_name, flags);
        perror("Unable to create rpmsg endpoint device");
    }

    printf("Created endpoint device %s, fd = %d, port = %d\n", eptdev_name,
           rcdev->fd, rcdev->endpt);


    // Example: Call R5 function
    int result = call_function_a_blocking(rcdev, 5, 3);
    printf("Linux: Result from FUNCTION_A: %d\n", result);

    // Optional: Main loop for continuous processing
    while (1) {
        MESSAGE msg;
        // If there is no message, I get hung on this read. I would like linux to continue on if there is no message
        int ret = read(rcdev->fd, &msg, sizeof(MESSAGE));
        if (ret == sizeof(MESSAGE)) {
            if (msg.tag == MESSAGE_REQUEST) {
                handle_request_linux(&msg, rcdev);
            } else if (msg.tag == MESSAGE_RESPONSE) {
                printf("Linux: Received response for request_id %u\n", msg.request_id);
            }
        } else {
            printf("read returned %d\n", ret);
        }

        burn_time_pretending_to_do_stuff(800, 1200);

        result = call_function_a_blocking(rcdev, 5, 3);
        printf("Linux: Result from FUNCTION_A: %d\n", result);
        printf("\n");
        burn_time_pretending_to_do_stuff(800, 1200);
    }
    

    // this cleanup function will sometimes get called twice 
    return rpmsg_cleanup(rcdev);
}

/*
 * Main function for the Linux side RPC example.
 */
int rpmsg_char_simple_main(void)
{
    int ret, status;
    int rproc_id = R5F_MAIN0_0; // 2, THIS IS HOW LINUX KNOWS WHICH R5 TO TALK TO!!
    unsigned int local_endpt = RPMSG_ADDR_ANY;
    unsigned int remote_endpt = RPMSG_CHAR_ENDPOINT; // 14
    char *dev_name = RPMSG_CHAR_DEVICE_NAME; // "rpmsg_chrdev"

    ret = rpmsg_char_init(NULL);
    if (ret)
    {
        printf("rpmsg_char_init failed, ret = %d\n", ret);
        return ret;
    }
    
    status = rpmsg_char_rpc(rproc_id, dev_name, local_endpt, remote_endpt);
    (void)status;
    rpmsg_char_exit();

    return 0;
}
