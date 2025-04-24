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

volatile int wait_for_debugger = 0;

void do_other_random_things() {
    printf("\n> Inside do_other_random_things()\n");
    run_pwm_test(5);
}


// MAKE SURE TO ADD YOUR FUNCTIONS TO request_tagged_union_t in SHARED_CODE/include/shared_rpmsg.h

int function_a(int a, int b) {
    int rt = a + b;
    printf("function_a: Got %d and %d from linux call, sending back %d\n", a, b, rt);
    return rt;
}

float function_b(float a, float b, float c) {
    float rt = a + b + c;
    printf("function_b: Got %f, %f, and %f from linux call, sending back %f\n", a, b, c, rt);
    return rt;
}

void handle_request(MESSAGE *req_msg, RPMessage_Handle handle, uint32_t myEndPt, uint32_t remoteEndPt, uint32_t remoteProcId) {
    request_data_t *req = &req_msg->data.request;
    MESSAGE response_msg = {0};
    response_msg.tag = MESSAGE_RESPONSE;
    response_msg.request_id = req_msg->request_id;
    response_msg.data.response.function_tag = req->function_tag;

    switch (req->function_tag) {
        case FUNCTION_A:
            response_msg.data.response.result.result_function_a = 
                function_a(req->params.function_a.a, req->params.function_a.b);
            break;
        case FUNCTION_B:
            response_msg.data.response.result.result_function_b = 
                function_b(req->params.function_b.a, req->params.function_b.b, req->params.function_b.c);
            break;
        default:
            printf("R5: Unknown function tag %d\n", req->function_tag);
            return;  // No response sent for unknown functions
    }

    burn_time_pretending_to_do_stuff(800, 1200);

    int32_t status = RPMessage_send(handle, remoteProcId, remoteEndPt, myEndPt, &response_msg, sizeof(MESSAGE));
    if (status != IPC_SOK) {
        printf("R5: Failed to send response, status = %ld\n", status);
    }
}

float call_linux_function_x_blocking(RPMessage_Handle handle, uint32_t myEndPt, uint32_t remoteEndPt, uint32_t remoteProcId, int param) {
    // printf("Start of call_linux_function_x_blocking\n");
    static uint32_t request_id_counter = 1;
    uint32_t request_id = request_id_counter++;

    // Send request
    MESSAGE req_msg = {0};
    req_msg.tag = MESSAGE_REQUEST;
    req_msg.request_id = request_id;
    req_msg.data.request.function_tag = FUNCTION_X;
    req_msg.data.request.params.function_x.param = param;

    int32_t status = RPMessage_send(handle, remoteProcId, remoteEndPt, myEndPt, &req_msg, sizeof(MESSAGE));
    if (status != IPC_SOK) {
        printf("R5: Failed to send request, status = %ld\n", status);
        return 0.0f;
    }

    burn_time_pretending_to_do_stuff(800, 1200);

    // Wait for response, processing other requests to avoid deadlock
    while (1) {
        MESSAGE resp_msg;
        uint16_t rxLen = sizeof(MESSAGE);
        status = RPMessage_recvNb(handle, (Ptr)&resp_msg, &rxLen, &remoteEndPt, &remoteProcId);
        if (status == IPC_SOK && rxLen == sizeof(MESSAGE)) {
            if (resp_msg.tag == MESSAGE_RESPONSE && resp_msg.request_id == request_id) {
                if (resp_msg.data.response.function_tag == FUNCTION_X) {
                    printf("GOT RETURN VALUE %f from linux function with tag %d\n", resp_msg.data.response.result.result_function_x, resp_msg.data.response.function_tag);
                    return resp_msg.data.response.result.result_function_x;
                } else {
                    printf("R5: Mismatched function tag %d in response\n", resp_msg.data.response.function_tag);
                }
            } else if (resp_msg.tag == MESSAGE_REQUEST) {
                printf("Hit request while waiting for response To R5 (possible out-of-order responses, potential for lockup)\n");
                handle_request(&resp_msg, handle, myEndPt, remoteEndPt, remoteProcId);
            }
        }

        // if (status != IPC_SOK) {
        //     printf("status != IPC_SOK\n");
        //     return status;  // No message or error
        // }

        if (status == IPC_ETIMEOUT)
        {
            // The is the case for pulling when there is no message iirc
            // printf("Tried to pull message, there was none\n");
        } else if (status == IPC_SOK) {
            printf("handle_request() was likely called from call_linux_function_x_blocking\n");
        } else {
            printf("ERROR: RPMessage_recvNb status = %ld\n", status);
        }
        // break;
        // Optional: Add delay or timeout to prevent infinite loop
    }

    // printf("End of call_linux_function_x_blocking..\n");
    return 0.0f;  // Unreachable with infinite loop; add timeout in production
}

int process_one_message(RPMessage_Handle handle, uint32_t myEndPt, uint32_t *remoteEndPt, uint32_t *remoteProcId) {
    MESSAGE msg;
    uint16_t rxLen = sizeof(MESSAGE);
    int32_t status = RPMessage_recvNb(handle, (Ptr)&msg, &rxLen, remoteEndPt, remoteProcId);
    if (status != IPC_SOK) {
        return status;  // No message or error
    }
    if (rxLen != sizeof(MESSAGE)) {
        printf("R5: Received message of unexpected size %d, expected %d\n", rxLen, sizeof(MESSAGE));
        return -1;
    }

    if (msg.tag == MESSAGE_REQUEST) {
        handle_request(&msg, handle, myEndPt, *remoteEndPt, *remoteProcId);
    } else if (msg.tag == MESSAGE_RESPONSE) {
        printf("R5: Received response for request_id %lu, function_tag %d\n", 
               msg.request_id, msg.data.response.function_tag);
               
    } else {
        printf("R5: Unknown message tag %d\n", msg.tag);
    }
    return 0;
}



int32_t start_listing_to_linux(void)
{
    uint32_t myEndPt = 0; // becomes 14
    uint32_t remoteEndPt = 0; // Seems to always become 1025
    uint32_t remoteProcId = IPC_MPU1_0; // Stays as 0, or IPC_MPU1_0, which is "ARM A72 - VM0"
    RPMessage_Handle handle_chrdev;
    setup_ipc(&handle_chrdev, &myEndPt);

    Osal_delay(2000);
    // call_linux_function_x_blocking(handle_chrdev, myEndPt, remoteEndPt, remoteProcId, 16);

    printf("Inside start_listing_to_linux() waiting for messages\n");
    for (size_t i = 0;; i++)
    {   
        // printf("R5 loop print %d\n", i);
        int rt = 0;
        rt = process_one_message(handle_chrdev, myEndPt, &remoteEndPt, &remoteProcId);
        // printf("myEndPt= %ld, remoteEndPt=%ld, remoteProcId=%ld\n", myEndPt, remoteEndPt, remoteProcId);
        switch (rt)
        {
        case IPC_ETIMEOUT: // no message
            // printf("Tried to pull a message but there were none\n");
            break;
        case IPC_SOK: // eh
            break;
        default:
            printf("Possible problem, process_one_message() rt = %d...\n", rt);
            break;
        }

        // do_other_random_things();
        call_linux_function_x_blocking(handle_chrdev, myEndPt, remoteEndPt, remoteProcId, 16);
        printf("\n");

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
    run_pwm_test(5);
    burn_time_pretending_to_do_stuff(800, 1200);


    printf("\nAbout to start listing to linux\n");
    start_listing_to_linux();
    

    while(wait_for_debugger == 1)
        ;


    printf("R5 hit end of main\n");
    return 0;
}
