#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <ai64/sys_eqep.h>

#include <ai64/bbai64_eqep.h>

volatile eqep_t *get_eqep_module(enum eqep_encoder encoder) {
    switch (encoder)
    {
    case EQEP0_P9_42a_P9_27b:
        return &EQEP0;
    case EQEP1_P8_35_P8_33:
        return &EQEP1;
    default:
        exit(1); // Hopefully will cause crash
        return NULL; // Bad, should never happen
    }
}

// Function to initialize an EQEP encoder using only the A and B inputs
void eqep_init(enum eqep_encoder encoder) {
    volatile eqep_t *eqep = get_eqep_module(encoder);

    // Set the maximum position value
    eqep->QPOSMAX = 0xFFFFFFFF; // Set maximum count value

    // Clear any previous position
    eqep->QPOSCNT = 0;

    // Configure the decoder to use quadrature mode with A and B inputs only
    eqep->QDEC_QEP_CTL = 0x0000; // Reset control register
    eqep->QDEC_QEP_CTL |= (1 << 19); // Enable the quadrature position counter
    eqep->QDEC_QEP_CTL |= (0b00 << 14); // Quadrature count mode, no edges count
    eqep->QDEC_QEP_CTL |= (0b00 << 28); // Position-counter reset on maximum position

    // Enable unit timer to run continuously, automatic latching?
    eqep->QDEC_QEP_CTL |= (1 << 17); // Enable unit timer
    eqep->QUPRD = 1000000; // Set unit period to 1,000,000 for unit timer
}

// read current position
unsigned int eqep_get_position(enum eqep_encoder encoder) {
    volatile eqep_t *eqep = get_eqep_module(encoder);
    return eqep->QPOSCNT;
}

unsigned int eqep_add_offset(enum eqep_encoder encoder, unsigned int offset) {
    volatile eqep_t *eqep = get_eqep_module(encoder);
    eqep->QPOSCNT = eqep->QPOSCNT + offset;
    return eqep->QPOSCNT;
}

void eqep_reset(enum eqep_encoder encoder) {
    volatile eqep_t *eqep = get_eqep_module(encoder);
    // Clear any previous position
    eqep->QPOSCNT = 0;
}


