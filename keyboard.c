/* keyboard.c - implementation for keyboard
 *  vim:ts=4 noexpandtab
 */

#include "keyboard.h"
#include "lib.h"
#include "asm_linkage.h"
#include "x86_desc.h"
#include "terminal.h"
#include "i8259.h"


/*
 * init_keyboard
 *    DESCRIPTION: Initializes the keyboard by setting the IDT entry and PIC IRQ
 *    INPUTS/OUTPUTS: none  
 */
void init_keyboard(){
    enable_irq(KEYBOARD_IRQ);
    SET_IDT_ENTRY(idt[0x21], &keyboard_processor);        //index 21 of IDT reserved for keyboard
}

/*
 * keyboard_handler
 *    DESCRIPTION: Handler for keyboard interrupts
 *    INPUTS/OUTPUTS: none  
 */
void keyboard_handler() {
    /* scan_code stores the hex value of the key that is stored in the keyboard port */
    // Scan code + 0x80 is that key but released/"de-pressed"
    // ASCII + 0x20 is the lower case of that letter
    
    // Index is the scan code, the value at an index is that scan code key's ASCII
    int scan_code_to_ascii[] = {
    0, 0x1B, '1', '2', '3', '4', '5', '6', '7', '8',
    '9', '0', '-', '=', '\b', '\t', 'q', 'w', 'e', 'r',
    't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n', 0,
    'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';',
    '\'', '`', 0, '\\', 'z', 'x', 'c', 'v', 'b', 'n',
    'm', ',', '.', '/', 0, '*', 0, ' ', 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, '7', '8', '9', '-', '4', '5', '6', '+', '1',
    '2', '3', '0', '.', 0, 0, 0, 0, 0
    };

    int temp_i;   // Temp var used for tab loop index

    // Define alias vars for readability (using visible_terminal as the keyboard only types to visible terminal)
    char * kb_buf = terminals[visible_terminal].kb_buf;
    volatile int32_t * kb_buf_i = &terminals[visible_terminal].kb_buf_i;
    char print_allowed = terminals[visible_terminal].in_terminal_read; // Only allow keyboard to putc if in terminal_read

    // Get scan code from keyboard
    int scan_code = inb(KEYBOARD_PORT);

    // Set flags is control character key is pressed
    switch(scan_code){
        case LEFT_SHIFT_PRESSED:
            left_shift_flag = 1;
            break;
        case LEFT_SHIFT_RELEASED:
            left_shift_flag = 0;
            break;
        case RIGHT_SHIFT_PRESSED:
            right_shift_flag = 1;
            break;
        case RIGHT_SHIFT_RELEASED:
            right_shift_flag = 0;
            break;
        case CAPS_LOCK_PRESSED:
            caps_flag = !caps_flag;
            break;
        case LEFT_CTRL_PRESSED:
            ctrl_flag = 1;
            break;
        case LEFT_CTRL_RELEASED:
            ctrl_flag = 0;
            break;
        case LEFT_ALT_PRESSED:
            alt_flag = 1;
            break;
        case LEFT_ALT_RELEASED:
            alt_flag = 0;
            break;
    }

    // Ignore key releases (F4 pressed is 0x3B, any scan codes greater than that are releases)
    if(scan_code >= 0x3E || scan_code == LEFT_SHIFT_PRESSED || scan_code == RIGHT_SHIFT_PRESSED || scan_code == CAPS_LOCK_PRESSED ||
        scan_code == LEFT_CTRL_PRESSED || scan_code == LEFT_ALT_PRESSED) {
        send_eoi(KEYBOARD_IRQ);
        return;
    }

    // Convert scan code to ASCII equivalent
    char key_pressed = scan_code_to_ascii[scan_code];

    // Backspace pressed: delete prev char if buffer isn't empty, then return from interrupt
    if(key_pressed == '\b') {
        if(*kb_buf_i > 0) {
            (*kb_buf_i)--;
            if(print_allowed)
                putc('\b',1);
        }
        send_eoi(KEYBOARD_IRQ);
        return;
    }
    
    // If enter pressed, print newline, set enter_flag, and return from interrupt (terminal_read will clear buf)
    if(key_pressed == '\n') {
        // If visible_terminal isn't in terminal_read, pressing enter should do nothing 
        if(!print_allowed) {
            send_eoi(KEYBOARD_IRQ);
            return;
        }
        kb_buf[*kb_buf_i] = key_pressed;
        (*kb_buf_i)++;
        terminals[visible_terminal].kb_enter_flag = 1;
        if(print_allowed)
            putc('\n',1);
        send_eoi(KEYBOARD_IRQ);
        return;
    }

    // Ctrl + l and Ctrl + L clears screen and prints keyboard buffer again
    if(ctrl_flag && (key_pressed == 'l' || key_pressed == 'L')) {
        clear();
        if(print_allowed) {
            for(temp_i = 0; temp_i < *kb_buf_i; temp_i++) {
                putc((uint8_t)kb_buf[temp_i], 1);
            }
        }
        send_eoi(KEYBOARD_IRQ);
        return;
    }

    // Case for alt flag for terminal switching
    if(alt_flag) {
        // Alt + F1
        if(scan_code==TERMINAL_ONE){    
            switch_visible_terminal(0);       //pass in terminal id to switch to
            send_eoi(KEYBOARD_IRQ);
            return;
        }

        // Alt + F2
        if(scan_code==TERMINAL_TWO){
            switch_visible_terminal(1);
            send_eoi(KEYBOARD_IRQ);
            return;
        }

        // Alt + F3
        if(scan_code == TERMINAL_THREE) {
            switch_visible_terminal(2);
            send_eoi(KEYBOARD_IRQ);
            return;
        }
    }

    // Ignore any non-printing chars (placed here as we don't want to overlook terminal switcher)
    if(key_pressed == 0) {
        send_eoi(KEYBOARD_IRQ);
        return;
    }

    // If entering a char will overflow either buffer (only buf_size-1 chars + '\n' allowed), ignore the key press
    if(*kb_buf_i == KEYBOARD_BUF_CHAR_MAX || *kb_buf_i == terminal_buf_n_bytes - 1) {
        send_eoi(KEYBOARD_IRQ);
        return;
    }

    if(key_pressed == '\t') {
        // Tab = 8 spaces, clipping on overflow
        for(temp_i = 0; temp_i < 8; temp_i++) {
            if(*kb_buf_i < KEYBOARD_BUF_CHAR_MAX && *kb_buf_i < terminal_buf_n_bytes - 1) {
                kb_buf[*kb_buf_i] = ' ';
                (*kb_buf_i)++;
                if(print_allowed)
                    putc(' ',1);
            }
            else 
                break;
        }
        send_eoi(KEYBOARD_IRQ);
        return;
    }

    // Handle caps lock and shift for letter keys (ignored if shift is held by XOR)
    if((caps_flag ^ (left_shift_flag || right_shift_flag)) && (key_pressed >= 97 && key_pressed <= 122)){
        key_pressed = key_pressed - 32; // Subtracting letter ASCII by 32 maps to caps
    }

    // Handle shift for special characters
    if((left_shift_flag || right_shift_flag)) {
        // Map special characters
        switch(key_pressed){
            case '`':
                key_pressed = '~';
                break;
            case '1':
                key_pressed = '!';
                break;
            case '2':
                key_pressed = '@';
                break;
            case '3':
                key_pressed = '#';
                break;
            case '4':
                key_pressed = '$';
                break;
            case '5':
                key_pressed = '%';
                break;
            case '6':
                key_pressed = '^';
                break;
            case '7':
                key_pressed = '&';
                break;
            case '8':
                key_pressed = '*';
                break;
            case '9':
                key_pressed = '(';
                break;
            case '0':
                key_pressed = ')';
                break;
            case '-':
                key_pressed = '_';
                break;
            case '=':
                key_pressed = '+';
                break;
            case '[':
                key_pressed = '{';
                break;
            case ']':
                key_pressed = '}';
                break;
            case '\\':
                key_pressed = '|';
                break;
            case ';':
                key_pressed = ':';
                break;
            case '\'':
                key_pressed = '"';
                break;
            case ',':
                key_pressed = '<';
                break;
            case '.':
                key_pressed = '>';
                break;
            case '/':
                key_pressed = '?';
                break;
        }
    }
    
    // Put key pressed in buffer and on screen and advance buffer index
    kb_buf[*kb_buf_i] = key_pressed;
    (*kb_buf_i)++;  
    if(print_allowed)
        putc(key_pressed,1);
    
    // Send EOI to PIC
    send_eoi(KEYBOARD_IRQ);         // 0x01 is IRQ number for keyboard
}

