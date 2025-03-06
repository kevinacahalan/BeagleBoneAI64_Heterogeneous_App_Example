#include <stdio.h>
#include <ai64/sys_eqep.h>

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

void test_eqep1_mmr() {
    eqep1_init(); // Initialize EQEP1

    while (1) {
        unsigned int position = eqep1_get_position();
        printf("Current Position: %u\n", position);
        for (volatile int i = 0; i < 1000000; ++i); // delay loop
    }
}