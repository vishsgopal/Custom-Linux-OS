/* rtc.c - implementation of real time clock device
 *  vim:ts=4 noexpandtab
 */

#include "rtc.h"
#include "asm_linkage.h"
#include "x86_desc.h"
#include "i8259.h"
#include "terminal.h"


/* List of usable RTC frequencies as bitmaps to Register A's lowest 4 bits
   Ordered as 2^(index + 1) Hz but we don't go above 1024 Hz*/
// unsigned char freq_list[10] = {0x0F, 0x0E, 0x0D, 0x0C, 0x0B, 0x0A, 0x09, 0x08, 0x07, 0x06};

/*
 * init_RTC
 *    DESCRIPTION: Initialize RTC to 1024Hz and turns on IRQ8
 *    INPUTS: none
 *    OUTPUTS: none
 *    RETURNS: none
 *    SIDE EFFECTS: Turns on periodic interupts
 *    NOTES: See OSDev links in .h file to understand macros
 */ 
void init_RTC() {

    //cli();                              // important that no interrupts happen (perform a CLI)
    outb(DISABLE_NMI_B, RTC_PORT);		// select register B, and disable NMI
    uint32_t prev = inb(CMOS_PORT);	        // read the current value of register B
    outb(DISABLE_NMI_B, RTC_PORT);		// set the index again (a read will reset the index to register D)
    outb(prev | 0x40, CMOS_PORT);	    // write the previous value ORed with 0x40. This turns on bit 6 of register B
    
    // Get current value of register
    outb(REGISTER_A, RTC_PORT);
    prev = inb(CMOS_PORT); 

    // Clear existing frequency bits
    prev &= 0xF0;

    // Set RTC to maximum freq. of 1024 Hz
    outb(prev | HIGHEST_FREQ_BITMASK, CMOS_PORT);

    enable_irq(RTC_IRQ);
    SET_IDT_ENTRY(idt[0x28], &RTC_processor);             //index 28 of IDT reserved for RTC
    //sti();                      // (perform an STI) and reenable NMI if you wish? 

}

/*
 * RTC_interrupt
 *    DESCRIPTION: RTC register C needs to be read, so interupts will happen again
 *    INPUTS: none
 *    OUTPUTS: none
 *    RETURNS: none
 *    SIDE EFFECTS: set RTC_int to 1
 *    NOTES: See OSDev links in .h file to understand macros
 */ 
void RTC_interrupt(){
    outb(REGISTER_C, RTC_PORT);	    // select register C
    inb(CMOS_PORT);		            // just throw away contents
    int i;
    
    // Decrement countdowns and determine if virtual interrupt should occur
    for(i = 0; i < MAX_TERMINALS; i++) {
        if(terminals[i].rtc_active) {
            terminals[i].rtc_countdown--;
            if(terminals[i].rtc_countdown == 0) {
                terminals[i].rtc_virt_interrupt = 1;
                terminals[i].rtc_countdown = HIGHEST_FREQ / terminals[i].rtc_freq;  // Reset countdown
            }
        }
    }

    send_eoi(RTC_IRQ);
    
    //test_interrupts();
}

/*
 * RTC_open
 *    DESCRIPTION: Set virtual RTC frequency to 2 Hz and set countdown var
 *    INPUTS: none
 *    OUTPUTS: none
 *    RETURNS: Always returns 0
 *    SIDE EFFECTS: Sets terminal's virtual RTC frequency to 2 Hz
 *    NOTES: See OSDev links in .h file to understand macros
 */
int32_t RTC_open(const uint8_t * filename) {
    terminals[scheduled_terminal].rtc_freq = 2;
    terminals[scheduled_terminal].rtc_active = 1;
    
    // To virtualize, we've set the RTC to its max of 1024Hz, but to get it to do one interrupt at the
    // desired frequency, we must divide 1024 by this desired frequency to find the number of interrupts
    // at 1024Hz to see how many interrupts at 1024 is equivalent to one interrupt at desired frequency.  
    terminals[scheduled_terminal].rtc_countdown =  HIGHEST_FREQ / terminals[scheduled_terminal].rtc_freq;
    return 0;
}

/*
 * RTC_read
 *    DESCRIPTION: Blocks system until RTC interrupt
 *    INPUTS: none
 *    OUTPUTS: none
 *    RETURNS: Always returns 0
 *    SIDE EFFECTS: Blocks system until RTC interrupt
 *    NOTES: none
 */
int32_t RTC_read(int32_t fd, void * buf, int32_t n_bytes) {
    // Block until the interrupt handler determines that the scheduled terminal has hit its virtual RTC interrupt
    while(!terminals[scheduled_terminal].rtc_virt_interrupt);
    // Reset virtual interrupt flag
    terminals[scheduled_terminal].rtc_virt_interrupt = 0;
    return 0;
}

/*
 * RTC_write
 *    DESCRIPTION: Changes frequency of RTC to input value
 *    INPUTS: buf -- input buffer that holds the new frequency
 *    OUTPUTS: none
 *    RETURNS: 0 if successful, -1 if input is not a power of 2 or is invalid
 *    SIDE EFFECTS: Changes RTC frequency to input value
 *    NOTES: Input cannot surpass 1024, check is at line 133
 */
int32_t RTC_write(int32_t fd, const void * buf, int32_t n_bytes) {
    uint32_t freq;          // Hold desired RTC frequency
    uint8_t index;          // Holds index of where the 1 bit is in the buffer
    uint8_t flag = 0;       // Checks how many bits of input buffer are set

    // Check null pointer
    if(buf == 0)
        return -1;

    // Read buffer and ignore LSB as frequency must be power of 2 excluding 1
    freq = *((uint8_t*)buf);
    index = 0;

    // If LSB is set in the first place, it's invalid
    if(freq % 2)
        return -1;

    // Ignore LSB
    freq >>= 1;
    
    // If there is no bit set that is a power of 2 excluding 1, it's invalid (or if freq was 0 in the first place)
    if(freq == 0)
        return -1;

    // Check if input is power of 2
    while(freq){
        
        // Check if input is above 1024 (bit 10), if so, it's invalid
        if(index > 10)
            return -1;
        
        // Check if next bit is 1
        if(freq % 2)
            flag++;
        
        // If there isn't exactly one 1, it's not a power of 2 and is invalid
        if(flag > 1)
            return -1;

        // Increment index and advance
        index++;
        freq >>= 1;
    }

    // Set the new frequency and countdown in the terminal struct
    terminals[scheduled_terminal].rtc_freq = *((uint8_t*)buf);
    terminals[scheduled_terminal].rtc_countdown = HIGHEST_FREQ / terminals[scheduled_terminal].rtc_freq;
    return 0;
}

/*
 * RTC_close
 *    DESCRIPTION: Set terminal's virtual RTC as inactive
 *    INPUTS: none
 *    OUTPUTS: none
 *    RETURNS: Always returns 0
 *    SIDE EFFECTS: none
 *    NOTES: none
 */
int32_t RTC_close(int32_t fd) {
    terminals[scheduled_terminal].rtc_freq = 0;
    terminals[scheduled_terminal].rtc_active = 0;
    terminals[scheduled_terminal].rtc_countdown = 0;
    terminals[scheduled_terminal].rtc_virt_interrupt = 0;
    return 0;
}
