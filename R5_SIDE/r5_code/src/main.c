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

#include <ai64/bbai64_rpmsg.h>


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

int32_t start_listing_to_linux(void)
{
    uint32_t myEndPt = 0; // becomes 14
    uint32_t remoteEndPt = 0; // Seems to always become 1025
    uint32_t remoteProcId = IPC_MPU1_0; // Stays as 0, or IPC_MPU1_0, which is "ARM A72 - VM0"
    RPMessage_Handle handle_chrdev;
    setup_ipc(&handle_chrdev, &myEndPt);

    printf("Inside start_listing_to_linux() waiting for messages\n");
    for (size_t i = 0;; i++)
    {   
        Osal_delay(1000); // 1 second 
        // printf("R5 loop print %d\n", i);
        int rt = 0;
        rt = process_one_rproc_message(&handle_chrdev, &myEndPt, &remoteEndPt, &remoteProcId);
        // printf("myEndPt= %ld, remoteEndPt=%ld, remoteProcId=%ld\n", myEndPt, remoteEndPt, remoteProcId);
        switch (rt)
        {
        case IPC_ETIMEOUT: // no message
            // printf("Tried to pull a message but there were none\n");
            break;
        
        default:
            printf("rt = %d...", rt);
            break;
        }

        do_other_random_things();
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


    printf("\nAbout to start listing to linux\n");
    start_listing_to_linux();
    

    while(wait_for_debugger == 1)
        ;


    printf("R5 hit end of main\n");
    return 0;
}
