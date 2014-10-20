#include <stdio.h>
#include <stdlib.h>
#include "types.h"
#include "tlb.h"
#include "cpu.h"
#include "mmu.h"

// Student: Shawn Piano Vega     	  
// Student Email: spv224@ynu.edu          
// Student Number: N18649228	          
// Assignment: program 2		   	
// Instructor:Professor Goldberg          
// Course: Operating Systems 

/* I defined the TLB as an array of entries,
   each containing the following:
   Valid bit: 1bit
   Virtual Page: 21 bits
   Reference bit: 1 bit
   Modified bit: 1 bit
   Page Frame: 21 bits
*/

typedef struct {
  unsigned int vbit_and_vpage;  // 32 bits containing the valid bit and the 21-bit
                                // virtual page number.
  unsigned int mr_pframe;       // 32 bits containing the modified bit, reference bit,
                                // and 21-bit page frame number
} TLB_ENTRY;

// This is the actual TLB array. It should be dynamically allocated
// to the right size, depending on the num_tlb_entries value 
// assigned when the simulation started running.

TLB_ENTRY *tlb;

// This is the TLB size (number of TLB entries) chosen by the user. 
unsigned int num_tlb_entries;

BOOL tlb_miss;  //this is set to TRUE when there is a tlb miss;

#define VBIT_MASK   0x80000000  //VBIT is leftmost bit of first word 
#define VPAGE_MASK  0x001FFFFF  //lowest 21 bits of first word 
#define MBIT_MASK   0x80000000  //MBIT is leftmost bit of second word
#define RBIT_MASK   0x40000000  //RIT is second leftmost bit of second word 
#define PFRAME_MASK 0x001FFFFF  //lowest 21 bits of second word

 int next_vpage_to_check;  // points to next TLB entry to consider writing to.
/*----------------------------Initialize-----------------------------------------------*/
// Initialize the TLB (called by the mmu)
void tlb_initialize()
{                  
  //Here's how you can allocate a TLB of the right size (32 bits)
  tlb = (TLB_ENTRY *) malloc(num_tlb_entries * sizeof(TLB_ENTRY));
  next_vpage_to_check = 0;			
  int i;
  for (i = 0; i < num_tlb_entries; i++){
	  tlb[i].vbit_and_vpage=0;
	  tlb[i].mr_pframe=0;
  }
}
/*-----------------------------mmu_modify_mr_bit----------------------------------------*/
// This gives allows us to to insert the single 
// binary value 0 or 1 into the bitmap without 
// all the extra binary numbers associated with 
// the vpage and pframe.
void mmu_modify_mr_bit(int val){
	int modified;
	int referenced;
	//set r bit to either 0 or 1
	if ((val & RBIT_MASK) != 0){referenced = 1;}
	else{referenced = 0;}
	//set r bit to either 0 or 1
	if ((val & MBIT_MASK) != 0){modified = 1;}
	else{modified = 0;}
	mmu_modify_rbit_in_bitmap(val & PFRAME_MASK, referenced);
	mmu_modify_mbit_in_bitmap(val & PFRAME_MASK, modified);
}
/*-----------------------------------tlb_clear_all--------------------------------------*/
void tlb_clear_all() 
{
  int i;
  //ex: if the valid bit is 1, this will & with the VBIT_MASK 
  //1&1=1 and then we ~1 to get 0
  for (i = 0; i < num_tlb_entries; i++){tlb[i].vbit_and_vpage &= (~VBIT_MASK);}
}
/*-----------------------------------tlb_clear_R_bits-----------------------------------*/
//clears all the R bits in the TLB
void tlb_clear_R_bits() 
{
   int i;
   //ex: if the valid bit is 1, this will & with the RBIT_MASK
   //1&1=1 and then we ~1 to get 0
   for (i = 0; i < num_tlb_entries; i++){tlb[i].mr_pframe &= (~RBIT_MASK);}
}
/*---------------------------------tlb_clear_entry--------------------------------------*/
// This clears out the entry in the TLB for the specified
// virtual page, by clearing the valid bit for that entry.
void tlb_clear_entry(VPAGE_NUMBER vpage)
{
  	int i=0;
	for (i = 0; i < num_tlb_entries; i++){
		//ex: if the valid bit is 1, this will & with the VBIT_MASK 
  		//1&1=1 and then we ~1 to get 0
		if((tlb[i].vbit_and_vpage & VPAGE_MASK)==vpage){tlb[i].vbit_and_vpage &= (~VBIT_MASK);}	
	}
}
/*-----------------------------PAGEFRAME_NUMBER tlb_lookup_vpage------------------------*/
// Returns a page frame number if there is a TLB hit. If there is a
// TLB miss, then it sets tlb_miss (see above) to TRUE. Otherwise, it
// sets tlb_miss to FALSE, sets the R bit of the entry and, if the
// specified operation is a STORE, sets the M bit.
PAGEFRAME_NUMBER tlb_lookup_vpage(VPAGE_NUMBER vpage, OPERATION op)
{
  	int i;
	for (i = 0; i < num_tlb_entries; i++) {		
		if( (((tlb[i].vbit_and_vpage) & (VBIT_MASK))!=0) ){
			//there was a hit
			if(((tlb[i].vbit_and_vpage) & (VPAGE_MASK))==vpage){
				tlb_miss=FALSE;//0
				//set the R bit
  				//ex: r bit is 0 so 0|1 gives us 1 
  				tlb[i].mr_pframe |= RBIT_MASK;
				//check operation 
				if(op==STORE){
					//set the M bit
  					//ex: m bit is 0 so 0|1 gives us 1 
  			     	tlb[i].mr_pframe |= MBIT_MASK;
				}
			//return page frame number 
			return (tlb[i].mr_pframe & PFRAME_MASK);
			}
		}
	}
	return tlb_miss=TRUE;
}
/*-------------------------------tlb_insert_vpage---------------------------------------*/
// tlb_insert_vpage(), below, inserts a new mapping of
// vpage to pageframe into the TLB. When choosing which
// entry in the TLB to write to, it selects the first
// entry it encounters that either has a zero valid bit 
// or a zero R bit. To avoid choosing the same entries
// over and over, it starts searching the entries from
// where it left off the previous time (see the description
// below.
void tlb_insert_vpage(VPAGE_NUMBER new_vpage, PAGEFRAME_NUMBER new_pframe,
		BOOL new_rbit, BOOL new_mbit)
{
  // Starting at tlb[next_vpage_to_check], choose the first entry
  // with either valid bit  = 0 or the R bit = 0 to write to. If there

  // If the chosen entry has a valid bit = 1 (i.e. a valid entry is
  // being evicted), then write the M and R bits of the entry back
  // to the M and R bitmaps, respectively, in the MMU (see
  // mmu_modify_rbit_in_bitmap, etc. in mmu.h)

	BOOL found = FALSE;
	//keeps track of the last position the page was inserted 
    int next_fit = -1;
    int i;
	for (i=next_vpage_to_check ; i < num_tlb_entries; i++){
		if((((tlb[i].vbit_and_vpage) & (VBIT_MASK))==0) || (((tlb[i].vbit_and_vpage) & (RBIT_MASK))==0)){
			if( (((tlb[i].vbit_and_vpage) & (VBIT_MASK))!=0) ){
  			//Writes all the R and M bits of the valid entries of the
			//TLB back to the page table
			int val=tlb[i].mr_pframe;
			mmu_modify_mr_bit(val);
			next_vpage_to_check=i;
			}
			 next_fit = i;
            found = TRUE;
            break;
  		}
  	}
  	if (!found) {
  		for (i = 0; i < next_vpage_to_check; i++){
			 if((((tlb[i].vbit_and_vpage) & (VBIT_MASK))==0) || (((tlb[i].vbit_and_vpage) & (RBIT_MASK))==0)){
					if( (((tlb[i].vbit_and_vpage) & (VBIT_MASK))!=0) ){
  						//Writes all the R and M bits of the valid entries of the
						//TLB back to the page table
						int val=tlb[i].mr_pframe;
						mmu_modify_mr_bit(val);
						next_vpage_to_check=i;
					}
						next_fit = i;
            			found = TRUE;
           				break;
  		 	}
  	   }
  	}
  	if (!found) {// If there is no such entry, then just choose tlb[next_vpage_to_check].
         next_fit = next_vpage_to_check;
        found = TRUE;
    }	
	
  // Then, insert the new vpage, pageframe, R bit, and M bit into the
  // TLB entry that was just found (and possibly evicted).
  
  	//Vpage 
  	tlb[next_fit].vbit_and_vpage=0;
	tlb[next_fit].vbit_and_vpage=new_vpage;
	//set the V bit
  	//ex: v bit is 0 so 0|1 gives us 1 
  	tlb[next_fit].vbit_and_vpage |= VBIT_MASK;
  	
  	//Pframe
  	tlb[next_fit].mr_pframe=0;
	tlb[next_fit].mr_pframe=new_pframe;
	tlb[next_fit].mr_pframe|= (new_rbit << 30);
	tlb[next_fit].mr_pframe|= (new_mbit << 31);

  // set next_vpage_to_check to point to the next entry after the
  // entry found above.
  if(next_fit==num_tlb_entries-1){next_vpage_to_check=0;}
  else{next_vpage_to_check =  next_fit + 1;}
}
/*--------------------------------tlb_write_back_r_m_bits-------------------------------*/
//Writes all the R and M bits of the valid entries of the
//TLB back to the page table.
void tlb_write_back_r_m_bits()
{
    int i;
	for (i = 0; i < num_tlb_entries; i++){
		// write back R and M bits for each entry with valid bit set
		if((tlb[i].vbit_and_vpage & VBIT_MASK)!=0){
			//Writes all the R and M bits of the valid entries of the
			//TLB back to the page table
			int val=tlb[i].mr_pframe;
			mmu_modify_mr_bit(val);
		}
	}
}