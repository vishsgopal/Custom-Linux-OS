#ifndef _SYSTEM_CALLS_H
#define _SYSTEM_CALLS_H

#include "types.h"

#define MAX_PROCESSES 6
#define MAX_ARGS 100

//Appendix A 8.2, fops table should contain entries for open, read, write, and close
//Note: functions are casted to pointers, otherwise C won't recognize them in struct
typedef struct{
    int32_t (*read)(int32_t fd, void* buf, int32_t nbytes);
    int32_t (*write)(int32_t fd, const void* buf, int32_t nbytes);
    int32_t (*open)(const uint8_t* filename);
    int32_t (*close)(int32_t fd);
} fops_jump_table_t;


// File descriptor struct for entries in the file descriptor array (FDA)
typedef struct file_descriptor_t {
    fops_jump_table_t fops_table_ptr;      
    uint32_t inode;
    uint32_t file_pos; 
    uint32_t flags;     //marks whether is in use
} file_descriptor_t;

//Process control block (PCB) struct described in Appendix A 8.2
typedef struct pcb {
    file_descriptor_t fda[8];    //up to 8 open files are represented with a file array in PCB
    uint32_t process_id;
    uint32_t parent_process_id;
    uint32_t parent_esp;        // Used to restore parent's ESP when process halts
    uint32_t parent_ebp;        // Used to restore parent's EBP when process halts
    uint32_t curr_esp;          // ESP for this process so its not corrupted upon scheduler task switching
    uint32_t curr_ebp;          // EBP for this process so its not corrupted upon scheduler task switching
    uint8_t called_vidmap;
    int8_t arg[MAX_ARGS];             // holds the arguments passed by the shell cmd 
    struct pcb * parent_pcb;
}pcb_t;



//static uint32_t last_assigned_pid;      //keeps track of current pid

int32_t bad_call();

void bootup_terminals();

/*required functions for CP3/CP4, function formats in Appendix B*/
int32_t halt(uint8_t status);

int32_t execute(const uint8_t* command);

int32_t read(int32_t fd, void* buf, int32_t nbytes);

int32_t write(int32_t fd, const void* buf, int32_t nbytes);

int32_t open(const uint8_t* filename);

int32_t close(int32_t fd);

int32_t getargs(uint8_t * buf, int32_t nbytes);

int32_t vidmap(uint8_t ** screen_start);

int32_t set_handler(int32_t signum, void* handler_address);

int32_t sigreturn(void);

#endif /* _SYSTEM_CALLS_H */
