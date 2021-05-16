#include "tests.h"
#include "x86_desc.h"
#include "lib.h"
#include "rtc.h"
#include "file_system.h"
#include "terminal.h"
#include "paging.h"
#include "system_calls.h"

#define PASS 1
#define FAIL 0

/* format these macros as you see fit */
#define TEST_HEADER 	\
	printf("[TEST %s] Running %s at %s:%d\n", __FUNCTION__, __FUNCTION__, __FILE__, __LINE__)
#define TEST_OUTPUT(name, result)	\
	printf("[TEST %s] Result = %s\n", name, (result) ? "PASS" : "FAIL")


static inline void assertion_failure(){
	/* Use exception #15 for assertions, otherwise
	   reserved by Intel */
	asm volatile("int $15");
}


/* Checkpoint 1 (MP3.1) tests */

/* IDT Test - Example
 * 
 * Asserts that first 10 IDT entries are not NULL
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: None
 * Coverage: Load IDT, IDT definition
 * Files: x86_desc.h/S
 */
int idt_test(){
	TEST_HEADER;

	int i;
	int result = PASS;
	for (i = 0; i < 10; ++i){
		if ((idt[i].offset_15_00 == NULL) && 
			(idt[i].offset_31_16 == NULL)){
			assertion_failure();
			result = FAIL;
		}
	}
	return result;
}

/* MP3.1: Test cases for exceptions */

/*
 * test_divzero_exception (MP3.1)
 *    DESCRIPTION: Tests divide by 0 exception (INT 0)
 *    INPUTS: none
 *    OUTPUTS: none
 *    RETURN VALUES: none
 *    SIDE EFFECTS: Should force an infinite loop because our exception handlers just infinite loop
 */
int test_divzero_exception() {
 	TEST_HEADER;
	
 	int i, j;
	j = 1 - 1; 			// Dodge divide by zero warning
 	i = 1 / j;

 	return FAIL;		// If exception wasn't thrown and we aren't looping, we failed (at least for MP3.1)
}


/*
 * test_opcode_exception (MP3.1)
 *    DESCRIPTION: Tests invalid opcode exception (INT 6)
 *    INPUTS: none
 *    OUTPUT: none
 *    RETURN VALUE: none
 * 	  SIDE EFFECTS: Should force an infinite loop because our exception handlers just infinite loop
 */
int test_opcode_exception() {
 	TEST_HEADER;

 	// The processor should throw invalid opcode as register CR6 is reserved
 	asm volatile("movl %eax, %cr6");

 	return FAIL;		// If exception wasn't thrown and we aren't looping, we failed (at least for MP3.1)	
}


/*
 * test_page_fault (MP3.1)
 *    DESCRIPTION: Test accessing a part of the physical mem that's supposed to be restricted such as first 4MB
 *    INPUTS: none
 *    OUTPUTS: none
 *    RETURN VALUES: none
 *    SIDE EFFECTS: Should throw page fault as non-present mem shouldn't be accessible
 */
int test_page_fault(){
	TEST_HEADER;
	int * test_ptr = (int *) 0x90000;
	int a = *(test_ptr);
	a = 0;					// Dodge unused var warning
	return FAIL;			// If exception wasn't thrown and we aren't looping, we failed (at least for MP3.1)
}


/*
 * test_no_page_fault (MP3.1)
 *    DESCRIPTION: Test accessing a part of the physical mem that's not restricted such as xb8001
 *    INPUTS: none
 *    OUTPUTS: none
 *    RETURN VALUES: none
 *    SIDE EFFECTS: Should throw page fault as non-present mem shouldn't be accessible
 */
int test_no_page_fault(){
	TEST_HEADER;
	int * test_ptr = (int *) 0xb8001;
	int a = *(test_ptr);
	a = 0;				// Dodge unused var warning
	return PASS;		// If exception wasn't thrown and we aren't looping, we passed (at least for MP3.1)
}

/* Checkpoint 2 (MP3.2) tests */

int list_all_files(){
	char * names[] = {".", "sigtest", "shell", "grep", "syserr", "rtc", "fish", "counter",
    "pingpong", "cat", "frame0.txt", "verylargetextwithverylongname.txt", "ls", "testprint",
	"created.txt", "frame1.txt", "hello"};
		
	uint32_t i, j;
	uint32_t num_files=boot->num_dentries;
	
	for(i=0; i<num_files; i++){
		dentry_t dentry;
		(void)read_dentry_by_name((uint8_t*)names[i], &dentry);
		printf("file_name: ");
		for(j=0; j<32; j++){
			if(dentry.fname[j] == 0)
				break;
			putc(dentry.fname[j],0);
		}
		//printf("%s", (int8_t*)dentry.fname);
		//printf((uint8_t*)names[i]);
		//printf(" type: ");
		//printf(dentry.ftype);
		printf("\n");
	}
	return PASS;
	
}

int read_file_by_name(){
	dentry_t dentry;
	uint32_t index;
	int32_t nbytes;
	uint8_t buffer[512];
	uint32_t length;
	uint32_t finode;
	int i;

	uint8_t* fname= (uint8_t*)"frame0.txt";

	(void)read_dentry_by_name((uint8_t*)fname, &dentry);

	index=dentry.inode;
	finode=((uint32_t)boot + (BLOCK_SIZE * (index + 1)));
	length=((inode_t*)finode)->file_size;

	for(i = 0; i < length; i++){
		buffer[i] = 0x34;
	}

	nbytes=read_data(index,0,buffer,length);
	//printf("\n");
	//printf("%x", *((int *)(buffer)));
	//printf("\n");
	//printf("%d \n", index);
	//printf("%s \n", dentry.fname);
	terminal_write(NULL, (char*)buffer, nbytes);


	return PASS;
}


/*
 * test_RTC_open
 *    DESCRIPTION: Test if we set RTC to 2Hz.
 *    INPUTS: none
 *    OUTPUTS: none
 *    RETURN VALUES: none
 *    SIDE EFFECTS: Should set RTC to 2Hz. Look at top row.
 */
int test_RTC_open() {
	TEST_HEADER;
	RTC_open(NULL);
	return PASS;
}

/*
 * test_RTC_read
 *    DESCRIPTION: Test if we properly block the RTC.
 *    INPUTS: none
 *    OUTPUTS: none
 *    RETURN VALUES: none
 *    SIDE EFFECTS: Should block the system for 6 secs total and print twelve 1's.
 */
int test_RTC_read() {
	TEST_HEADER;
	int i;
	for(i = 0; i < 6; i++)
		RTC_read(NULL, NULL, NULL);
	printf("Six 1's should've printed, and now we print 6 more");
	for(i = 0; i < 6; i++)
		RTC_read(NULL, NULL, NULL);
	return PASS;
}

/*
 * test_RTC_write
 *    DESCRIPTION: Test if we can change the RTC frequency
 *    INPUTS: none
 *    OUTPUTS: none
 *    RETURN VALUES: none
 *    SIDE EFFECTS: RTC should be set to appropriate buf frequency
 */
int test_RTC_write(){
	TEST_HEADER;
	uint32_t buf = 512; // try 512 Hz
	if(RTC_write(NULL, &buf, NULL) == -1)
		printf("RTC freq %u invalid", buf);
	return PASS;
}

// Change this definition to test different buffer sizes for the terminal test below
#define test_term_buf_size 130

/*
 * test_terminal_keyboard
 *    DESCRIPTION: Test terminal and keyboard input for consistency
 *    INPUTS: none
 *    OUTPUTS: none
 *    RETURN VALUES: none
 *    SIDE EFFECTS: Should echo keyboard buffer upon hitting enter
 */
int test_terminal_keyboard(){
	/* TEST_HEADER;
	int32_t fd;				
	char buf[test_term_buf_size];				// Buffer that keyboard_buf should be copied to 
	terminal_open(NULL);
	terminal_read(fd, buf, test_term_buf_size);
	terminal_write(fd, buf, terminals[scheduled_terminal].term_kb_buf_i);
	// If the buffers up until '\n' are not the same, the copy failed
	if(strncmp(buf, terminals[scheduled_terminal].term_kb_buf, terminals[scheduled_terminal].term_kb_buf_i))
		return FAIL;
	terminal_read(fd, buf, test_term_buf_size);
	terminal_write(fd, buf, terminals[scheduled_terminal].term_kb_buf_i);
	if(strncmp(buf, terminals[scheduled_terminal].term_kb_buf, terminals[scheduled_terminal].term_kb_buf_i))
		return FAIL;
	*/
	return PASS;
}

/* Checkpoint 3 (MP3.3) tests */


/* Checkpoint 4 (MP3.4) tests */


/* Checkpoint 5 (MP3.5) tests */


/* Test suite entry point */
void launch_tests(){
	TEST_OUTPUT("idt_test", idt_test());							// Checks descriptor offset field for NULL
	// launch your tests here
	//TEST_OUTPUT("test_opcode_exception", test_opcode_exception());
	//TEST_OUTPUT("test_divzero_exception", test_divzero_exception()); 
	//TEST_OUTPUT("test_no_page_fault", test_no_page_fault());
	//TEST_OUTPUT("test_page_fault", test_page_fault());
	//TEST_OUTPUT("test_RTC_open", test_RTC_open());
	//TEST_OUTPUT("test_RTC_read", test_RTC_read());
	//TEST_OUTPUT("test_RTC_write", test_RTC_write());
	//TEST_OUTPUT("test_terminal_keyboard", test_terminal_keyboard());
	//TEST_OUTPUT("list_all_files", list_all_files());
	//TEST_OUTPUT("read_file_by_name", read_file_by_name());
}
