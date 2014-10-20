#include <stdio.h>
#include <stdlib.h>
#include "types.h"
#include "mmu.h"
#include "page.h"
#include "cpu.h"


// Student: Shawn Piano Vega     	  
// Student Email: spv224@ynu.edu          
// Student Number: N18649228	          
// Assignment: program 3		   	
// Instructor:Professor Goldberg          
// Course: Operating Systems

/* The following machine parameters are being used:
   
   Number of bits in an address:  32
   Page size: 2KB (i.e. 2^11 bytes)
   Number of pages = 4GB address space/ 2KB page size =  2^32/2^11 = 2^21 = 2M pages
   Page Table Type:  2 level page table
   Size of first level page table: 2048 (i.e. 2^11) entries
   Size of first level Page Table Entry:  32 bits
   Size of each 2nd Level Page Table: 1024 (i.e. 2^10) entries
   Size of 2nd Level Page Table Entry: 32 bits

   Bits of address giving index into first level page table: 11
   Bits of address giving index into second level page table: 10
   Bits of address giving offset into page: 11

*/


/* Each entry of a 2nd level page table has
   the following:
   Present/Absent bit: 1 bit
   Modified bit: 1 bit
   Reference bit: 1 bit
   Page Frame: 21 bits
*/
#define VBIT_MASK   0x80000000  //VBIT is leftmost bit of first word 
#define PFRAME_MASK 0x001FFFFF  //lowest 21 bits of second word
#define one_level   0x000007FF // 11 bits to index into first PT
#define two_level   0x001FF800 // 10 bits to index into second PT

// This is the type definition for the 
// an entry in a second level page table

typedef unsigned int PT_ENTRY;

// This is the declaration of the variable, first_level_page_table,
// representing the first level page table. 

PT_ENTRY **first_level_page_table;

BOOL page_fault;  //set to true if there is a page fault

// This sets up the initial page table. The function
// is called by the MMU.
//
// Initially, all the entries of the first level 
// page table should be set to NULL. Later on, 
// when a new page is referenced by the CPU, the 
// second level page table for storing the entry
// for that new page should be created if it doesn't
// exist already. 

void pt_initialize_page_table()
{
	//Size of first level page table: 2048 (i.e. 2^11) entries
	first_level_page_table = (PT_ENTRY **) malloc(2048 * sizeof(PT_ENTRY*));
	 int i;
	 for(i=0; i<2048;i++){
	 first_level_page_table[i]=NULL;
	}
}

// This procedure is called by the MMU when there is a TLB miss.
// The vpage contains 21 significant bits (11 bits to index into first PT, 
// then 10 bits to index into second PT  -- it is not an entire 32-bit 
// physical address). It should return the corresponding page frame 
// number, if there is one.
// It should set page_fault to TRUE if there is a page fault, otherwise
// it should set page_fault to FALSE.

PAGEFRAME_NUMBER pt_get_pframe_number(VPAGE_NUMBER vpage)
{
		// gets 11 bits to index into first PT
	 	int one_index= (vpage & one_level);
	 	if (first_level_page_table[one_index]==NULL){
	 		page_fault=TRUE;
	 	}
	 	else {
	 		//This shift by 11 is important to give you a 
	 		//correct value for the second index
			int two_index= (vpage & two_level) >> 11;
			int level_two_entry= first_level_page_table[one_index][two_index];
	 		//determines if present/absent bit is valid
	 		if ( (level_two_entry & VBIT_MASK) == 0){
	 		page_fault=TRUE;
	 		}
	 		else{
	 		page_fault=FALSE; 
	 	 	return (level_two_entry&PFRAME_MASK);
	 		}
	 }
}

// This inserts into the page table an entry mapping of the 
// the specified virtual page to the specified page frame.
// It might require the creation of a second-level page table
// to hold the entry, if it doesn't already exist.

void pt_update_pagetable(VPAGE_NUMBER vpage, PAGEFRAME_NUMBER pframe)
{
		// gets 11 bits to index into first PT
	 	int one_index= (vpage & one_level);
	 	if (first_level_page_table[one_index]==NULL){
	 		//Size of first level page table: 2048 (i.e. 2^11) entries
			first_level_page_table[one_index] = (PT_ENTRY *) malloc(1024 * sizeof(PT_ENTRY));
		}
		//This shift by 11 is important to give you a 
	 	//correct value for the second index
	 	int two_index=(vpage & two_level) >> 11;
	 	first_level_page_table[one_index][two_index]=pframe;
	 	 //don't forget to set the present bit for the new entry
	 	first_level_page_table[one_index][two_index] |=VBIT_MASK;
}

// This clears the entry of a page table by clearing the present bit.
// It is called by the OS (in kernel.c) when a page is evicted from memory
void pt_clear_page_table_entry(VPAGE_NUMBER vpage)
{
  		// gets 11 bits to index into first PT
	 	int one_index= vpage & one_level;
	 	if (first_level_page_table[one_index]==NULL){
	 		return;
	 	}
		//This shift by 11 is important to give you a 
	 	//correct value for the second index
	 	int two_index= (vpage & two_level) >>11;
	 	first_level_page_table[one_index][two_index] &= (~VBIT_MASK);
}