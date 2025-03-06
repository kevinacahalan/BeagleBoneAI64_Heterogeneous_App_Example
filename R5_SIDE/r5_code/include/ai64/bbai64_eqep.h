#ifndef BBAI64_EQEP_H
#define BBAI64_EQEP_H

#include <stdint.h>
#include <stdbool.h>

enum eqep_encoder
{
    EQEP0_P9_42a_P9_27b = 0,
    EQEP1_P8_35_P8_33,
    EQEP_INVALID,
};


extern void eqep_init(enum eqep_encoder encoder);
extern unsigned int eqep_get_position(enum eqep_encoder encoder);
extern unsigned int eqep_add_offset(enum eqep_encoder encoder, unsigned int offset);
extern void eqep_reset(enum eqep_encoder encoder);

#endif