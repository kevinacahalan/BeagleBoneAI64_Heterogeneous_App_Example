/*
 * rpmsg_char_simple.c
 *
 * Simple Example application using rpmsg-char library for RPC.
 * The Linux side sends an addition request (2 ints) to the R5,
 * which adds them and returns the result.
 *
 *  (License text omitted for brevity)
 */

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

#define REMOTE_ENDPT 14

/* Structure for the addition request (must match baremetal) */
// typedef struct {
//     int a;
//     int b;
// } addition_request_t;

typedef enum
{
    UNKNOWN = 0,
    FUNCTION_A = 1,
    FUNCTION_B = 2,
} FUNCTION_TAG;

typedef struct {
    FUNCTION_TAG tag;
    union {
        struct {
            int a;
            int b;
        } function_a;  // Named member for FUNCTION_A arguments
        struct {
            float a;
            float b;
            float c;
        } function_b;  // Named member for FUNCTION_B arguments
    };
} request_tagged_union_t;

static int rpmsg_cleanup(rpmsg_char_dev_t *rcdev);

/*
 * Helper function to send a message.
 */
static int send_msg(int fd, const void *msg, int len)
{
    int ret = write(fd, msg, len);
    if (ret < 0)
    {
        perror("Error writing to rpmsg endpoint");
        return -1;
    }
    return ret;
}

/*
 * Helper function to receive a message.
 */
static int recv_msg(int fd, int max_len, void *reply_msg, int *reply_len)
{
    int ret = read(fd, reply_msg, max_len);
    if (ret < 0)
    {
        perror("Error reading from rpmsg endpoint");
        return -1;
    }
    *reply_len = ret;
    return 0;
}

static int function_a(rpmsg_char_dev_t *rcdev, int a, int b) {
    request_tagged_union_t req;
	int result = 0;
    int ret = 0;
	req.tag = FUNCTION_A;
    req.function_a.a = a;  /* First integer */
    req.function_a.b = b;  /* Second integer */
    printf("Sending addition request: %d + %d\n", req.function_a.a, req.function_a.b);

    ret = send_msg(rcdev->fd, (const void *)&req, sizeof(req));
    if (ret != sizeof(req))
    {
        printf("Error: written bytes (%d) do not match expected (%zu)\n", ret, sizeof(req));
        rpmsg_cleanup(rcdev);
    }

    /* Wait for the result (an integer) */
    int reply_len = 0;
    ret = recv_msg(rcdev->fd, sizeof(result), (void *)&result, &reply_len);
    if (ret < 0)
    {
        printf("Error receiving the response\n");
        rpmsg_cleanup(rcdev);
    }
    if (reply_len != sizeof(result))
    {
        printf("Unexpected reply size (%d bytes)\n", reply_len);
        rpmsg_cleanup(rcdev);
    }

	printf("Received result from baremetal: %d\n", result);

    return result;
}

static float function_b(rpmsg_char_dev_t *rcdev, float a, float b, float c) {
    request_tagged_union_t req;
    float f_result = 0.0;
    int ret = 0;
	req.tag = FUNCTION_B;
	req.function_b.a = a;  /* First integer */
    req.function_b.b = b;  /* Second integer */
	req.function_b.c = c;
    printf("Sending Float request: %f + %f + %f\n", req.function_b.a, req.function_b.b, req.function_b.c);

    ret = send_msg(rcdev->fd, (const void *)&req, sizeof(req));
    if (ret != sizeof(req))
    {
        printf("Error: written bytes (%d) do not match expected (%zu)\n", ret, sizeof(req));
        rpmsg_cleanup(rcdev);
    }

    /* Wait for the result (a float) */
    int reply_len = 0;
    ret = recv_msg(rcdev->fd, sizeof(f_result), (void *)&f_result, &reply_len);
    if (ret < 0)
    {
        printf("Error receiving the response\n");
        rpmsg_cleanup(rcdev);
    }
    if (reply_len != sizeof(f_result))
    {
        printf("Unexpected reply size (%d bytes)\n", reply_len);
        rpmsg_cleanup(rcdev);
    }

    printf("Received f_result from baremetal: %f\n", f_result);
	return a + b + c;
}

static int rpmsg_cleanup(rpmsg_char_dev_t *rcdev) {
    int ret = rpmsg_char_close(rcdev);
    if (ret < 0)
    {
        perror("Error closing the rpmsg endpoint device");
    }
    return ret;
}

/*
 * Function to perform the RPC:
 * Send an addition request (two ints) and wait for the integer result.
 */
static int rpmsg_char_rpc(int rproc_id, char *dev_name,
                          unsigned int local_endpt, unsigned int remote_endpt)
{
    char eptdev_name[64] = {0};
    rpmsg_char_dev_t *rcdev = NULL;
    int flags = 0;

    /* Create a unique endpoint name */
    snprintf(eptdev_name, sizeof(eptdev_name), "rpmsg-char-%d-%d", rproc_id, getpid());

    /* Open the rpmsg-char device */
    rcdev = rpmsg_char_open(rproc_id, dev_name, local_endpt, remote_endpt, eptdev_name, flags);
    if (!rcdev)
    {
        perror("Unable to create rpmsg endpoint device");
        // return -EPERM;
    }

    for (int i = 0 ; !rcdev ; i++) {
        sleep(5);
        printf("ATTEMPT #%d\n", i);
        rcdev = rpmsg_char_open(rproc_id, dev_name, local_endpt, remote_endpt, eptdev_name, flags);
        perror("Unable to create rpmsg endpoint device");
    }

    printf("Created endpoint device %s, fd = %d, port = %d\n", eptdev_name,
           rcdev->fd, rcdev->endpt);

    // ADD INT
    function_a(rcdev, 4,7);
    function_a(rcdev, 3,7);
    function_a(rcdev, 2,7);
    function_a(rcdev, 1,7);
    function_a(rcdev, 0,7);

	// ADD FLOAT
    function_b(rcdev, 4.0, 7.0, 3.0);
    

    // this cleanup function will sometimes get called twice 
    return rpmsg_cleanup(rcdev);
}

/*
 * Main function for the Linux side RPC example.
 */
int rpmsg_char_simple_main(void)
{
    int ret, status;
    int rproc_id = 2;
    unsigned int local_endpt = RPMSG_ADDR_ANY;
    unsigned int remote_endpt = REMOTE_ENDPT;
    char *dev_name = NULL;  /* Auto-detect if supported */

    ret = rpmsg_char_init(NULL);
    if (ret)
    {
        printf("rpmsg_char_init failed, ret = %d\n", ret);
        return ret;
    }
    
    status = rpmsg_char_rpc(rproc_id, dev_name, local_endpt, remote_endpt);
    rpmsg_char_exit();

    if (status < 0)
    {
        printf("TEST STATUS: FAILED\n");
    }
    else
    {
        printf("TEST STATUS: PASSED\n");
    }

    return 0;
}
