/* terminal.c - terminal driver functions
 *  vim:ts=4 noexpandtab
 */

#include "terminal.h"
#include "lib.h"
#include "paging.h"
#include "system_calls.h"


/*
 * init_terminal
 *    DESCRIPTION: Initalizes all terminal-related variables for multi-terminal setup
 *    INPUTS: none
 *    OUTPUTS: none
 *    RETURN VALUE: none
 *    SIDE EFFECTS: Initializes all terminal-related vars
 */
void init_terminal(){
    // Find next available terminal ID to assign
    int i; 
    shell_count = 0;
    scheduled_terminal = 0;
    visible_terminal = 0;
    for(i = 0; i < MAX_TERMINALS; i++) {    
        terminals[i].terminal_pcb = NULL;      // will be implemented in scheduler
        terminals[i].terminal_id = i;          //identifies terminal
        terminals[i].cursor_x = 0;
        terminals[i].cursor_y = 0;
        terminals[i].last_assigned_pid = -1;   // flag as no process running
        terminals[i].rtc_freq = 0;             // Set RTC freq for that terminal to 0Hz (set later by RTC_open and write)
        terminals[i].rtc_active = 0;
        terminals[i].rtc_countdown = 0;
        terminals[i].rtc_virt_interrupt = 0;
        terminals[i].in_terminal_read = 0;
        clear_keyboard_vars(i);                 // Initialize each terminal's keyboard buffer 
    }
}

/*
 * clear_keyboard_vars
 *    DESCRIPTION: Zeros keyboard buffer, resets enter flag and buffer index
 *    INPUTS: terminal_id -- the terminal whose keyboard vars are to be reset 
 *    OUTPUTS: none
 *    RETURN VALUE: none
 *    SIDE EFFECTS: clears keyboard buffer and resets kb flags and buffer index  
 */
void clear_keyboard_vars(int32_t terminal_id) {
    if(terminal_id < 0 || terminal_id >= MAX_TERMINALS)
        return;
    int i;      // Loop index
    terminals[terminal_id].kb_buf_i = 0;        //set keyboard index to start
    terminals[terminal_id].kb_enter_flag = 0;   //enter key has not been pressed
    for(i = 0; i < KEYBOARD_BUF_SIZE; i++) {
        terminals[terminal_id].kb_buf[i] = 0;   //clear keyboard buffer
    }
}

/*
 * terminal_open
 *    DESCRIPTION: Open the terminal (stdin)
 *    INPUTS: none
 *    OUTPUTS: none
 *    RETURN VALUE: Always 0 (success)
 *    SIDE EFFECTS: none
 */
int32_t terminal_open(int32_t fd) {
    return 0;
}

/*
 * terminal_close
 *    DESCRIPTION: This should never be called
 *    INPUTS: none
 *    OUTPUTS: none
 *    RETURN VALUE: Always -1 as terminal should never be closed
 *    SIDE EFFECTS: none
 */
int32_t terminal_close(int32_t fd) {
    return -1;
}

/*
 * terminal_read
 *    DESCRIPTION: Reads from the buffer 
 *    INPUTS: file descriptor, buf -- ptr to output buffer that we copy keyboard_buf to, n_bytes
 *    OUTPUTS: copies buf to terminal_buf
 *    RETURN VALUE: Number of bytes written
 *    SIDE EFFECTS: Writes to buffer pointed to by input
 */
int32_t terminal_read(int32_t fd, void * buf, int32_t n_bytes) {
    
    // NULL check input and return 0 if NULL to signify no bytes read
    if(buf == 0 || n_bytes <= 0)
        return 0; 

    // Let keyboard know how many bytes the buffer is (is this meaningless?)
    terminal_buf_n_bytes = n_bytes;

    // Set flag to allow keyboard inputs to write to screen
    terminals[scheduled_terminal].in_terminal_read = 1;

    // Block until enter ('\n') has been pressed for the scheduled terminal
    while(!terminals[scheduled_terminal].kb_enter_flag);

    // Clear flag to have keyboard inputs be invisible
    terminals[scheduled_terminal].in_terminal_read = 0;
    
    // Alias vars for readability (using scheduled_terminal as we might be in a background process)
    char * kb_buf = terminals[scheduled_terminal].kb_buf;
    volatile int32_t * kb_buf_i = &terminals[scheduled_terminal].kb_buf_i;
    
    // Return value var as we clear the terminal's keyboard info at the end
    int32_t bytes_written;

    // Strip any extraneous chars after '\n' due to race cond in issue #18's reopen
    if(kb_buf_i > 0) { 
        if(kb_buf[*kb_buf_i - 1] != '\n') {
            kb_buf[*kb_buf_i - 1] = '\0';
            (*kb_buf_i)--; // decrement index of kb
        }
    }

    // Copy keyboard buf to input buf
    (void)strncpy((int8_t *)buf, (int8_t *)kb_buf, n_bytes);

    // Copy keyboard_buf index, which is the same as the number of bytes read from keyboard_buf including '\n'
    bytes_written = *kb_buf_i;

    // Reset so the keyboard buffer can write to its normal size
    terminal_buf_n_bytes = KEYBOARD_BUF_SIZE;
    clear_keyboard_vars(scheduled_terminal);

    return bytes_written;
}

/*
 * terminal_write
 *    DESCRIPTION: Writes the bytes from input buf to the screen
 *    INPUTS: buf -- bytes to write to screen
 *    OUTPUTS: none
 *    RETURN VALUE: number of bytes/chars written to screen
 *    SIDE EFFECTS: writes to video memory using putc
 */
int32_t terminal_write(int32_t fd, const void * buf, int32_t n_bytes) {

    int i;      // Loop index

    // NULL check input
    if(buf == 0 || n_bytes < 0)
        return -1;

    // Print n_bytes worth of chars
    for(i = 0; i < n_bytes; i++) {
        putc(((char *)(buf))[i], 0);
    }

    /* Stops the shell prompt from being backspaced if we type before the prompt is printed (like during fish)
       Or, if the user types when the user program isn't expecting keyboard inputs, reveal when we return to shell
       Prints out the keyboard buffer again if the shell prompt (length 7) is the argument */
    if(!strncmp((int8_t *)buf, "391OS> ", 7) && shell_count == 3) {
        terminal_write(NULL, terminals[scheduled_terminal].kb_buf, terminals[scheduled_terminal].kb_buf_i);
    }

    return i;
}

/*
 * switch_visible_terminal
 *    DESCRIPTION: Switches to the desired terminal
 *    INPUTS: terminal_id -- the terminal to switch to
 *    OUTPUTS: none
 *    RETURN VALUE: none
 *    SIDE EFFECTS: Switches to the desired terminal (including video page)
 */
void switch_visible_terminal(int32_t terminal_id) {
    if(terminal_id == visible_terminal)  //check if we are switching to same terminal
        return;

    // Save visible terminal to its video page and load the one to change to
    change_terminal_video_page(visible_terminal, terminal_id);

    // Save cursor
    terminals[visible_terminal].cursor_x = get_screen_x();
    terminals[visible_terminal].cursor_y = get_screen_y();

    // Update keyboard buffer and screen coordinates
    update_cursor(terminals[terminal_id].cursor_x, terminals[terminal_id].cursor_y);

    visible_terminal = terminal_id;  // Update visible terminal ID to the one we switch to
}
