///////////////////////////////////////////////////////////////////////////////
//
// Copyright 2020-2021 Deb Deppeler based on work by Jim Skrentny
// Posting or sharing this file is prohibited, including any changes/additions.
// Used by permission Fall 2021, CS354-deppeler
//
///////////////////////////////////////////////////////////////////////////////

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdio.h>
#include <string.h>
#include "myHeap.h"
 
/*
 * This structure serves as the header for each allocated and free block.
 * It also serves as the footer for each free block but only containing size.
 */
typedef struct blockHeader {           

    int size_status;
    /*
     * Size of the block is always a multiple of 8.
     * Size is stored in all block headers and in free block footers.
     *
     * Status is stored only in headers using the two least significant bits.
     *   Bit0 => least significant bit, last bit
     *   Bit0 == 0 => free block
     *   Bit0 == 1 => allocated block
     *
     *   Bit1 => second last bit 
     *   Bit1 == 0 => previous block is free
     *   Bit1 == 1 => previous block is allocated
     * 
     * End Mark: 
     *  The end of the available memory is indicated using a size_status of 1.
     * 
     * Examples:
     * 
     * 1. Allocated block of size 24 bytes:
     *    Allocated Block Header:
     *      If the previous block is free      p-bit=0 size_status would be 25
     *      If the previous block is allocated p-bit=1 size_status would be 27
     * 
     * 2. Free block of size 24 bytes:
     *    Free Block Header:
     *      If the previous block is free      p-bit=0 size_status would be 24
     *      If the previous block is allocated p-bit=1 size_status would be 26
     *    Free Block Footer:
     *      size_status should be 24
     */
} blockHeader;         

/* Global variable - DO NOT CHANGE. It should always point to the first block,
 * i.e., the block at the lowest address.
 */
blockHeader *heapStart = NULL;     

/* Size of heap allocation padded to round to nearest page size.
 */
int allocsize;
/*
 * Additional global variables may be added as needed below
 */

/*
* gets the closest value that is a multiple of * to preserve the structure 
*/
int get8num(int given){

    while(given%8 != 0){
        given++; 
    }
    return given; 
}
/*
* gets the absolute value of a number 
*/
int abs(int given){
    if(given<0){
        return given *-1;
    }
    return given;
}
/* 
 * Function for allocating 'size' bytes of heap memory.
 * Argument size: requested size for the payload
 * Returns address of allocated block (payload) on success.
 * Returns NULL on failure.
 *
 * This function must:
 * - Check size - Return NULL if not positive or if larger than heap space.
 * - Determine block size rounding up to a multiple of 8 
 *   and possibly adding padding as a result.
 *
 * - Use BEST-FIT PLACEMENT POLICY to chose a free block
 *
 * - If the BEST-FIT block that is found is exact size match
 *   - 1. Update all heap blocks as needed for any affected blocks
 *   - 2. Return the address of the allocated block payload
 *
 * - If the BEST-FIT block that is found is large enough to split 
 *   - 1. SPLIT the free block into two valid heap blocks:
 *         1. an allocated block
 *         2. a free block
 *         NOTE: both blocks must meet heap block requirements 
 *       - Update all heap block header(s) and footer(s) 
 *              as needed for any affected blocks.
 *   - 2. Return the address of the allocated block payload
 *
 * - If a BEST-FIT block found is NOT found, return NULL
 *   Return NULL unable to find and allocate block for desired size
 *
 * Note: payload address that is returned is NOT the address of the
 *       block header.  It is the address of the start of the 
 *       available memory for the requesterr.
 *
 * Tips: Be careful with pointer arithmetic and scale factors.
 */
void* myAlloc(int size) {   
    static int ammountalocaed;// a varible that can check wether or not its the first instance of an addition   
    //TODO: Your code goes in here.
    int blocksize = size+4; // adds 4 bytes to acount for the head
    blocksize = get8num(blocksize); // calls a helper function 
    blockHeader *current = heapStart; // sets the current to the start of the heap for best fit 
    // making sure that the blocksize is not larger than the heap 
    if(blocksize >= allocsize){
        return NULL;
    }
    // if this is the first thing added to the heap 
    if(ammountalocaed == 0){
        blockHeader *this = ((blockHeader*)((char*)current + blocksize));// splits the heap 
        this->size_status = (allocsize- blocksize)+2; // makes sure the size is correct 
        current->size_status = blocksize+3; // has the size status as tis block is reserved and since its at the begining the one before is too 
        void *ptr = ((char*)current+4);// adds 4 bytes to get the start of the ptr excuding the header 
        ammountalocaed = 1+ammountalocaed;// adds to allocated 
        return ptr; 
        
    }
    blockHeader *closest = NULL;
    int cursize  = allocsize; // current correct size for best fit 
    // iterates untill at the end block 
    while(current->size_status !=1 ){
         int sizeblock =  current->size_status;
        // if it is free
        if(sizeblock % 2 == 0){
            // if the previous is not free 
            if(sizeblock &2){
                sizeblock = sizeblock - 2;
            }
            // if it is the best fit meaning equal 
            if(sizeblock == blocksize){
                closest = current ;
                break;

            }
            //determining the closest to the correct block size 
            else if((sizeblock-blocksize)<cursize &&(sizeblock-blocksize)>0 ){
                cursize = sizeblock-blocksize;
                closest = current;
            }
            //going to the next current 
            if(current->size_status &2){
                current = (blockHeader*)((char*)current + (current->size_status-2));
            }else{
               current = (blockHeader*)((char*)current + (current->size_status));
            }
        }else{
             //going to the next current untill its free 
                if(sizeblock &2){
                    sizeblock = sizeblock -3 ;
                    current = (blockHeader*)((char*)current + sizeblock);
                }else{
                     sizeblock = sizeblock -1;
                     current = (blockHeader*)((char*)current + sizeblock);
                }
        }

    }
    //gotta change footer

    //if there is a closest 
    if(closest !=NULL){
        //if the closest is too small 
        if(closest->size_status < blocksize){
            return NULL;
        }
          int sizeforblock ;
         // checking if it has a previous  
        if(closest->size_status & 2){
             sizeforblock = closest->size_status -2;
        }else{
             sizeforblock = closest->size_status;
        }
        // if the size is equal to the size needed 
        if(blocksize == sizeforblock){
            closest->size_status = closest->size_status+1;
            blockHeader *next = (blockHeader*)((char*)closest+sizeforblock);
            if(next->size_status != 1){
                next->size_status = next->size_status +2;
            }
            void *ptr = ((char*)closest+4);
            ammountalocaed++;
            return ptr;
        // if it is unequal preform a split and then add  
        }else{

            blockHeader *this = ((blockHeader*)((char*)closest + blocksize));
            if(closest->size_status & 2){
                this->size_status = (closest->size_status - blocksize);
                closest->size_status = blocksize+3;
            }else{
                this->size_status = (closest->size_status - blocksize)+2;
                closest->size_status = blocksize+1;
            }
            void *ptr = ((char*)closest+4);
            ammountalocaed++;
            return ptr; 
        }
    }
    return NULL;
} 
 
/* 
 * Function for freeing up a previously allocated block.
 * Argument ptr: address of the block to be freed up.
 * Returns 0 on success.
 * Returns -1 on failure.
 * This function should:
 * - Return -1 if ptr is NULL.
 * - Return -1 if ptr is not a multiple of 8.
 * - Return -1 if ptr is outside of the heap space.
 * - Return -1 if ptr block is already freed.
 * - Update header(s) and footer as needed.
 */                    
int myFree(void *ptr) {    
    // checks  pointer is not null 
    if(ptr == NULL){
        return -1;
    }
    blockHeader *current = (blockHeader*)((char*)ptr-4); // gets the current is the ptr -4 bytes to get the header 
    int size =current->size_status;
    blockHeader *end = (blockHeader*)((char*)heapStart +allocsize );// the end block 
    // if the current is out of our heap or equal to the end 
    if(current >= end){
        return -1;
    }
    // if it is alread freed 
    if(size% 2 ==0){
        return -1;
    }
    // making sure that the size is in padding 
    if(size &2){
        int temp = size -3;
        if(temp %8 != 0){
            return -1;
        }
    }else{
        int temp = size -1;
        if(temp %8 != 0){
            return -1;
        }
    }
    blockHeader *foot = NULL;
    blockHeader *next = NULL;
    //if the previous is allocated 
    if(size &2){
          int temp = size -3 ;
          foot = (blockHeader*)((char*)current+(temp -4));// makes the footer 
          foot->size_status = temp;// change foooter 
          next = (blockHeader*)((char*)foot+4);// updates the next header
          next->size_status = (next->size_status -2);
          current->size_status = temp+2; // changes the current header 
    // if the previous is not allocated 
    }else{
          int temp = size -1 ;
          foot = (blockHeader*)((char*)current+(temp-4));
          foot->size_status = temp;
          next = (blockHeader*)((char*)foot+4);
          next->size_status = (next->size_status -2);
          current->size_status = temp;
    }

    return 0;
} 

/*
 * Function for traversing heap block list and coalescing all adjacent 
 * free blocks.
 *
 * This function is used for delayed coalescing.
 * Updated header size_status and footer size_status as needed.
 */
int coalesce() {
    blockHeader *current  = heapStart;//starts at the begining 
    int counter = 0;
    // untill at the end of heap 
    while (current->size_status !=1)
    {
        int curr_stat = current->size_status; // gets the current size 
        // if it is free 
        if(curr_stat %2 ==0){
            if(curr_stat &2){// if the previous is not free 
                 current = (blockHeader*)((char*)current+(curr_stat-2)); // keep going 
            }
            //if previous is free updated and combine 
            else{

                counter =counter+1;// for how many colaced 
                int temp = curr_stat; 
                blockHeader *foot = (blockHeader*)((char*)current-4);// get the footer from the previous free 
                int sizeofprev = foot->size_status; // gets the size of the previous 
                blockHeader *previous =  (blockHeader*)((char*)current-sizeofprev);// gets the previous 
                previous->size_status = temp+previous->size_status;// updates size of previous 
                blockHeader *curr_foot = (blockHeader*)((char*)current+(temp-4));//gets current footer 
                curr_foot->size_status = temp +sizeofprev;// updates size
                current = (blockHeader*)((char*)current+(curr_stat));// keeps going 
            }
        }
        //if its not free it keeps going till the free on exists 
        else{
             if(curr_stat &2){
                current = (blockHeader*)((char*)current+(curr_stat-3));
             }else{
                 current = (blockHeader*)((char*)current+(curr_stat-1));
             }
        }
    }
    return counter;
}

 
/* 
 * Function used to initialize the memory allocator.
 * Intended to be called ONLY once by a program.
 * Argument sizeOfRegion: the size of the heap space to be allocated.
 * Returns 0 on success.
 * Returns -1 on failure.
 */                    
int myInit(int sizeOfRegion) {    
 
    static int allocated_once = 0; //prevent multiple myInit calls
 
    int pagesize;   // page size
    int padsize;    // size of padding when heap size not a multiple of page size
    void* mmap_ptr; // pointer to memory mapped area
    int fd;

    blockHeader* endMark;
  
    if (0 != allocated_once) {
        fprintf(stderr, 
        "Error:mem.c: InitHeap has allocated space during a previous call\n");
        return -1;
    }

    if (sizeOfRegion <= 0) {
        fprintf(stderr, "Error:mem.c: Requested block size is not positive\n");
        return -1;
    }

    // Get the pagesize
    pagesize = getpagesize();

    // Calculate padsize as the padding required to round up sizeOfRegion 
    // to a multiple of pagesize
    padsize = sizeOfRegion % pagesize;
    padsize = (pagesize - padsize) % pagesize;

    allocsize = sizeOfRegion + padsize;

    // Using mmap to allocate memory
    fd = open("/dev/zero", O_RDWR);
    if (-1 == fd) {
        fprintf(stderr, "Error:mem.c: Cannot open /dev/zero\n");
        return -1;
    }
    mmap_ptr = mmap(NULL, allocsize, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
    if (MAP_FAILED == mmap_ptr) {
        fprintf(stderr, "Error:mem.c: mmap cannot allocate space\n");
        allocated_once = 0;
        return -1;
    }
  
    allocated_once = 1;

    // for double word alignment and end mark
    allocsize -= 8;

    // Initially there is only one big free block in the heap.
    // Skip first 4 bytes for double word alignment requirement.
    heapStart = (blockHeader*) mmap_ptr + 1;

    // Set the end mark
    endMark = (blockHeader*)((void*)heapStart + allocsize);
    endMark->size_status = 1;

    // Set size in header
    heapStart->size_status = allocsize;

    // Set p-bit as allocated in header
    // note a-bit left at 0 for free
    heapStart->size_status += 2;

    // Set the footer
    blockHeader *footer = (blockHeader*) ((void*)heapStart + allocsize - 4);
    footer->size_status = allocsize;
  
    return 0;
} 
                  
/* 
 * Function to be used for DEBUGGING to help you visualize your heap structure.
 * Prints out a list of all the blocks including this information:
 * No.      : serial number of the block 
 * Status   : free/used (allocated)
 * Prev     : status of previous block free/used (allocated)
 * t_Begin  : address of the first byte in the block (where the header starts) 
 * t_End    : address of the last byte in the block 
 * t_Size   : size of the block as stored in the block header
 */                     
void dispMem() {     
 
    int counter;
    char status[6];
    char p_status[6];
    char *t_begin = NULL;
    char *t_end   = NULL;
    int t_size;

    blockHeader *current = heapStart;
    counter = 1;

    int used_size = 0;
    int free_size = 0;
    int is_used   = -1;

    fprintf(stdout, 
	"*********************************** Block List **********************************\n");
    fprintf(stdout, "No.\tStatus\tPrev\tt_Begin\t\tt_End\t\tt_Size\n");
    fprintf(stdout, 
	"---------------------------------------------------------------------------------\n");
  
    while (current->size_status != 1) {
        t_begin = (char*)current;
        t_size = current->size_status;
    
        if (t_size & 1) {
            // LSB = 1 => used block
            strcpy(status, "alloc");
            is_used = 1;
            t_size = t_size - 1;
        } else {
            strcpy(status, "FREE ");
            is_used = 0;
        }

        if (t_size & 2) {
            strcpy(p_status, "alloc");
            t_size = t_size - 2;
        } else {
            strcpy(p_status, "FREE ");
        }

        if (is_used) 
            used_size += t_size;
        else 
            free_size += t_size;

        t_end = t_begin + t_size - 1;
    
        fprintf(stdout, "%d\t%s\t%s\t0x%08lx\t0x%08lx\t%4i\n", counter, status, 
        p_status, (unsigned long int)t_begin, (unsigned long int)t_end, t_size);
    
        current = (blockHeader*)((char*)current + t_size);
        counter = counter + 1;
    }

    fprintf(stdout, 
	"---------------------------------------------------------------------------------\n");
    fprintf(stdout, 
	"*********************************************************************************\n");
    fprintf(stdout, "Total used size = %4d\n", used_size);
    fprintf(stdout, "Total free size = %4d\n", free_size);
    fprintf(stdout, "Total size      = %4d\n", used_size + free_size);
    fprintf(stdout, 
	"*********************************************************************************\n");
    fflush(stdout);

    return;  
} 



// }

// end of myHeap.c (Fall 2021)                                         
// dd


