#ifndef _PIT_H
#define _PIT_H

#include "lib.h"
#include "types.h"

/*
 *   Helpful PIT links:
 *      https://wiki.osdev.org/Programmable_interval_timer
 */

#define PIT_PORT		    0x70
#define PIT_IRQ             0x00        //highest priority on PIC
#define PIT_CH0             0x40
#define PIT_MODE_REG        0x43
//Note: must be as responsive as possible, so we chose min frequency required
#define PIT_FREQ            11932       // 1193180/100Hz(10ms) for frequency
#define PIT_MODE_2          0x34

// Initialize the RTC and turn on IRQ8
void init_PIT();

// Handles interrupts from the real-time clock
extern void PIT_handler();

#endif /* _PIT_H */
