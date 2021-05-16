#include "pit.h"
#include "asm_linkage.h"
#include "x86_desc.h"
#include "i8259.h"
#include "scheduler.h"


/*
 * init_PIT
 *    DESCRIPTION: Initialize PIT to 100Hz/10ms and turns on IRQ0 
 *    INPUTS: none
 *    OUTPUTS: none
 *    RETURNS: none
 *    SIDE EFFECTS: Turns on periodic interrupts for scheduler
 *    NOTES: 
 */
void init_PIT(){
    //cli();
    outb(PIT_MODE_2, PIT_MODE_REG);  // Select PIT channel 0, lobyte/hibyte access, rate gen. mode
    outb(PIT_FREQ & 0xFF, PIT_CH0);       // Set low byte of PIT reload value
    outb((PIT_FREQ & 0xFF00)>>8, PIT_CH0);        // Set high byte of PIT reload value
    SET_IDT_ENTRY(idt[0x20], &PIT_processor);   // Set entry on IDT
    enable_irq(PIT_IRQ);    // Enable IRQ on PIC
    //sti();
}

/*
 * PIT_interrupt
 *    DESCRIPTION: Calls scheduler on every PIT interrupt
 *    INPUTS: none
 *    OUTPUTS: none
 *    RETURNS: none
 *    SIDE EFFECTS: none
 *    NOTES: 
 */ 
void PIT_handler(){
    send_eoi(PIT_IRQ);  //supplemental session included this before calling scheduler helper
    scheduler();        //PIT handler calls scheduling algorithm
}
