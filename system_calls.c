#include "x86_desc.h"
#include "system_calls.h"
#include "paging.h"
#include "lib.h"
#include "rtc.h"
#include "file_system.h"
#include "terminal.h"
#include "idt.h"

/*fops tables for different types*/
fops_jump_table_t rtc_table = {RTC_read, RTC_write, RTC_open, RTC_close};
fops_jump_table_t directory_table = {read_dir, write_dir, open_dir, close_dir};
fops_jump_table_t file_table = {read_file, write_file, open_file, close_file};

fops_jump_table_t stdin_table = {terminal_read,bad_call,bad_call,bad_call};
fops_jump_table_t stdout_table = {bad_call,terminal_write,bad_call,bad_call};

fops_jump_table_t bad_table = {bad_call,bad_call,bad_call,bad_call};

uint32_t processes[MAX_PROCESSES] = {0, 0, 0, 0, 0, 0}; // Array of flags (should they be PCBs?) to track currently running processes



/*
 * bad_call
 *    DESCRIPTION: Generic function called upon bad sycall for any invalid function type
 *    INPUTS/OUTPUTS: None
 *    RETURNS: Always -1
 */
int32_t bad_call(){
    return -1;
}

/*
 * halt
 *    DESCRIPTION: Cleans up process that just finished running
 *    INPUTS: status -- Return code of the halting process
 *    OUTPUTS: none
 *    RETURNS: should never return, it should instantly jump to execute
 */
int32_t halt(uint8_t status){

    pcb_t *pcb_ptr =(pcb_t*)terminals[scheduled_terminal].terminal_pcb;  //initialize to current running terminal's pcb

    //initalize pcb
    int i;
    for(i = 2; i < 8; i++) {
        if(pcb_ptr->fda[i].flags==1){   //close any file descriptors in use
            close(i);   
            pcb_ptr->fda[i].fops_table_ptr=bad_table;
        }    
    }

    // Mark PID as free
    processes[pcb_ptr->process_id] = 0;
    terminals[scheduled_terminal].last_assigned_pid = pcb_ptr->parent_process_id;
    
    // Check if we're at base shell and spawn new base shell if so
    if(pcb_ptr->parent_process_id == pcb_ptr->process_id){
        execute((uint8_t*)"shell");
    }

    // restore paging
    set_user_prog_page(pcb_ptr -> parent_process_id, 1);
    

    // Update TSS to return to parent context
    tss.esp0 = EIGHT_MB - (pcb_ptr -> parent_process_id * EIGHT_KB) - 4;    //setting ESP0 to base of new kernel stack
    tss.ss0 = KERNEL_DS;    //setting SS0 to kernel data segment

    // Check to see if parent called vidmap and turn it back on if so
    pcb_t *parent_pcb_ptr = pcb_ptr->parent_pcb;
    if(parent_pcb_ptr->called_vidmap)
        set_user_video_page(1);

    // Terminal's PCB var should track parent process
    terminals[scheduled_terminal].terminal_pcb = parent_pcb_ptr;

    // Check for exceptions and return 256 if so
    int32_t real_status;
    if(exception_flag) {
        exception_flag = 0;
        real_status = 256;
    }
    else
        real_status = status;

    // Jump back to execute so we can return
    asm volatile(
        "movl %0, %%esp;"
        "movl %1, %%ebp;"
        "xorl %%eax, %%eax;"
        "movl %2, %%eax;"
        "jmp EXECUTE_LABEL;"
        :       // Outputs
        : "r"(pcb_ptr->parent_esp), "r"(pcb_ptr->parent_ebp), "r"(real_status) // Inputs
        : "eax" // Clobbers
    );
    return -1;      // Should never reach here
}

/*
 * execute
 *    DESCRIPTION: Executes a program
 *    INPUTS: command -- the executable to run including its arguments
 *    OUTPUTS: none
 *    SIDE EFFECTS: Copies program to corresponding page and runs it
 *    RETURNS: Returns code given by program, or -1 if unsuccessful
 */
int32_t execute(const uint8_t* command){
    
    // Check null input 
    if(command == NULL){
        return -1;
    }

    // Find next available PID to assign
    int i, next_pid; 
    for(i = 0; i < MAX_PROCESSES; i++) {    //find next available process index
        // If we've found a free PID, mark it and break from loop
        if(processes[i] == 0) {
            next_pid = i;
            break;
        }
        // If we've reached end without finding a free PID, cannot execute
        if(i == MAX_PROCESSES - 1)
            return -1;
    }

    // Allocate PCB
    // Calculate pointer to next PCB
    pcb_t * next_pcb_ptr = (pcb_t *)(EIGHT_MB - ((next_pid + 1) * EIGHT_KB));

    // Initialize every fda entry and activate stdin and stdout
    for(i = 0; i < 2; i++) {
        next_pcb_ptr->fda[i].inode = 0;
        next_pcb_ptr->fda[i].file_pos = 0;
        next_pcb_ptr->fda[i].flags = 1;
    }
    for(i = 2; i < 8; i++) {
        next_pcb_ptr->fda[i].inode = 0;
        next_pcb_ptr->fda[i].file_pos = 0;
        next_pcb_ptr->fda[i].flags = 0;
    }

    // Set up fops tables for stdin and stdout respectively in the new pcb
    next_pcb_ptr->fda[0].fops_table_ptr = stdin_table;
    next_pcb_ptr->fda[1].fops_table_ptr = stdout_table;

    // Parse command
    uint32_t command_length = strlen((int8_t *)command) + 1;    // Adding 1 allows us to add a NULL terminator
    uint8_t exec_name[command_length];
    dentry_t file_dentry;

    // Find command from entry (IMPORTANT: Strip leading spaces?)
    i = 0;
    int j = 0;
    memset(exec_name, '\0', command_length);    // Zero out exec_name
    while(command[i] != NULL && i < command_length){
        // Strip spaces before cmd
        if(command[i] == ' ' && j == 0) {
            i++;
            continue;
        }
        // Jump out if we read a space
        if(command[i] == ' ' && j != 0)
            break;
        // Parse executable name
        exec_name[j] = command[i];
        i++;
        j++;
    }

    // Parse possible arguments (strips spaces between cmd and arg)
    j = 0;
    memset(next_pcb_ptr->arg, '\0', MAX_ARGS);  // Zero out PCB's args array
    if(command[i] != NULL){
        i++;
        while(command[i] != NULL && i < command_length && j<MAX_ARGS){
            // Strip spaces between cmd and arg
            if(command[i] == ' ' && j == 0){
                i++;
                continue;
            }
            next_pcb_ptr->arg[j] = command[i];
            i++;
            j++;
        }
    }

    // Find file and do executable check
    //check whether file exists within directory
    int dentry_res = read_dentry_by_name(exec_name, &file_dentry);  
    if(dentry_res == -1){       
        return -1;
    }

    // Copy program file to allocated page
    // Allocate Page and flush TLB
    set_user_prog_page(next_pid, 1); // Set present bit in execute and 0 in halt
    
    // Load executable into user page
    int val = read_data(file_dentry.inode, 0, (uint8_t*)PROG_IMG_ADDR, 100000);
    if(val == -1){
        set_user_prog_page(next_pid, 0);
        set_user_prog_page(terminals[scheduled_terminal].last_assigned_pid, 1);
        return -1;
    }

    // Check ELF constant to see if file is an executable
    uint8_t elf_check[4];
    read_data(file_dentry.inode, 0, elf_check, 4);
    if(elf_check[0] != 0x7f){
        set_user_prog_page(next_pid, 0);
        set_user_prog_page(terminals[scheduled_terminal].last_assigned_pid, 1);
        return -1;
    }
    if(elf_check[1] != 0x45){
        set_user_prog_page(next_pid, 0);
        set_user_prog_page(terminals[scheduled_terminal].last_assigned_pid, 1);
        return -1;
    }
    if(elf_check[2] != 0x4c){
        set_user_prog_page(next_pid, 0);
        set_user_prog_page(terminals[scheduled_terminal].last_assigned_pid, 1);
        return -1;
    }
    if(elf_check[3] != 0x46){
        set_user_prog_page(next_pid, 0);
        set_user_prog_page(terminals[scheduled_terminal].last_assigned_pid, 1);
        return -1;
    }

    if(next_pid <= 2){    // base shell of terminal: assign given pid as both parent and process to denote base shell 
        next_pcb_ptr->parent_process_id = next_pid;
        next_pcb_ptr->process_id = next_pid;
    }
    else{
        next_pcb_ptr->parent_process_id = terminals[scheduled_terminal].last_assigned_pid;
        next_pcb_ptr->process_id = next_pid;
    }

    // Mark PID as in use and set PCB
    processes[next_pid] = 1;
    terminals[scheduled_terminal].last_assigned_pid = next_pid;

    
    // Initialize vidmap flag
    next_pcb_ptr->called_vidmap = 0;
    
    // Get addr exec's first instruction (bytes 24-27 of the exec file)
    uint8_t prog_entry_buf[4];
    uint32_t prog_entry_addr;
    read_data(file_dentry.inode, 24, prog_entry_buf, 4);
    prog_entry_addr = *((uint32_t*)prog_entry_buf);

    // Prepare TSS for context switch
    tss.esp0 = EIGHT_MB - (next_pid * EIGHT_KB) - 4;    //setting ESP0 to base of new kernel stack
    tss.ss0 = KERNEL_DS;    //setting SS0 to kernel data segment
    
    // If we're executing the first base shells for the terminals, the scheduler already got their ESP/EBP
    if(next_pid <= 2){
        next_pcb_ptr->curr_esp = terminals[scheduled_terminal].terminal_pcb->curr_esp;
        next_pcb_ptr->curr_ebp = terminals[scheduled_terminal].terminal_pcb->curr_ebp;
    }
    // Otherwise, the scheduler will put the exec's ESP/EBP into the PCB later
    else{
        next_pcb_ptr->curr_esp = NULL;
        next_pcb_ptr->curr_ebp = NULL;
    }
    
    // Save state of current/parent stack into PCB
    asm volatile ("movl %%esp, %0;"
                  "movl %%ebp, %1;"
                : "=r" (next_pcb_ptr->parent_esp), "=r" (next_pcb_ptr->parent_ebp)    // Outputs
    );

    next_pcb_ptr->parent_pcb = terminals[scheduled_terminal].terminal_pcb; // Save existing PCB as parent
    terminals[scheduled_terminal].terminal_pcb = next_pcb_ptr; // Update pcb pointer for current terminal
    
    // Push items to stack and context switch using IRET
    asm volatile (
        
        "movl %1, %%ds;"
        
        "pushl %1;"                 //push USER_DS, 0x2B
        
        "pushl $0x083ffffc;"        // Set ESP to point to the user page (132MB - 4B)
        
        "pushfl;"                   //push flags
        "popl %%eax;"
        "orl $0x200, %%eax;"        //sets bit 9 to 1 in the flags register to sti
        "pushl %%eax;"

        "pushl %2;"                 //push USER_CS, 0x23
        
        "pushl %0;"                 // Push the addr of exec's first instruction for EIP

        "iret;"

        "EXECUTE_LABEL: "
        :                       // No Outputs
        : "r"(prog_entry_addr), "r"(USER_DS), "r"(USER_CS)      // Inputs
        : "eax"                     // Clobbers
    );

    // Maintain program's return value
    register int32_t retval asm("eax");
    return retval;
}


/*
 * read
 *    DESCRIPTION: Calls the correspoding read() function
 *    INPUTS: fd -- file dsecriptor
 *            buf -- input buffer
 *            nbytes -- number of bytes to write to buf
 *    OUTPUTS: none
 *    RETURNS: The return value of the desired read() function
 */
int32_t read(int32_t fd, void* buf, int32_t nbytes){
    sti();
    pcb_t *pcb = (pcb_t*)(tss.esp0  & 0xFFFFE000); //ANDing the process's ESP register w/ appropriate bit mask to reach top of stack
    if(fd<0 || fd>7 || pcb->fda[fd].flags == 0) //check for valid fd index, max 8 files
        return -1;

    if(buf==NULL)
        return -1;
    
    //question is how do i initialize the pcb??

    return pcb->fda[fd].fops_table_ptr.read(fd, buf, nbytes);
}

/*
 * write
 *    DESCRIPTION: Calls the correspoding write() function
 *    INPUTS: fd -- file dsecriptor
 *            buf -- input buffer
 *            nbytes -- number of bytes to write to buf
 *    OUTPUTS: none
 *    RETURNS: The return value of the desired write() function
 */
int32_t write(int32_t fd, const void* buf, int32_t nbytes){
    
    pcb_t *pcb=(pcb_t*)(tss.esp0  & 0xFFFFE000);    //temp placeholder until we figure out how to initialize the pcb
    if(fd<0 || fd>7 || pcb->fda[fd].flags == 0) //check for valid fd index, max 8 files
        return -1;

    if(buf==NULL)
        return -1;
    
    return pcb->fda[fd].fops_table_ptr.write(fd, buf, nbytes);
}

/*
 * open
 *    DESCRIPTION: Opens the desired file
 *    INPUTS: filename -- name of the file to open
 *    OUTPUTS: none
 *    RETURNS: The file descriptor the opened file was assigned to, or -1 if unsuccessful
 */
int32_t open(const uint8_t* filename){
    
    pcb_t *pcb=(pcb_t*)(tss.esp0  & 0xFFFFE000);    //temp placeholder until we figure out how to initialize the pcb
    if(filename==NULL)  //check for valid file
        return -1;

    dentry_t dentry;
    if(read_dentry_by_name(filename,&dentry)==-1)   //check if file exists within dentry
        return -1;
    

    /*find an available space in file descriptor array, 
    * 0 and 1 indices are reserved for stdin and stdout
    * max 8 open files at a time
    */
    uint32_t i;
    uint32_t available=0;       //flags whether available space is found
    /* IMPORTANT */ // Ask if we need to include stdin and stdout below
    for(i=2; i<8; i++){         
        if(pcb->fda[i].flags==0){   //if available space is found, initialize file descriptor
            pcb->fda[i].file_pos=0; //should always start at beginning of file
            pcb->fda[i].flags=1;    //mark space as used
            available=1;            
            break;
        }
    }

    if(available==0)             //if no available space is found, fail
        return -1;
    
    uint32_t file_type = dentry.ftype;
    if(file_type==0){   //ftype 0 for RTC
        pcb->fda[i].fops_table_ptr=rtc_table;
        (void)RTC_open((uint8_t *)"rtc");
    }
    else if(file_type==1){   //ftype 1 for directory (don't need to call open_dir as it's successful at this point)
        pcb->fda[i].fops_table_ptr=directory_table;
    }
    else if(file_type==2){   //ftype 2 for regular file (don't need to call open_file as it's successful at this point)
        pcb->fda[i].fops_table_ptr=file_table;
        pcb->fda[i].inode=dentry.inode;
    }

    return i;                   //return index of file assigned in file descriptor array
}

/*
 * close
 *    DESCRIPTION: Calls the corresponding close function
 *    INPUTS: fd -- the file descriptor 
 *    OUTPUTS: none
 *    RETURNS: The return value of the desired close function
 */
int32_t close(int32_t fd){
    
    pcb_t* pcb=(pcb_t*)(tss.esp0  & 0xFFFFE000); //temp placeholder until we figure out how to initialize the pcb
    
    if(fd<2 || fd>7 || pcb->fda[fd].flags == 0) //check for valid fd index, max 8 files
        return -1;
    
    pcb->fda[fd].flags=0; //mark index in file descriptor array as available
    
    return pcb->fda[fd].fops_table_ptr.close(fd);
}

/*
 * getargs
 *    DESCRIPTION: Puts the arguments of the last shell command to the output buffer
 *    INPUTS: buf -- output buffer in user level
 *            nbytes -- number of bytes to write
 *    OUTPUTS: writes the arguments of the last shell command to buf
 *    RETURNS: 0 on success, -1 on fail
 */
int32_t getargs(uint8_t * buf, int32_t nbytes) {
    
    pcb_t *pcb=(pcb_t*)(tss.esp0  & 0xFFFFE000);

    // Check for valid buf and bytes to be read, or no args were passed
    if(buf==NULL || nbytes < MAX_ARGS || pcb->arg[0] == '\0')
        return -1;

    memcpy((void*) buf, (const void*) pcb->arg, nbytes); //copy argument into buffer

    return 0;
}

/*
 * vidmap
 *    DESCRIPTION: Creates a video memory page for the user at the given address
 *    INPUTS: screen_start -- pointer to user level
 *    OUTPUTS: Writes the VirtMem address that we map the user's video page to
 *                into ptr pointed by screen_start (should always be 256MB as that's our pre-set addr)
 *    RETURNS: 0 on success, -1 on fail
 */
int32_t vidmap(uint8_t ** screen_start) {

    // Verify that screen_start is within the user-level page (128MB-132MB)
    if((uint32_t)screen_start > ONE_THREE_TWO_MB || (uint32_t)screen_start < ONE_TWO_EIGHT_MB) {
        return -1;
    }
    
    // Mark that the process called vidmap
    pcb_t *pcb=(pcb_t*)(tss.esp0  & 0xFFFFE000);
    pcb->called_vidmap = 1;

    // Set the page entry and copy the VirtMem address to user space
    set_user_video_page(1);
    *screen_start = (uint8_t*)TWO_FIVE_SIX_MB;
    return 0;
}

/*
 * set_handler
 *    DESCRIPTION: Does nothing for now
 */
int32_t set_handler(int32_t signum, void* handler_address) {
    return -1;
}

/*
 * getargs
 *    DESCRIPTION: Does nothing for now
 */
int32_t sigreturn(void) {
    return -1;
}
