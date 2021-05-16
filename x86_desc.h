/* x86_desc.h - Defines for various x86 descriptors, descriptor tables,
 * and selectors
 * vim:ts=4 noexpandtab
 */

#ifndef _X86_DESC_H
#define _X86_DESC_H

#include "types.h"

/* Segment selector values */
#define KERNEL_CS   0x0010
#define KERNEL_DS   0x0018
#define USER_CS     0x0023
#define USER_DS     0x002B
#define KERNEL_TSS  0x0030
#define KERNEL_LDT  0x0038

/* Size of the task state segment (TSS) */
#define TSS_SIZE    104

/* Number of vectors in the interrupt descriptor table (IDT) */
#define NUM_VEC     256

// (MP3.1) Define macros for byte sizes
#define ONE_KB 1024             // 1024 bytes make a KB
#define FOUR_KB 4096
#define EIGHT_KB 8192
#define FOUR_MB 0x400000        // Coincidentally, ONE_KB * ONE_KB * 4 == 0x400000, also the kernel is loaded here
#define EIGHT_MB 0x800000
#define ONE_TWO_EIGHT_MB 0x08000000
#define ONE_THREE_TWO_MB 0x08400000
#define TWO_FIVE_SIX_MB 0x10000000
#define PROG_IMG_ADDR 0x08048000
#define VIDMEM 0xB8000          // Start of video memory

#ifndef ASM

/* This structure is used to load descriptor base registers
 * like the GDTR and IDTR */
typedef struct x86_desc {
    uint16_t padding;
    uint16_t size;
    uint32_t addr;
} x86_desc_t;

/* This is a segment descriptor.  It goes in the GDT. */
typedef struct seg_desc {
    union {
        uint32_t val[2];
        struct {
            uint16_t seg_lim_15_00;
            uint16_t base_15_00;
            uint8_t  base_23_16;
            uint32_t type          : 4;
            uint32_t sys           : 1;
            uint32_t dpl           : 2;
            uint32_t present       : 1;
            uint32_t seg_lim_19_16 : 4;
            uint32_t avail         : 1;
            uint32_t reserved      : 1;
            uint32_t opsize        : 1;
            uint32_t granularity   : 1;
            uint8_t  base_31_24;
        } __attribute__ ((packed));
    };
} seg_desc_t;

// (MP3.1) Struct used for 4kb page directory entries (32 bits long)
typedef struct page_dir_4kb_t {
    union {
        uint32_t val;
        struct {
            uint32_t present                :1;
            uint32_t read_write             :1;
            uint32_t user_supervisor        :1;
            uint32_t page_write_through     :1;
            uint32_t page_cache_disabled    :1;
            uint32_t accessed               :1;
            uint32_t reserved               :1;
            uint32_t page_size              :1;
            uint32_t global_bit             :1;
            uint32_t available              :3;        
            uint32_t page_table_addr        :20;
        } __attribute__ ((packed));
    };
} page_dir_4kb_t;

// (MP3.1) Struct used for 4mb page directory entries (32 bits long)
typedef struct page_dir_4mb_t {
    union {
        uint32_t val;
        struct {
            uint32_t present                :1;
            uint32_t read_write             :1;
            uint32_t user_supervisor        :1;
            uint32_t page_write_through     :1;
            uint32_t page_cache_disabled    :1;
            uint32_t accessed               :1;
            uint32_t dirty                  :1;
            uint32_t page_size              :1;
            uint32_t global_bit             :1;
            uint32_t available              :3;     
            uint32_t page_attr_index        :1;  
            uint32_t reserved               :9; 
            uint32_t base_addr              :10;
        } __attribute__ ((packed));
    };
} page_dir_4mb_t;

// (MP3.1) Typedef union so that we can have both 4KB and 4MB page dir entries
typedef union page_dir_desc_t {
    page_dir_4kb_t pd_kb;
    page_dir_4mb_t pd_mb;
} page_dir_desc_t;

// (MP3.1) Struct used for page table entries (32 bits long)
typedef struct page_tab_desc_t {
    union {
        uint32_t val;
        struct {
            uint16_t present                 : 1;
            uint16_t read_write              : 1;
            uint8_t  user_supervisor         : 1;
            uint32_t page_write_through      : 1;
            uint32_t page_cache_disabled     : 1;
            uint32_t accessed                : 1;
            uint32_t dirty                   : 1;
            uint32_t page_attr_tab_index     : 1;
            uint32_t global_bit              : 1;
            uint32_t avail                   : 3;
            uint32_t page_base_address       : 20;
        } __attribute__ ((packed));
    };
} page_tab_desc_t;



/* TSS structure */
typedef struct __attribute__((packed)) tss_t {
    uint16_t prev_task_link;
    uint16_t prev_task_link_pad;

    uint32_t esp0;
    uint16_t ss0;
    uint16_t ss0_pad;

    uint32_t esp1;
    uint16_t ss1;
    uint16_t ss1_pad;

    uint32_t esp2;
    uint16_t ss2;
    uint16_t ss2_pad;

    uint32_t cr3;

    uint32_t eip;
    uint32_t eflags;

    uint32_t eax;
    uint32_t ecx;
    uint32_t edx;
    uint32_t ebx;
    uint32_t esp;
    uint32_t ebp;
    uint32_t esi;
    uint32_t edi;

    uint16_t es;
    uint16_t es_pad;

    uint16_t cs;
    uint16_t cs_pad;

    uint16_t ss;
    uint16_t ss_pad;

    uint16_t ds;
    uint16_t ds_pad;

    uint16_t fs;
    uint16_t fs_pad;

    uint16_t gs;
    uint16_t gs_pad;

    uint16_t ldt_segment_selector;
    uint16_t ldt_pad;

    uint16_t debug_trap : 1;
    uint16_t io_pad     : 15;
    uint16_t io_base_addr;
} tss_t;

/* Some external descriptors declared in .S files */
extern x86_desc_t gdt_desc;

extern uint16_t ldt_desc;
extern uint32_t ldt_size;
extern seg_desc_t ldt_desc_ptr;
extern seg_desc_t gdt_ptr;
extern uint32_t ldt;

extern uint32_t tss_size;
extern seg_desc_t tss_desc_ptr;
extern tss_t tss;

/* Sets runtime-settable parameters in the GDT entry for the LDT */
#define SET_LDT_PARAMS(str, addr, lim)                          \
do {                                                            \
    str.base_31_24 = ((uint32_t)(addr) & 0xFF000000) >> 24;     \
    str.base_23_16 = ((uint32_t)(addr) & 0x00FF0000) >> 16;     \
    str.base_15_00 = (uint32_t)(addr) & 0x0000FFFF;             \
    str.seg_lim_19_16 = ((lim) & 0x000F0000) >> 16;             \
    str.seg_lim_15_00 = (lim) & 0x0000FFFF;                     \
} while (0)

/* Sets runtime parameters for the TSS */
#define SET_TSS_PARAMS(str, addr, lim)                          \
do {                                                            \
    str.base_31_24 = ((uint32_t)(addr) & 0xFF000000) >> 24;     \
    str.base_23_16 = ((uint32_t)(addr) & 0x00FF0000) >> 16;     \
    str.base_15_00 = (uint32_t)(addr) & 0x0000FFFF;             \
    str.seg_lim_19_16 = ((lim) & 0x000F0000) >> 16;             \
    str.seg_lim_15_00 = (lim) & 0x0000FFFF;                     \
} while (0)

/* IMPORTANT */
// Colon denotes number of bits that the value is
// Find every exception and interrupt in ISA doc table
// The val array doesn't need to be used if u init using the struct
/* An interrupt descriptor entry (goes into the IDT) */
typedef union idt_desc_t {
    uint32_t val[2];
    struct {
        uint16_t offset_15_00;
        uint16_t seg_selector;
        uint8_t  reserved4;
        uint32_t reserved3 : 1;
        uint32_t reserved2 : 1;
        uint32_t reserved1 : 1;
        uint32_t size      : 1;
        uint32_t reserved0 : 1;
        uint32_t dpl       : 2;
        uint32_t present   : 1;
        uint16_t offset_31_16;
    } __attribute__ ((packed)); // Already aligned
} idt_desc_t;

/* IMPORTANT */
// Put the instances of the above union into this array.
/* The IDT itself (declared in x86_desc.S */
extern idt_desc_t idt[NUM_VEC];
/* The descriptor used to load the IDTR */
extern x86_desc_t idt_desc_ptr;

/* Sets runtime parameters for an IDT entry */
// str is pointer to element in interrupt table and handler is func ptr
// Just call macro after making interrupt handler function
#define SET_IDT_ENTRY(str, handler)                              \
do {                                                             \
    str.offset_31_16 = ((uint32_t)(handler) & 0xFFFF0000) >> 16; \
    str.offset_15_00 = ((uint32_t)(handler) & 0xFFFF);           \
} while (0)

/* Load task register.  This macro takes a 16-bit index into the GDT,
 * which points to the TSS entry.  x86 then reads the GDT's TSS
 * descriptor and loads the base address specified in that descriptor
 * into the task register */
#define ltr(desc)                       \
do {                                    \
    asm volatile ("ltr %w0"             \
            :                           \
            : "r" (desc)                \
            : "memory", "cc"            \
    );                                  \
} while (0)

/* Load the interrupt descriptor table (IDT).  This macro takes a 32-bit
 * address which points to a 6-byte structure.  The 6-byte structure
 * (defined as "struct x86_desc" above) contains a 2-byte size field
 * specifying the size of the IDT, and a 4-byte address field specifying
 * the base address of the IDT. */
#define lidt(desc)                      \
do {                                    \
    asm volatile ("lidt (%0)"           \
            :                           \
            : "g" (desc)                \
            : "memory"                  \
    );                                  \
} while (0)

/* Load the local descriptor table (LDT) register.  This macro takes a
 * 16-bit index into the GDT, which points to the LDT entry.  x86 then
 * reads the GDT's LDT descriptor and loads the base address specified
 * in that descriptor into the LDT register */
#define lldt(desc)                      \
do {                                    \
    asm volatile ("lldt %%ax"           \
            :                           \
            : "a" (desc)                \
            : "memory"                  \
    );                                  \
} while (0)

#endif /* ASM */

#endif /* _x86_DESC_H */
