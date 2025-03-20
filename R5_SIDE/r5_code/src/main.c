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

volatile int wait_for_debugger = 0;


/*
 * This is the main test function which initializes and runs the Sender and
 * responder functions.
 * NOTE: Sender function is not actually active in this example.
 *       Only the receiver function does a echo back of incoming messages
 */
/* ==========================================*/
int32_t start_listing_to_linux(void)
{
    uint32_t myEndPt = 0, remoteEndPt = 0, remoteProcId = 0;
    RPMessage_Handle handle_chrdev;
    setup_ipc(&handle_chrdev, &myEndPt);
    /* run responder function to receive and reply messages back */

    printf("Inside start_listing_to_linux() waiting for messages\n");
    for (size_t i = 0;; i++)
    {   
        Osal_delay(1000); // 1 second 
        // printf("R5 loop print %d\n", i);
        int rt = 0;
        rt = process_one_rproc_message(&handle_chrdev, &myEndPt, &remoteEndPt, &remoteProcId);
        switch (rt)
        {
        case IPC_ETIMEOUT: // no message
            // printf("Tried to pull a message but there were none\n");
            break;
        
        default:
            printf("rt = %d...", rt);
            break;
        }
    }
        
    return 0;
}

#include <io_test_functions/epwm_tests.h>
#include <io_test_functions/spi_tests.h>
int main()
{   
    printf("R5_SIDE started from main\n");

    run_pwm_test(20);

    int count = 2;
    for (int i = 1; i <= count; i++)
    {
        Osal_delay(1000);
        printf("print from R5 %d/%d\n", i, count);
    }

    // output some 
    test_spi_mcspi7(0);
    printf("Done SPI7 test\n");
    Osal_delay(1000);

    printf("About to do IPC test\n");
    start_listing_to_linux();
    

    while(wait_for_debugger == 1)
        ;


    printf("R5 hit end of main\n");
    return 0;
}
