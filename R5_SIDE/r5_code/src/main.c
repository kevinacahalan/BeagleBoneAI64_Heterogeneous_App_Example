#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <ti/csl/csl_gpio.h>
#include <ti/csl/csl_mcspi.h>
#include <ti/csl/csl_epwm.h>
#include <ti/csl/soc.h>
#include <ti/osal/osal.h>

#include "../../../SHARED_CODE/include/random_utils.h"
// #include "../../../SHARED_CODE/src/random_utils.c"

#include <ai64/bbai64_rpmsg.h>
#include <ai64/bbai64_clocks.h>


#include <io_test_functions/gpio_tests.h>
#include <io_test_functions/epwm_tests.h>
#include <io_test_functions/spi_tests.h>
#include <io_test_functions/eqep_tests.h>
#include <io_test_functions/uart_tests.h>

volatile int wait_for_debugger = 0;

void do_other_random_things() {
    printf("\n> Inside do_other_random_things()\n");
    run_pwm_test(5);
    test_uart(BBAI64_UART6_BASE);
}


// MAKE SURE TO ADD YOUR FUNCTIONS TO request_tagged_union_t in SHARED_CODE/include/shared_rpmsg.h

// Example R5 functions callable from Linux
int function_a(int a, int b) {
    int rt = a + b;
    // printf("R5 function_a: Got %d,%d from Linux, returning %d\n", a, b, rt);
    return rt;
}

float function_b(float a, float b, float c) {
    float rt = a + b + c;
    // printf("R5 function_b: Got %f,%f,%f from Linux, returning %f\n", a, b, c, rt);
    return rt;
}

// Handle incoming request from Linux
int handle_request(MESSAGE *req_msg, RPMessage_Handle handle, uint32_t myEndPt, uint32_t remoteEndPt, uint32_t remoteProcId) {
    request_data_t *req = &req_msg->data.request;
    MESSAGE resp = {0};
    resp.tag = MESSAGE_RESPONSE;
    resp.request_id = req_msg->request_id;
    resp.data.response.function_tag = req->function_tag;

    switch (req->function_tag) {
        case FUNCTION_A:
            resp.data.response.result.result_function_a = function_a(req->params.function_a.a, req->params.function_a.b);
            printf("%s: Got %d and %d from linux call, sending back %d\n", function_tag_to_string(req->function_tag), req->params.function_a.a, req->params.function_a.b, resp.data.response.result.result_function_a);
            break;
        case FUNCTION_B:
            resp.data.response.result.result_function_b = function_b(req->params.function_b.a, req->params.function_b.b, req->params.function_b.c);
            printf("%s: Got %f, %f, and %f from linux call, sending back %f\n", function_tag_to_string(req->function_tag), req->params.function_b.a, req->params.function_b.b, req->params.function_b.c, resp.data.response.result.result_function_b);
            break;
        default:
            printf("R5: Unknown request tag %s\n", function_tag_to_string(req->function_tag));
            return -1;
    }

    burn_time_pretending_to_do_stuff(800, 1200);
    int32_t status = RPMessage_send(handle, remoteProcId, remoteEndPt, myEndPt, &resp, sizeof(MESSAGE));

    if (status != IPC_SOK) {
        printf("R5: Send failed (%ld); retrying once...\n", status);
        Osal_delay(500);
        status = RPMessage_send(handle, remoteProcId, remoteEndPt, myEndPt, &resp, sizeof(MESSAGE));  // Simple retry
        if (status != IPC_SOK) 
            return -1; // eh, FIXME?
    }
    
    if (status != IPC_SOK) {
        printf("R5: Send response failed (%ld)\n", status);
    }

    return 0;
}

// Handle incoming command (non-blocking, no response)
void handle_command(MESSAGE *cmd_msg) {
    // Example: Reuse function logic without response
    switch (cmd_msg->data.request.function_tag) {
        case FUNCTION_A:
            function_a(cmd_msg->data.request.params.function_a.a, cmd_msg->data.request.params.function_a.b);
            break;
        // Add others...
        default:
            printf("R5: Unknown command tag %s\n", function_tag_to_string(cmd_msg->data.request.function_tag));
    }
    printf("R5: Handled non-blocking command %s\n", function_tag_to_string(cmd_msg->data.request.function_tag));
}

// Blocking call to Linux function X
float call_linux_function_x_blocking(RPMessage_Handle handle, uint32_t myEndPt, uint32_t remoteEndPt, uint32_t remoteProcId, int param) {
    static uint32_t req_id_counter = 1;
    uint32_t req_id = req_id_counter++;

    MESSAGE req = {0};
    req.tag = MESSAGE_REQUEST;
    req.request_id = req_id;
    req.data.request.function_tag = FUNCTION_X;
    req.data.request.params.function_x.param = param;

    int32_t status = RPMessage_send(handle, remoteProcId, remoteEndPt, myEndPt, &req, sizeof(MESSAGE));

    if (status != IPC_SOK) {
        printf("R5: Send failed (%ld); retrying once...\n", status);
        Osal_delay(500);
        status = RPMessage_send(handle, remoteProcId, remoteEndPt, myEndPt, &req, sizeof(MESSAGE));  // Simple retry
        if (status != IPC_SOK) return -1.0f;
    }

    if (status != IPC_SOK) {
        printf("R5: Send request failed (%ld)\n", status);
        return -1.0f;
    }

    uint64_t start = get_gtc_as_u64seconds();  // Use OSAL timer or similar
    while (get_gtc_as_u64seconds() - start < 10) {
    MESSAGE msg;
        uint16_t len = sizeof(MESSAGE);
        uint32_t srcEndPt = remoteEndPt;
        uint32_t srcProc = remoteProcId;
        status = RPMessage_recvNb(handle, &msg, &len, &srcEndPt, &srcProc);
        if (status == IPC_SOK && len == sizeof(MESSAGE)) {
            printf("R5: Polled msg tag=%s, id=%lu during wait\n", message_tag_to_string(msg.tag), msg.request_id);  // Debug
            if (msg.tag == MESSAGE_RESPONSE && msg.request_id == req_id) {
                printf("R5: RESPONSE RECEIVED!! from blocking %s\n", function_tag_to_string(msg.data.response.function_tag));
            } else if (msg.tag == MESSAGE_REQUEST) {
                handle_request(&msg, handle, myEndPt, srcEndPt, srcProc);
            } else if (msg.tag == MESSAGE_COMMAND) {
                handle_command(&msg);
            } else {
                // Queue or ignore others
            }
            remoteEndPt = srcEndPt;  // Always update
            remoteProcId = srcProc;
        } else if (status != IPC_ETIMEOUT) {
            printf("R5: Poll error %ld\n", status);
        }
        Osal_delay(1);  // Tighter poll
    }
    printf("R5: RESPONSE FAILURE!!, Timeout waiting for FUNCTION_X response\n");
    return -1.0f;
}

// Non-blocking command to Linux (e.g., trigger FUNCTION_X without return)
int send_command_x_nonblocking(RPMessage_Handle handle, uint32_t myEndPt, uint32_t remoteEndPt, uint32_t remoteProcId, int param) {
    MESSAGE cmd = {0};
    cmd.tag = MESSAGE_COMMAND;
    cmd.data.request.function_tag = FUNCTION_X;
    cmd.data.request.params.function_x.param = param;

    int32_t status = RPMessage_send(handle, remoteProcId, remoteEndPt, myEndPt, &cmd, sizeof(MESSAGE));
    if (status != IPC_SOK) {
        printf("R5: Send failed (%ld); retrying once...\n", status);
        Osal_delay(500);
        status = RPMessage_send(handle, remoteProcId, remoteEndPt, myEndPt, &cmd, sizeof(MESSAGE));  // Simple retry
        if (status != IPC_SOK) 
        return -1; // eh fixme
    }
    
    printf("R5: Sent non-blocking command for FUNCTION_X\n");
    return 0; 
}

// Process one incoming message (polling helper)
int process_one_message(RPMessage_Handle handle, uint32_t myEndPt, uint32_t *remoteEndPt, uint32_t *remoteProcId) {
    MESSAGE msg;
    uint16_t len = sizeof(MESSAGE);
    int32_t status = RPMessage_recvNb(handle, &msg, &len, remoteEndPt, remoteProcId);
    if (status != IPC_SOK) return status;
    if (len != sizeof(MESSAGE)) {
        printf("R5: Bad message size %d\n", len);
        return -1;
    }

    if (msg.tag == MESSAGE_REQUEST) {
        handle_request(&msg, handle, myEndPt, *remoteEndPt, *remoteProcId);
    } else if (msg.tag == MESSAGE_COMMAND) {
        handle_command(&msg);
    } else if (msg.tag == MESSAGE_RESPONSE) {
        printf("R5: Received unexpected response (request_id %lu)\n", msg.request_id);
    } else {
        printf("R5: Unknown tag %d\n", msg.tag);
    }
    return IPC_SOK;
}

int32_t start_listing_to_linux(void) {
    uint32_t myEndPt = 0;
    uint32_t remoteEndPt = 0;
    uint32_t remoteProcId = IPC_MPU1_0;
    RPMessage_Handle handle;
    setup_ipc(&handle, &myEndPt);

    // Wait for first message to resolve endpoints
    printf("R5: Waiting for handshake...\n");
    uint64_t timeout_sec = 10;
    uint64_t start = get_gtc_as_u64seconds();
    while (get_gtc_as_u64seconds() - start < timeout_sec) {
        MESSAGE msg;
        uint16_t len = sizeof(MESSAGE);
        int32_t status = RPMessage_recvNb(handle, &msg, &len, &remoteEndPt, &remoteProcId);
        if (status == IPC_SOK && len == sizeof(MESSAGE)) {
            if (msg.tag == MESSAGE_PING) {
                printf("R5: Received ping, endpoints resolved (remoteEndPt=%lu)\n", remoteEndPt);
                break;
            } else {
                // Handle unexpected early message
                process_one_message(handle, myEndPt, &remoteEndPt, &remoteProcId);
            }
        }
        Osal_delay(100);  // Poll every 100ms
    }
    if (remoteEndPt == 0) {
        printf("R5: Handshake timeout; using default endpoint? Proceed with caution.\n");
        remoteEndPt = 1024;  // Common default for Linux->R5; check your dmesg
    }


    Osal_delay(2000);
    printf("R5: Listening for messages\n");

    // Main loop: Poll for messages, make example calls
    while (1) {
        process_one_message(handle, myEndPt, &remoteEndPt, &remoteProcId);

        // Example blocking call
        call_linux_function_x_blocking(handle, myEndPt, remoteEndPt, remoteProcId, 16);

        // Example non-blocking command
        send_command_x_nonblocking(handle, myEndPt, remoteEndPt, remoteProcId, 32);

        burn_time_pretending_to_do_stuff(800, 1200);
    }
    return 0;
}

int main()
{   
    printf("R5_SIDE started from main\n");

    // PIN_MOSI=P8_09, PIN_SCLK=P8_08, PIN_CS=P8_06
    test_bitbang_spi();

    test_spi_mcspi7(0);
    test_eqep1_with_gpio_encoder_simulation();
    test_uart(BBAI64_UART6_BASE);
    run_pwm_test(5);
    burn_time_pretending_to_do_stuff(800, 1200);


    printf("\nAbout to start listing to linux\n");
    start_listing_to_linux();
    

    while(wait_for_debugger == 1)
        ;


    printf("R5 hit end of main\n");
    return 0;
}
