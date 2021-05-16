#ifndef _FILE_SYSTEM_H
#define _FILE_SYSTEM_H

#include "types.h"

#define BLOCK_SIZE 4096         //file system memory is divided into 4KB blocks
#define FNAME_LENGTH  32        //file name limit is 32 characters 
#define MAX_DENTRY 64

typedef struct{ 
    uint8_t block[BLOCK_SIZE];    
}data_block_t;

typedef struct{
    uint32_t file_size;         //described in bytes, used for regular files
    uint32_t index_num[1023];   //holds max data block index (1KB)
}inode_t;

typedef struct{
    uint8_t fname[FNAME_LENGTH]; 
    uint32_t ftype;
    uint32_t inode;
    uint8_t reserved[24]; //24B reserved in dir entries, Appendix A
}dentry_t;

/*first block in file system memory*/
typedef struct{
    uint32_t num_dentries;
    uint32_t num_inodes;
    uint32_t num_data_blocks;
    uint8_t reserved[52];   //52B reserved in boot block, Appendix A
    dentry_t dentries[63]; //64B dir entries in boot block, Appendix A
}boot_block_t;


/*global variables that will keep track of entire file system structure*/
data_block_t* fs_data_block;
inode_t* fs_inode;
boot_block_t* boot;       
dentry_t* fs_dentry;       

//initializes filesystem based off start pointer
extern void init_filesystem(uint32_t start);

/*these file system functions are specified in Appendix A*/
extern int32_t read_dentry_by_name(const uint8_t* fname, dentry_t* dentry);
extern int32_t read_dentry_by_index(uint32_t index, dentry_t* dentry);
extern int32_t read_data (uint32_t inode, uint32_t offset, uint8_t* buf, uint32_t length);

/*required system call functions from Appendix B for files and directories*/
// extern int32_t read_file(int32_t fd, void* buf, int32_t nbytes);         //format cant be used for cp2 bc of fd
extern int32_t read_file(int32_t fd, void* buf, int32_t nbytes);
extern int32_t write_file(int32_t fd, const void* buf, int32_t nbytes);
extern int32_t open_file(const uint8_t* filename);
extern int32_t close_file (int32_t fd);

// extern int32_t read_dir(int32_t fd, void* buf, int32_t nbytes);          //format cant be used for cp2 bc of fd
extern int32_t read_dir(int32_t fd, void* buf, int32_t nbytes);
extern int32_t write_dir(int32_t fd, const void* buf, int32_t nbytes);
extern int32_t open_dir(const uint8_t* dirname);
extern int32_t close_dir (int32_t fd);

#endif /* _FILE_SYSTEM_H */
