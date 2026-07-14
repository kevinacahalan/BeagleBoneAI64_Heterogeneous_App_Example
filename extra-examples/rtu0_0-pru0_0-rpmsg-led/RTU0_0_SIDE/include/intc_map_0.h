/*
 * intc_map_0.h — RTU INTC map for RPMsg kicks to RTU0_0.
 * System event 21 (From ARM) → channel 10 → host interrupt 10 (R31 bit 30).
 */

#ifndef INTC_MAP_0_H
#define INTC_MAP_0_H

#include <stddef.h>
#include <rsc_types.h>

#pragma DATA_SECTION(my_irq_rsc, ".pru_irq_map")
#pragma RETAIN(my_irq_rsc)
struct pru_irq_rsc my_irq_rsc = {
	0, /* type */
	1, /* number of system events mapped */
	{
		{ 21, 10, 10 }, /* {sysevt, channel, host interrupt} */
	},
};

#endif /* INTC_MAP_0_H */
