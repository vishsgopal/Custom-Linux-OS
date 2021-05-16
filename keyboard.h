/* keyboard.h - declarations and macros for keyboard
 *  vim:ts=4 noexpandtab
 */

// http://www.philipstorr.id.au/pcbook/book3/scancode.htm

#ifndef _KEYBOARD_H
#define _KEYBOARD_H

#include "types.h"

#define KEYBOARD_PORT           0x60
#define KEYBOARD_IRQ            0x01
#define LEFT_SHIFT_PRESSED      0x2A
#define RIGHT_SHIFT_PRESSED     0x36
#define LEFT_SHIFT_RELEASED     0xAA
#define RIGHT_SHIFT_RELEASED    0xB6
#define CAPS_LOCK_PRESSED       0x3A
#define LEFT_CTRL_PRESSED       0x1D
#define LEFT_CTRL_RELEASED      0x9D
#define LEFT_ALT_PRESSED        0x38
#define LEFT_ALT_RELEASED       0xB8
#define KEYBOARD_BUF_SIZE       128
#define KEYBOARD_BUF_CHAR_MAX   127
#define TERMINAL_ONE            0x3B
#define TERMINAL_TWO            0x3C
#define TERMINAL_THREE          0x3D

//------------------------VARS DEPRECATED IN CP5------------------------------ 

// Line buffer for keyboard entries
//char keyboard_buf[KEYBOARD_BUF_SIZE]; (IS NOW "kb_buf" IN TERMINAL STRUCT DUE TO MULTI-TERMINAL)

// Keyboard buffer index to keep track of buffer pos
//int keyboard_buf_i; (IS NOW "kb_buf_i" IN TERMINAL STRUCT DUE TO MULTI-TERMINAL)

// Tracks if enter was pressed so terminal_read can continue
//volatile int enter_flag; (IS NOW "kb_enter_flag" TERMINAL STRUCT DUE TO MULTI-TERMINAL)

//--------------------------END DEPRECATED VARS-----------------------------

// Holds the arg "n_bytes" passed into terminal_read (is this meaningless?)
int terminal_buf_n_bytes;

// Keyboard flags
int left_shift_flag;

int right_shift_flag;

int ctrl_flag;

int caps_flag;

int alt_flag;

// Initialize the keyboard by enabling the PIC IRQ
void init_keyboard();

// Handles Keyboard interrupts
extern void keyboard_handler();

#endif /* _KEYBOARD_H */
