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
#include <example_rpmsg_talk.h>


#include <io_test_functions/gpio_tests.h>
#include <io_test_functions/epwm_tests.h>
#include <io_test_functions/spi_tests.h>
#include <io_test_functions/eqep_tests.h>
#include <io_test_functions/uart_tests.h>

volatile int wait_for_debugger = 0;

static int run_demo_cycle(void)
{
    float function_x_result = 0.0f;
    int function_y_result = 0;
    REMOTE_RETURN tmp;

    tmp = call_linux_function_x_blocking(16, &function_x_result);
    if (tmp.error != 0) {
        printf("R5: call_linux_function_x_blocking failed\n");
        return -1;
    }
    printf("R5: call_linux_function_x_blocking succeeded, rt=%.3f\n", *(float *)tmp.rt);

    burn_time_pretending_to_do_stuff(5, 200);
    if ((get_random_u32() % 5U) == 0U) {
        return 0;
    }

    tmp = call_linux_function_y_blocking(32.0f, &function_y_result);
    if (tmp.error != 0) {
        printf("R5: call_linux_function_y_blocking failed\n");
        return -1;
    }
    printf("R5: call_linux_function_y_blocking succeeded, rt=%d\n", *(int *)tmp.rt);

    burn_time_pretending_to_do_stuff(5, 200);
    if ((get_random_u32() % 5U) == 0U) {
        return 0;
    }

    tmp = send_command_x_nonblocking(32);
    if (tmp.error != 0) {
        printf("R5: send_command_x_nonblocking failed\n");
        return -1;
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
    test_uart(UART_TEST_TX_BASE, UART_TEST_RX_BASE);
    run_pwm_test(5);
    burn_time_pretending_to_do_stuff(800, 1200);


    printf("\nAbout to start listening to Linux\n");
    if (setup_r5_example_talk() != 0) {
        printf("R5: setup_r5_example_talk failed\n");
        return 1;
    }

    while (1) {
        process_linux_messages(8);

        if (!example_rpmsg_peer_ready()) {
            Osal_delay(100);
            continue;
        }

        if (run_demo_cycle() != 0) {
            Osal_delay(100);
            continue;
        }

        burn_time_pretending_to_do_stuff(800, 1200);
    }
    

    while(wait_for_debugger == 1)
        ;


    printf("R5 hit end of main\n");
    return 0;
}
