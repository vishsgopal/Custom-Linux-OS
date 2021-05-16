/* idt.c (MP3.1) - All IDT related functions
 * vim:ts=4 noexpandtab
 */

#include "idt.h"

#include "asm_linkage.h"

#include "keyboard.h"

#include "rtc.h"

#include "i8259.h"

#include "lib.h"

#include "system_calls.h"


/*
* enter all relevant exceptions into IDT table
*/


// Refer to OSDEV for paging: flush, map page directory to memory correctly

/*
 * init_IDT
 *    DESCRIPTION: Initialize IDT but does not set the handler offset fields for each vector
 *    INPUTS: none
 *    OUTPUTS: none
 *    RETURNS: none
 *    SIDE EFFECTS: Fills entries of IDT as interrupt gates but without setting offset fields
 *    NOTES: See OSDev and Intel manual for how to set up interrupt gates in the IDT
 *           The offset fields should be initialized using macro SET_IDT_ENTRY(str, handler)
 *               upon creating those functions
 */ 
void init_IDT(){
    int i;                                      // Loop index
    for(i = 0; i < NUM_VEC; i++) {
        
        // Advance to vector 0x20 as we won't fill in vectors 0x14 thru 0x1F (see 3.1 hints)
        if(i == 0x14)
            i = 0x20;
        // Skip vector 15 as it's reserved by Intel (tests.c says use it for assertions?)
        else if(i == 15)
            continue;

        idt_desc_t next_entry;                  // Next idt entry that we are initializing
        next_entry.seg_selector = KERNEL_CS;    // initialize to kernel's code segment descriptor
        next_entry.reserved4 = 0x0;             // interrupt gate reserved bits are 0 0 1 1 0 for 32 bit size
        next_entry.reserved3 = 0x0;
        next_entry.reserved2 = 0x1;
        next_entry.reserved1 = 0x1;
        next_entry.size = 0x1;                  // size always 32-bits
        next_entry.reserved0 = 0x0;     
        next_entry.dpl = 0x0;                   // set privilege level
        next_entry.present = 0x1;               // set present to 1 to show that this interrupt is active
        
        // The offset fields should be initialized using macro SET_IDT_ENTRY(str, handler), not here

        // If we're filling in system call vector (0x80), change DPL to user ring
        if(i == 0x80)
            next_entry.dpl = 0x03;              // System calls should always have user level DPL (ring 3)

        // Turns out that the union means that either val or the struct can be used to represent the bits

        // Populate IDT vector with new entry
        idt[i] = next_entry;    
    }

    /*
    * enter in exceptions into the appropriate indices of the IDT table
    */
    SET_IDT_ENTRY(idt[0], &divide_by_zero);             //exception 0
    SET_IDT_ENTRY(idt[1], &debug);                      //exception 1
    SET_IDT_ENTRY(idt[2], &nm_interrupt);               //exception 2
    SET_IDT_ENTRY(idt[3], &breakpoint);                 //exception 3
    SET_IDT_ENTRY(idt[4], &overflow);                   //exception 4
    SET_IDT_ENTRY(idt[5], &br_exceeded);                //exception 5
    SET_IDT_ENTRY(idt[6], &inv_opcode);                 //exception 6
    SET_IDT_ENTRY(idt[7], &device_na);                  //exception 7
    SET_IDT_ENTRY(idt[8], &double_fault);               //exception 8
    SET_IDT_ENTRY(idt[9], &cp_seg_overrun);             //exception 9
    SET_IDT_ENTRY(idt[10], &inv_tss);                   //exception 10
    SET_IDT_ENTRY(idt[11], &seg_not_present);           //exception 11
    SET_IDT_ENTRY(idt[12], &stack_fault);               //exception 12
    SET_IDT_ENTRY(idt[13], &gen_protection);            //exception 13
    SET_IDT_ENTRY(idt[14], &page_fault);                //exception 14
    SET_IDT_ENTRY(idt[16], &fpu_floating_point);        //exception 16 (exception 15 is reserved by Intel)
    SET_IDT_ENTRY(idt[17], &alignment_check);           //exception 17
    SET_IDT_ENTRY(idt[18], &machine_check);             //exception 18
    SET_IDT_ENTRY(idt[19], &simd_floating_point);       //exception 19
    
    /*initialize 0x80 index in IDT for system calls, see Appendix B*/
    SET_IDT_ENTRY(idt[0x80], &systems_handler);         

    // IMPORTANT: Piazza (@882) said that we "shouldn't hard code DEVICE handlers into the IDT"
    // Moved keyboard set_idt_entry to init_keyboard in keyboard.c
    // Moved RTC set_idt_entry to init_RTC in rtc.c
    exception_flag = 0;
}

// Handles interrupt (print error message and other relevant items like regs)
// Is stack trace required?
void exception_handler(int32_t interrupt_vector){
    //clear();
    switch(interrupt_vector){
        case 0xFFFFFFFF:
            printf(" Divide By Zero Exception\n");             //print all resepctive exceptions
            halt_wrapper();
        case 0xFFFFFFFE:
            printf(" Debug Exception\n");
            halt_wrapper();
        case 0xFFFFFFFD:
            printf(" Non-masking Interrupt Exception\n");
            halt_wrapper();
        case 0xFFFFFFFC:
            printf(" Breakpoint Exception\n");
            halt_wrapper();
        case 0xFFFFFFFB:
            printf(" Overflow Exception\n");
            halt_wrapper();
        case 0xFFFFFFFA:
            printf(" Bound Range Exception\n");
            halt_wrapper();
        case 0xFFFFFFF9:
            printf(" Invalid Opcode Exception\n");
            halt_wrapper();
        case 0xFFFFFFF8:
            printf(" Device Not Available\n");
            halt_wrapper();
        case 0xFFFFFFF7:
            printf(" Double Fault Exception\n");
            halt_wrapper();
        case 0xFFFFFFF6:
            printf(" Coprocessor Segment Overrun\n");
            halt_wrapper();
        case 0xFFFFFFF5:
            printf(" Invalid TSS Exception\n");
            halt_wrapper();
        case 0xFFFFFFF4:
            printf(" Segment Not Present\n");
            halt_wrapper();
        case 0xFFFFFFF3:
            printf(" Stack Fault Exception\n");
            halt_wrapper();
        case 0xFFFFFFF2:
            printf(" General Protection Exception\n");
            halt_wrapper();
        case 0xFFFFFFF1:
            printf(" Page-Fault Exception\n");
            halt_wrapper();
        case 0xFFFFFFEF:
            printf(" x87 FPU Floating-Point Error\n");
            halt_wrapper();
        case 0xFFFFFFEE:
            printf(" Alignment Check Exception\n");
            halt_wrapper();
        case 0xFFFFFFED:
            printf(" Machine-Check Exception\n");
            halt_wrapper();
        case 0xFFFFFFEC:
            printf(" SIMD Floating-Point Exception\n");
            halt_wrapper();
    }
}

/*
 * halt_wrapper
 *    DESCRIPTION: Transitions to the halt syscall and setting exception_flag (Not rly a wrapper)
 */
void halt_wrapper() {
    exception_flag = 1;
    halt(255);
}
