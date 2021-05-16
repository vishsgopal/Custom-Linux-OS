/* rtc.h - declarations and macros for real time clock device
 *  vim:ts=4 noexpandtab
 */

// Helpful RTC-related links:
// https://wiki.osdev.org/RTC#Setting_the_Registers
// https://wiki.osdev.org/RTC#Turning_on_IRQ_8
// https://courses.engr.illinois.edu/ece391/sp2021/secure/references/ds12887.pdf slide 19 for RTC frequency

#ifndef _RTC_H
#define _RTC_H

#include "lib.h"
#include "types.h"

#define RTC_PORT		        0x70
#define RTC_IRQ                 0x08
#define CMOS_PORT		        0x71
#define DISABLE_NMI_A	        0x8A
#define DISABLE_NMI_B	        0x8B
#define DISABLE_NMI_C	        0x8C
#define REGISTER_A		        0x0A
#define REGISTER_B		        0x0B
#define REGISTER_C		        0x0C
#define HIGHEST_FREQ            1024
#define HIGHEST_FREQ_BITMASK    0x06         // Bitmask to set frequency to 1024Hz

// Initialize the RTC and turn on IRQ8
void init_RTC();

// Handles interrupts from the real-time clock
extern void RTC_interrupt();

// Initialize RTC to 2 Hz
int32_t RTC_open(const uint8_t* filename);

// Blocks system until RTC interrupt occurs
int32_t RTC_read(int32_t fd, void * buf, int32_t n_bytes);

// Sets RTC frequency
int32_t RTC_write(int32_t fd, const void * buf, int32_t n_bytes);

// Probably does nothing until we virtualize RTC
int32_t RTC_close(int32_t fd);

#endif /* _RTC_H */
