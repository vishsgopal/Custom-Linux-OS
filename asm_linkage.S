/* idt.S (MP3.1) : Contains assembly code that handles interrupts */
# vim:ts=4 noexpandtab

#define ASM     1

#include "asm_linkage.h"

.text

/*
* make all exception and device processors global to implement
* assembly linkage
*/
.globl divide_by_zero
.globl debug
.globl nm_interrupt
.globl breakpoint
.globl overflow
.globl br_exceeded
.globl inv_opcode
.globl device_na
.globl double_fault
.globl cp_seg_overrun
.globl inv_tss
.globl seg_not_present
.globl stack_fault
.globl gen_protection
.globl page_fault
.globl fpu_floating_point
.globl alignment_check
.globl machine_check
.globl simd_floating_point
.globl keyboard_processor
.globl RTC_processor
.globl systems_handler
.globl PIT_processor

/*
* moving esp into eax is unnecessary
*/

/*implementing assmebly linkage for idt exceptions*/
divide_by_zero: #0
    cli
    pushal              #saves all registers onto stack
    pushfl               #save flags
    pushl $0xFFFFFFFF         #prep for exception_processor arg
    //movl %esp, %eax
    jmp exception_processor

debug: #1               #for the rest of the exceptions, check DBZ exception for comments
    cli
    pushal
    pushfl               #save flags
    pushl $0xFFFFFFFE
    //movl %esp, %eax
    jmp exception_processor

nm_interrupt: #2
    cli
    pushal
    pushfl               #save flags
    pushl $0xFFFFFFFD
    //movl %esp, %eax
    jmp exception_processor

breakpoint: #3
    cli
    pushal
    pushfl               #save flags
    pushl $0xFFFFFFFC
    //movl %esp, %eax
    jmp exception_processor

overflow: #4
    cli
    pushal
    pushfl               #save flags
    pushl $0xFFFFFFFB
    //movl %esp, %eax
    jmp exception_processor

br_exceeded: #5
    cli
    pushal
    pushfl               #save flags
    pushl $0xFFFFFFFA
    //movl %esp, %eax
    jmp exception_processor

inv_opcode: #6
    cli
    pushal
    pushfl               #save flags
    pushl $0xFFFFFFF9
    //movl %esp, %eax
    jmp exception_processor

device_na: #7
    cli
    pushal
    pushfl               #save flags
    pushl $0xFFFFFFF8
    //movl %esp, %eax
    jmp exception_processor

double_fault: #8
    cli
    pushal
    pushfl               #save flags
    pushl $0xFFFFFFF7
    //movl %esp, %eax
    jmp exception_processor

cp_seg_overrun: #9
    cli
    pushal
    pushfl               #save flags
    pushl $0xFFFFFFF6
    //movl %esp, %eax
    jmp exception_processor

inv_tss: #10
    cli
    pushal
    pushfl               #save flags
    pushl $0xFFFFFFF5
    //movl %esp, %eax
    jmp exception_processor

seg_not_present: #11
    cli
    pushal
    pushfl               #save flags
    pushl $0xFFFFFFF4
    //movl %esp, %eax
    jmp exception_processor

stack_fault: #12
    cli
    pushal
    pushfl               #save flags
    pushl $0xFFFFFFF3
    //movl %esp, %eax
    jmp exception_processor

gen_protection: #13
    cli
    pushal
    pushfl               #save flags
    pushl $0xFFFFFFF2
    //movl %esp, %eax
    jmp exception_processor

page_fault: #14
    cli
    pushal
    pushfl               #save flags
    pushl $0xFFFFFFF1
    //movl %esp, %eax
    jmp exception_processor

fpu_floating_point: #16
    cli
    pushal
    pushfl               #save flags
    pushl $0xFFFFFFEF
    //movl %esp, %eax
    jmp exception_processor

alignment_check: #17
    cli
    pushal
    pushfl               #save flags
    pushl $0xFFFFFFEE
    //movl %esp, %eax
    jmp exception_processor

machine_check: #18
    cli
    pushal
    pushfl               #save flags
    pushl $0xFFFFFFED
    //movl %esp, %eax
    jmp exception_processor

simd_floating_point: #19
    cli
    pushal
    pushfl               #save flags
    pushl $0xFFFFFFEC
    //movl %esp, %eax
    jmp exception_processor

exception_processor:            #passes interrupt vector into exception_handler
    call exception_handler
    addl $4, %esp               #clear arg from stack
    /* ret_from_intr below*/
    popfl                       #restore all registers and flags
    popal
    sti
    iret

/*implementing assembly linkage for device interrupts*/
keyboard_processor:             #once keyboard interrupt occurs, call keyboard handler
    cli
    pushal 
    call keyboard_handler
    popal
    sti 
    iret 

RTC_processor:                  #once RTC interrupt occurs, call RTC_interrupt handler
    cli
    pushal 
    call RTC_interrupt
    popal
    sti 
    iret 

PIT_processor:
    cli 
    pushal 
    call PIT_handler
    popal 
    sti 
    iret 

/*implementing assembly linkage for system calls*/
systems_handler:
    cmpl $1, %eax       //make sure that system call stored in %eax is between 1 and 6 (for CP3)
    jl invalid_syscall
    cmpl $10, %eax
    jg invalid_syscall

    pushl %ebp          //save all registers, see OSDev
    pushl %edi
    pushl %esi
    pushl %edx
    pushl %ecx 
    pushl %ebx 
    pushl %esp 

    pushl %edx          //push all 3 args, order specified in Appendix B
    pushl %ecx 
    pushl %ebx 

    call *systems_jump_table(,%eax,4)   //jump to the respective system call C function
    addl $12, %esp                      //clear args from stack
    jmp end_systems_handler

invalid_syscall:
    movl $-1, %eax
    iret

end_systems_handler:
    popl %esp           //restore all registers
    popl %ebx
    popl %ecx 
    popl %edx 
    popl %esi 
    popl %edi
    popl %ebp
    sti
    iret 

/*jump table that redirects to system call functions in C,
*0x0 is used as a placeholder since all system call numbers
*stored in %eax are between 1 and 10, see Appendix B*/
systems_jump_table:
    .long invalid_syscall, halt, execute, read, write, open, close, getargs, vidmap, set_handler, sigreturn 




