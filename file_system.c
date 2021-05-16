#include "file_system.h"
#include "lib.h"
#include "system_calls.h"
#include "x86_desc.h"

/*  
 * init_filesystem
 *    DESCRIPTION: Initializes the file system structure based off the given start pointer
 *    INPUTS: pointer to the start of the file system
 *    OUTPUTS: NONE
 *    SIDE EFFECTS: Boot, inode, dentry, and data block global variables are initialized
 *    NOTES: See Appendix A
 */ 
void init_filesystem(uint32_t start){
    boot=(boot_block_t*)start;      //points to starting memory block of file system 
    fs_inode=(inode_t*)(start+BLOCK_SIZE);   //inodes start one block (4KB) after start/boot
    fs_dentry=(dentry_t*)(start+64);   //dir entries start 64B after start/boot 
    fs_data_block=(data_block_t*)(start+BLOCK_SIZE*(boot->num_inodes+1)); //data block starts a block after inode
}

/*  
 * read_dentry_by_name
 *    DESCRIPTION: Finds and copies over dentry info into a dentry block based on file name 
 *    INPUTS: file name (to find), dentry (to copy over to)
 *    OUTPUTS: 0 for success, -1 for fail
 *    SIDE EFFECTS: Dentry block is initialized with info upon success
 *    NOTES: See Appendix A
 */ 
int32_t read_dentry_by_name(const uint8_t* fname, dentry_t* dentry){
    if(fname==NULL||dentry==NULL||strlen((int8_t*)fname) > 32)   //check for invalid pointers
        return -1;

    int i;
    for(i=0;i<boot->num_dentries;i++){      //loop through all dentries
        /*if names match, copy over dentry file name, type, and index node into dentry block*/
        if(!strncmp((int8_t*)&(boot->dentries[i]),(int8_t*)fname,FNAME_LENGTH)){   
            // if(strlen((int8_t*)fname) <= 32){
                strncpy((int8_t*)dentry->fname, (int8_t*)boot->dentries[i].fname,FNAME_LENGTH);
                dentry->ftype=boot->dentries[i].ftype;
                dentry->inode=boot->dentries[i].inode;
                return 0;   //successfully copied over, return 0
            // }  
            
        }
    }
    return -1;  //dentry not found, return -1 
}

/*  
 * read_dentry_by_index
 *    DESCRIPTION: Finds and copies over dentry info into a dentry block based on given index
 *    INPUTS: index (to find), dentry (to copy over to)
 *    OUTPUTS: 0 for success, -1 for fail
 *    SIDE EFFECTS: Dentry block is initialized with info upon success
 *    NOTES: See Appendix A
 */ 
int32_t read_dentry_by_index(uint32_t index, dentry_t* dentry){
    if(dentry==NULL)   //check for invalid pointer
        return -1;

    if(index >= boot->num_dentries)    //check if index is out of bounds
        return -1;

    /*index is valid, so copy over dentry file name, type, and index node into dentry block*/
    strncpy((int8_t*)dentry->fname, (int8_t*)boot->dentries[index].fname,FNAME_LENGTH);
    dentry->ftype=boot->dentries[index].ftype;
    dentry->inode=boot->dentries[index].inode;
    
    return 0;   //successfully copied over, return 0
}

/*  
 * read_data
 *    DESCRIPTION: Finds and copies over dentry info into a dentry block based on given index
 *    INPUTS: inode -- file inode to read
 *            offset -- offset to start reading from
 *            buf -- output buffer to write file data to
 *            nbytes -- number of bytes to read from file
 *    OUTPUTS: number of bytes read
 *    SIDE EFFECTS: buf holds file data
 *    NOTES: See Appendix A
 */ 
int32_t read_data (uint32_t inode, uint32_t offset, uint8_t* buf, uint32_t length){
    if(buf==NULL)           //check for invalid pointer
        return 0;

    if(inode >= boot->num_inodes)      //check if index node is out of bounds
        return 0;
    
    inode_t* curr_inode=(inode_t*)(&(fs_inode[inode])); //points to the file with inode number inode

    if(offset >= curr_inode->file_size) //check if offset from start of file is out of bounds
        return 0;

    int bytes_read;
    data_block_t* curr_data_block;

    for(bytes_read=0; bytes_read<length; bytes_read++){     //loop through bytes that need to be read
        if((offset) > curr_inode->file_size)     //break if bytes read go out of file bounds
            break;
        curr_data_block=(data_block_t*)(fs_data_block + (curr_inode->index_num[offset/BLOCK_SIZE])); //finds start of data block of which to read bytes from
        buf[bytes_read]=curr_data_block->block[offset%BLOCK_SIZE];  //copy data into buf
        offset++;
    }
    return bytes_read;
}


/*  
 * read_file
 *    DESCRIPTION: Finds and copies over dentry info into a dentry block based on given index
 *    INPUTS: inode -- file inode to read
 *            offset -- offset to start reading from
 *            buf -- output buffer to write file data to
 *            nbytes -- number of bytes to read from file
 *    OUTPUTS: number of bytes read
 *    SIDE EFFECTS: buf holds file data
 *    NOTES: See Appendix A
 */
int32_t read_file(int32_t fd, void* buf, int32_t nbytes){
    uint32_t inode, offset;
    pcb_t* pcb=(pcb_t*)(tss.esp0  & 0xFFFFE000); //place holder until we figure out how to initialize pcb
    
    offset=pcb->fda[fd].file_pos;
    inode=pcb->fda[fd].inode;


    pcb->fda[fd].file_pos+=nbytes;  //update file position for next file
    return read_data(inode, offset, (uint8_t*)buf, nbytes);
}

/*  
 * write_file
 *    DESCRIPTION: Does nothing due to read-only file system
 *    INPUTS: none
 *    OUTPUTS: Always -1
 *    SIDE EFFECTS: none
 *    NOTES: See Appendix A
 */
int32_t write_file(int32_t fd, const void* buf, int32_t nbytes){
    return -1;          //file system is read-only so writing always fails
}

/*  
 * open_file
 *    DESCRIPTION: Does nothing for now
 *    INPUTS: filename -- pointer to file
 *    OUTPUTS: Always 0
 *    SIDE EFFECTS: none
 *    NOTES: See Appendix A
 */
int32_t open_file(const uint8_t* filename){
    return 0;           //always success, no need to check for name validity
}

/*  
 * close_file
 *    DESCRIPTION: Does nothing for now
 *    INPUTS: fd -- file descriptor
 *    OUTPUTS: Always 0
 *    SIDE EFFECTS: none
 *    NOTES: See Appendix A
 */
int32_t close_file (int32_t fd){
    return 0;           //closing is always successful
}

/*  
 * read_dir
 *    DESCRIPTION: Reads all file names into the buf
 *    INPUTS: fd -- file descriptor
 *            buf -- ptr to buffer to write to
 *            nbytes -- number of bytes to read
 *    OUTPUTS: returns number of bytes read
 *    SIDE EFFECTS: writes file names to buf
 *    NOTES: See Appendix A
 */
int32_t read_dir(int32_t fd, void* buf, int32_t nbytes){
    pcb_t* pcb=(pcb_t*)(tss.esp0  & 0xFFFFE000); //place holder until we figure out how to initialize pcb
    
    dentry_t dentry;
    uint32_t position;
    int32_t valid;
    
    position=pcb->fda[fd].file_pos;                     //initializes file position
    valid = read_dentry_by_index(position, &dentry);   //checks whether copying over is valid
    
    if(buf==NULL || position>=MAX_DENTRY || valid == -1)       //check for null pointer
        return 0;
    
    position+=1;
    pcb->fda[fd].file_pos=position;             //update next position in file descriptor array
    memcpy(buf, (const void*)dentry.fname, FNAME_LENGTH);   //copies over file name into buf
    return nbytes;
    
}

/*  
 * write_dir
 *    DESCRIPTION: Does nothing for now
 *    INPUTS: none
 *    OUTPUTS: Always -1
 *    SIDE EFFECTS: none
 *    NOTES: See Appendix A
 */
int32_t write_dir(int32_t fd, const void* buf, int32_t nbytes){
    return -1;          //file system is read-only so writing always fails
}

/*  
 * open_dir
 *    DESCRIPTION: Does nothing for now
 *    INPUTS: dirname -- pointer to directory
 *    OUTPUTS: Always 0
 *    SIDE EFFECTS: none
 *    NOTES: See Appendix A
 */
int32_t open_dir(const uint8_t* dirname){
    return 0;           //always success, no need to check for name validity
}

/*  
 * close_dir
 *    DESCRIPTION: Does nothing for now
 *    INPUTS: fd -- file descriptor
 *    OUTPUTS: Always 0
 *    SIDE EFFECTS: none
 *    NOTES: See Appendix A
 */
int32_t close_dir (int32_t fd){
    return 0;           //closing is always successful
}
