#include "scheduler.h"
#include "system_calls.h"
#include "terminal.h"
#include "i8259.h"
#include "pit.h"
#include "paging.h"
#include "x86_desc.h"
#include "rtc.h"
#include "keyboard.h"

/*
 * scheduler
 *    DESCRIPTION: Performs process switching and boots up all three terminals
 *    INPUTS: none
 *    OUTPUTS: none
 *    SIDE EFFECTS: Switches active process using round-robin scheduling
 */
void scheduler(){   
    // Get the current active process's PCB
    pcb_t * curr_pcb = terminals[scheduled_terminal].terminal_pcb;
        
    // Boot up the three terminals
    if(curr_pcb == NULL && shell_count < 3){
        pcb_t temp_pcb;
        asm volatile(       // Retrieve initial ESP and EBP before booting base shells
            "movl %%esp, %0;"
            "movl %%ebp, %1;"
            : "=r"(temp_pcb.curr_esp), "=r"(temp_pcb.curr_ebp) // Outputs
        );
        shell_count++;
        terminals[scheduled_terminal].terminal_pcb = &temp_pcb;
        terminals[scheduled_terminal].last_assigned_pid = scheduled_terminal;   //mark terminal as booted and initialize its pid

        switch_visible_terminal(scheduled_terminal);    // Switch video page so the bootup text stays in that terminal
        
        // Enable keyboard IRQ now that all 3 terminals are booted to avoid race condition during bootup
        if(shell_count == 3)
            init_keyboard();

        printf("Terminal %d booting...\n", shell_count);
        execute((uint8_t *)"shell");    //initial bootup for all three terminals
    }

    // Save old process's stack
    // This properly sets up first base shell's ESP/EBP by forcing it into this context
    asm volatile(       
        "movl %%esp, %0;"
        "movl %%ebp, %1;"
        : "=r"(curr_pcb->curr_esp), "=r"(curr_pcb->curr_ebp) // Outputs
    );

    // Round-robin increment
    scheduled_terminal = (scheduled_terminal + 1) % MAX_TERMINALS;

    // If the next terminal hasn't been booted yet, boot it on the next PIT interrupt
    if(terminals[scheduled_terminal].terminal_pcb == NULL)
        return;
    
    set_user_video_page(1); //sets up and marks user page for vidmem as present

    pcb_t * next_pcb = terminals[scheduled_terminal].terminal_pcb;

    // Remap user program page 
    set_user_prog_page(next_pcb->process_id, 1);

    // Update TSS
    tss.esp0 = EIGHT_MB - (next_pcb->process_id * EIGHT_KB) - 4;
    tss.ss0 = KERNEL_DS;

    asm volatile(       //Switch to next process's stack
        "movl %0, %%esp;"
        "movl %1, %%ebp;"
        : 
        :"r"(next_pcb->curr_esp), "r"(next_pcb->curr_ebp) // Inputs
        :"esp", "ebp"   //clobbers esp, ebp
    );
    
}
