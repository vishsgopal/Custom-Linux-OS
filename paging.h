/* paging.h (MP3.1): Header file for paging.c
 * vim:ts=4 noexpandtab
 */

#ifndef _PAGING_H
#define _PAGING_H

#include "x86_desc.h"

/* This is the page directory index of 128MB, which is where we'll put the 4MB user page
   Each page directory entry corresponds to 4MB of VirtMem, so 128 / 4 = 32 */
#define USER_PAGE_BASE_ADDR 32

/* This is the page directory index of 256MB, which is where we'll put the 4KB user video page
   Each page directory entry corresponds to 4MB of VirtMem, so 256 / 4 = 64 */
#define USER_VID_PAGE_DIR_I 64

// Page base address for video memory (0xB8000 >> 12)
#define VIDMEM_PAGE_BASE 0xB8

// (MP3.1) Page directory
page_dir_desc_t page_directory[1024] __attribute__((aligned (FOUR_KB)));
// (MP3.1) Page table
page_tab_desc_t page_table_one[1024] __attribute__((aligned (FOUR_KB)));
// (MP3.4) Page table for user video memory
page_tab_desc_t user_video_table[1024] __attribute__((aligned (FOUR_KB)));

// Function to initialize paging
extern void init_paging(void);

// Helper function to set up user page
extern void set_user_prog_page(uint32_t pid, int32_t present_flag);

// Helper function to set up user video memory page
extern void set_user_video_page(int32_t present_flag);

// Helper function to save and copy video memory for visible terminal switching
void change_terminal_video_page(int32_t from_terminal_id, int32_t to_terminal_id);

// Helper function to remap where virtual video memory maps to physically
void redirect_vidmem_page(int32_t terminal_id); 

// Function to flush TLB
extern void flush_tlb(void);

#endif /* _PAGING_H */
