/* idt.h (MP3.1) - Defines for necessary functions and vars for IDT initialization
 *         Note that the ifndef-endif wrapping combined with the define is equivalent to a pragma once
 * vim:ts=4 noexpandtab
 */

#ifndef _IDT_H
#define _IDT_H

#include "lib.h"
#include "x86_desc.h"
//#ifndef ASM

#include "types.h"

// Exception flag
volatile int exception_flag;

// Initializes the IDT
extern void init_IDT();

// Handles exceptions thrown by the processor
extern void exception_handler(int32_t interrupt_vector);

// Wrapper for the halt system call used by the exceptions
void halt_wrapper();

//#endif /* ASM */
#endif /* _IDT_H */
