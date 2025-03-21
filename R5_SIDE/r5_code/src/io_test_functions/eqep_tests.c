#include <stdio.h>
#include <ai64/sys_eqep.h>
#include <ai64/bbai64_eqep.h>
#include <ai64/bbai64_gpio.h>

#include <ti/csl/csl_gpio.h>
#include <ti/csl/soc.h>

#include <ti/osal/osal.h>

// Function to initialize EQEP1 using only the A and B inputs
void eqep1_init() {
    // Set the maximum position value
    EQEP1.QPOSMAX = 0xFFFFFFFF; // Set maximum count value

    // Clear any previous position
    EQEP1.QPOSCNT = 0;

    // Configure the decoder to use quadrature mode with A and B inputs only
    EQEP1.QDEC_QEP_CTL = 0x0000; // Reset control register
    EQEP1.QDEC_QEP_CTL |= (1 << 19); // Enable the quadrature position counter
    EQEP1.QDEC_QEP_CTL |= (0b00 << 14); // Quadrature count mode, no edges count
    EQEP1.QDEC_QEP_CTL |= (0b00 << 28); // Position-counter reset on maximum position

    // Enable unit timer to run continuously, automatic latching?
    EQEP1.QDEC_QEP_CTL |= (1 << 17); // Enable unit timer
    EQEP1.QUPRD = 1000000; // Set unit period to 1,000,000 for unit timer
}

// read current position from EQEP1
unsigned int eqep1_get_position() {
    return EQEP1.QPOSCNT;
}

void test_eqep1_mmr_loop_forever() {
    eqep1_init(); // Initialize EQEP1

    while (1) {
        unsigned int position = eqep1_get_position();
        printf("Current Position: %u\n", position);
        for (volatile int i = 0; i < 1000000; ++i); // delay loop
    }
}

// MAKE SURE to connect P8_33(eqepB) to P8_34, and P8_35(eqepA) to P8_36
#define PIN_GPIO_TO_EQEPA P8_36
#define PIN_GPIO_TO_EQEPB P8_34
void test_eqep1_with_gpio_encoder_simulation() {
    GPIOSetDirMode_v0(GPIO_PIN_BASE_ADDR(PIN_GPIO_TO_EQEPA), GPIO_PIN_NUM(PIN_GPIO_TO_EQEPA), GPIO_DIRECTION_OUTPUT);
    GPIOSetDirMode_v0(GPIO_PIN_BASE_ADDR(PIN_GPIO_TO_EQEPB), GPIO_PIN_NUM(PIN_GPIO_TO_EQEPB), GPIO_DIRECTION_OUTPUT);
    GPIOPinWrite_v0(GPIO_PIN_BASE_ADDR(PIN_GPIO_TO_EQEPA), GPIO_PIN_NUM(PIN_GPIO_TO_EQEPA), GPIO_PIN_LOW);
    GPIOPinWrite_v0(GPIO_PIN_BASE_ADDR(PIN_GPIO_TO_EQEPB), GPIO_PIN_NUM(PIN_GPIO_TO_EQEPB), GPIO_PIN_LOW);

    eqep_init(EQEP1_P8_35_P8_33);

    int position = eqep_get_position(EQEP1_P8_35_P8_33);
    printf("EQEP counter position before simulation: %d\n", position); // prints 0

    Osal_delay(100); // 1/10 second 
    GPIOPinWrite_v0(GPIO_PIN_BASE_ADDR(PIN_GPIO_TO_EQEPA), GPIO_PIN_NUM(PIN_GPIO_TO_EQEPA), GPIO_PIN_HIGH);
    Osal_delay(100); // 1/10 second 
    GPIOPinWrite_v0(GPIO_PIN_BASE_ADDR(PIN_GPIO_TO_EQEPB), GPIO_PIN_NUM(PIN_GPIO_TO_EQEPB), GPIO_PIN_HIGH);
    Osal_delay(100); // 1/10 second 
    GPIOPinWrite_v0(GPIO_PIN_BASE_ADDR(PIN_GPIO_TO_EQEPA), GPIO_PIN_NUM(PIN_GPIO_TO_EQEPA), GPIO_PIN_LOW);
    Osal_delay(100); // 1/10 second 
    GPIOPinWrite_v0(GPIO_PIN_BASE_ADDR(PIN_GPIO_TO_EQEPB), GPIO_PIN_NUM(PIN_GPIO_TO_EQEPB), GPIO_PIN_LOW);

    // The encoder count at this point is 3

    Osal_delay(100); // 1/10 second 
    GPIOPinWrite_v0(GPIO_PIN_BASE_ADDR(PIN_GPIO_TO_EQEPA), GPIO_PIN_NUM(PIN_GPIO_TO_EQEPA), GPIO_PIN_HIGH);
    Osal_delay(100); // 1/10 second 
    GPIOPinWrite_v0(GPIO_PIN_BASE_ADDR(PIN_GPIO_TO_EQEPB), GPIO_PIN_NUM(PIN_GPIO_TO_EQEPB), GPIO_PIN_HIGH);
    Osal_delay(100); // 1/10 second 
    GPIOPinWrite_v0(GPIO_PIN_BASE_ADDR(PIN_GPIO_TO_EQEPA), GPIO_PIN_NUM(PIN_GPIO_TO_EQEPA), GPIO_PIN_LOW);
    Osal_delay(100); // 1/10 second 
    GPIOPinWrite_v0(GPIO_PIN_BASE_ADDR(PIN_GPIO_TO_EQEPB), GPIO_PIN_NUM(PIN_GPIO_TO_EQEPB), GPIO_PIN_LOW);

    // The encoder count at this point is 7

    Osal_delay(100); // 1/10 second 
    GPIOPinWrite_v0(GPIO_PIN_BASE_ADDR(PIN_GPIO_TO_EQEPA), GPIO_PIN_NUM(PIN_GPIO_TO_EQEPA), GPIO_PIN_HIGH);
    Osal_delay(100); // 1/10 second 
    GPIOPinWrite_v0(GPIO_PIN_BASE_ADDR(PIN_GPIO_TO_EQEPB), GPIO_PIN_NUM(PIN_GPIO_TO_EQEPB), GPIO_PIN_HIGH);
    Osal_delay(100); // 1/10 second 
    GPIOPinWrite_v0(GPIO_PIN_BASE_ADDR(PIN_GPIO_TO_EQEPA), GPIO_PIN_NUM(PIN_GPIO_TO_EQEPA), GPIO_PIN_LOW);
    Osal_delay(100); // 1/10 second 
    GPIOPinWrite_v0(GPIO_PIN_BASE_ADDR(PIN_GPIO_TO_EQEPB), GPIO_PIN_NUM(PIN_GPIO_TO_EQEPB), GPIO_PIN_LOW);

    Osal_delay(100); // 1/10 second 
    GPIOPinWrite_v0(GPIO_PIN_BASE_ADDR(PIN_GPIO_TO_EQEPA), GPIO_PIN_NUM(PIN_GPIO_TO_EQEPA), GPIO_PIN_HIGH);
    Osal_delay(100); // 1/10 second 
    GPIOPinWrite_v0(GPIO_PIN_BASE_ADDR(PIN_GPIO_TO_EQEPB), GPIO_PIN_NUM(PIN_GPIO_TO_EQEPB), GPIO_PIN_HIGH);
    Osal_delay(100); // 1/10 second 
    GPIOPinWrite_v0(GPIO_PIN_BASE_ADDR(PIN_GPIO_TO_EQEPA), GPIO_PIN_NUM(PIN_GPIO_TO_EQEPA), GPIO_PIN_LOW);
    Osal_delay(100); // 1/10 second 
    GPIOPinWrite_v0(GPIO_PIN_BASE_ADDR(PIN_GPIO_TO_EQEPB), GPIO_PIN_NUM(PIN_GPIO_TO_EQEPB), GPIO_PIN_LOW);

    // The encoder count at this point is 15


    position = eqep_get_position(EQEP1_P8_35_P8_33);
    printf("EQEP counter position after simulation: %d\n", position); // prints 15
}