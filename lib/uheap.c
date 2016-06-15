
#include <inc/lib.h>
/*
 * Simple malloc()
 *
 * The address space for the dynamic allocation is
 * from "USER_HEAP_START" to "USER_HEAP_MAX"-1
 * Pages are allocated ON 4KB BOUNDARY
 * On succeed, return void pointer to the allocated space
 * return NULL if
 *	-there's no suitable space for the required allocation
 */

//my helper structure
struct memBlock{
	//block pages, and we can get the next block
	int blockPages;
	int priorBlockIndex;
};

//my helper global variables
uint32 NEXT_FIT_PTR = USER_HEAP_START;
int NEXT_FIT_INDEX = 0;

#define HEAP_PAGES_NUMBER (USER_HEAP_MAX - USER_HEAP_START) / PAGE_SIZE //max 2^18 which fit into int (2^31 - 1)
struct memBlock heapBlocks[HEAP_PAGES_NUMBER] = {{HEAP_PAGES_NUMBER, 0}};


//my helper global functions
void* nextFit(uint32 size);
void* bestFit(uint32 size);
int searchNextFit(uint32 size);
int searchBestFit(uint32 size);
void* heapAlloc(uint32 size, int index);
inline uint32 pageNumberToHeapVA(int index);
inline int heapVaToPageNumber(uint32 va);
inline int abs(int num);
void updateHeapBlocks(int blockIndex);


// malloc()
//	This function use both NEXT FIT and BEST FIT strategies to allocate space in heap
//  with the given size and return void pointer to the start of the allocated space

//	To do this, we need to switch to the kernel, allocate the required space
//	in Page File then switch back to the user again.
//
//	We can use sys_allocateMem(uint32 virtual_address, uint32 size); which
//		switches to the kernel mode, calls allocateMem(struct Env* e, uint32 virtual_address, uint32 size) in
//		"memory_manager.c", then switch back to the user mode here
//	the allocateMem function is empty, make sure to implement it.


void* malloc(uint32 size)
{
	//TODO: [PROJECT 2016 - Dynamic Allocation] malloc() [User Side]
	// Write your code here, remove the panic and write your code
	//panic("malloc() is not implemented yet...!!");

	// Steps:
	//	1) Implement both NEXT FIT and BEST FIT strategies to search the heap for suitable space
	//		to the required allocation size (space should be on 4 KB BOUNDARY)
	//	2) if no suitable space found, return NULL
	//	 Else,
	//	3) Call sys_allocateMem to invoke the Kernel for allocation
	// 	4) Return pointer containing the virtual address of allocated space,
	//

	//This function should find the space of the required range
	// ******** ON 4KB BOUNDARY ******************* //

	//Use sys_isUHeapPlacementStrategyNEXTFIT() and	sys_isUHeapPlacementStrategyBESTFIT()
	//to check the current strategy

	//check the fitting strategy
	if(sys_isUHeapPlacementStrategyNEXTFIT()){
		//then apply the NEXT FIT strategy
		return nextFit(ROUNDUP(size, PAGE_SIZE));
	}else if(sys_isUHeapPlacementStrategyBESTFIT()){
		//then apply the BEST FIT strategy
		return bestFit(ROUNDUP(size, PAGE_SIZE));
	}

	//TODO: [PROJECT 2016 - BONUS2] Apply FIRST FIT and WORST FIT policies

	return NULL;
}



// free():
//	This function frees the allocation of the given virtual_address
//	To do this, we need to switch to the kernel, free the pages AND "EMPTY" PAGE TABLES
//	from page file and main memory then switch back to the user again.
//
//	We can use sys_freeMem(uint32 virtual_address, uint32 size); which
//		switches to the kernel mode, calls freeMem(struct Env* e, uint32 virtual_address, uint32 size) in
//		"memory_manager.c", then switch back to the user mode here
//	the freeMem function is empty, make sure to implement it.

void free(void* virtual_address)
{
	//TODO: [PROJECT 2016 - Dynamic Deallocation] free() [User Side]
	// Write your code here, remove the panic Tand write your code
	//panic("free() is not implemented yet...!!");

	//get the size of the given allocation using its address
	//you need to call sys_freeMem()

	//get the size of the block through the given virtual address
	//first convert the given virtual address to page number
	int blockIndex = heapVaToPageNumber((uint32) virtual_address);
	uint32 size = abs(heapBlocks[blockIndex].blockPages) * PAGE_SIZE;

	sys_freeMem((uint32) virtual_address , size);

	//finally deallocate this block from the heapBlock array
	updateHeapBlocks(blockIndex);

}

//my helper global functions
void* nextFit(uint32 size){
	//NEXT FIT strategy

	//first search for the next fit space
	int fitIndex = searchNextFit(size);

	//check if it found a fit space for allocation
	if(fitIndex == -1) return NULL;

	//then we can allocate the size
	return heapAlloc(size, fitIndex);
}

void* bestFit(uint32 size){
	//BEST FIT strategy

	//first search for the best fit space
	int fitIndex = searchBestFit(size);

	//check if it found a fit space for allocation
	if(fitIndex == -1) return NULL;

	//cprintf("size = %d\n", fitIndex * PAGE_SIZE / 1024);
	return heapAlloc(size, fitIndex);
}

int searchNextFit(uint32 size){
	//validate the size
	if(size > USER_HEAP_MAX - USER_HEAP_START) return -1;

	//search for the next fit space for allocating
	short firstTime = 1;
	int blockIndex = NEXT_FIT_INDEX;
	while(blockIndex != NEXT_FIT_INDEX || firstTime){
		//check the block reserved or free

		//check the heap pages end
		if(blockIndex == HEAP_PAGES_NUMBER)
			blockIndex = 0;

		if(heapBlocks[blockIndex].blockPages >= (int) (size / PAGE_SIZE))
			//then there is enough free space for allocating
			return blockIndex;
		else if(heapBlocks[blockIndex].blockPages < (int) (size / PAGE_SIZE))
			//then move to the next block
			blockIndex += abs(heapBlocks[blockIndex].blockPages);

		firstTime = 0;
	}

	return -1;
}

int searchBestFit(uint32 size){
	//validate the size
	if(size > USER_HEAP_MAX - USER_HEAP_START) return -1;

	//declare variable to hold the best fit blockIndex
	int bestFitBlockIndex = -1;

	//start searching for the best fit space
	//start from the beginning of the virtual memory
	int blockIndex = 0;

	while(blockIndex != HEAP_PAGES_NUMBER){
		//if the current block is free and has the same page number as the size then it is the best block
		if(heapBlocks[blockIndex].blockPages == (int) (size / PAGE_SIZE))
			return blockIndex;
		//else if the current block has enough space for allocating and smaller than the last best block
		//then make it the best block
		else if(heapBlocks[blockIndex].blockPages > (int) (size / PAGE_SIZE)){
			if(bestFitBlockIndex == -1) bestFitBlockIndex = blockIndex;
			else
				if(heapBlocks[blockIndex].blockPages < heapBlocks[bestFitBlockIndex].blockPages)
					bestFitBlockIndex = blockIndex;
		}

		blockIndex += abs(heapBlocks[blockIndex].blockPages);
	}

	return bestFitBlockIndex;
}

void* heapAlloc(uint32 size, int index){
	//allocate the heap pages
	int pagesNumber = size / PAGE_SIZE;  //pages to allocate

	if(pagesNumber == heapBlocks[index].blockPages)
		heapBlocks[index].blockPages *= -1;
	else if (pagesNumber < heapBlocks[index].blockPages) {
		heapBlocks[index + pagesNumber].blockPages
				= heapBlocks[index].blockPages - pagesNumber;
		heapBlocks[index].blockPages = pagesNumber * -1;
		//set the prior index for free function
		heapBlocks[index + pagesNumber].priorBlockIndex = index;
		//update the after next prior to the next block
		if(index + pagesNumber + heapBlocks[index + pagesNumber].blockPages != HEAP_PAGES_NUMBER)
			heapBlocks[index + pagesNumber + heapBlocks[index + pagesNumber].blockPages].priorBlockIndex
					= index + pagesNumber;
	}

	//update the global pointers
	NEXT_FIT_INDEX = index + pagesNumber;
	NEXT_FIT_PTR = pageNumberToHeapVA(index + pagesNumber);

	//allocate the pages on the page file
	uint32 startVA = pageNumberToHeapVA(index);
	sys_allocateMem(startVA, size);

	//return start virtual address
	return (void*) startVA;
}

inline int abs(int num){
	if(num < 0) return num * -1;
	return num;
}

inline uint32 pageNumberToHeapVA(int index){
	//convert page index to heap virtual address
	return ((uint32) index * PAGE_SIZE + USER_HEAP_START);
}

inline int heapVaToPageNumber(uint32 va){
	//convert heap virtual address to page number
	return (int) ((va - USER_HEAP_START) / PAGE_SIZE);
}

void updateHeapBlocks(int blockIndex) {
	//NOTE: we can not find allocated block between two free blocks
	int priorBlockIndex = heapBlocks[blockIndex].priorBlockIndex;
	int nextBlockIndex = blockIndex + abs(heapBlocks[blockIndex].blockPages);
	//first check the prior block if it is free then join to it
	if (blockIndex != 0 && nextBlockIndex != HEAP_PAGES_NUMBER
			&& heapBlocks[priorBlockIndex].blockPages > 0
			&& heapBlocks[nextBlockIndex].blockPages > 0) {
		//combine the three free blocks as a one free block
		//update the beginning
		heapBlocks[priorBlockIndex].blockPages += abs(
				heapBlocks[blockIndex].blockPages)
				+ heapBlocks[nextBlockIndex].blockPages;
		//remove the combined blocks
		heapBlocks[blockIndex].blockPages = 0;
		heapBlocks[blockIndex].priorBlockIndex = 0;
		heapBlocks[nextBlockIndex].blockPages = 0;
		heapBlocks[nextBlockIndex].priorBlockIndex = 0;
		//update the next block prior if exist
		//check if the next block exist or not
		if(priorBlockIndex + heapBlocks[priorBlockIndex].blockPages != HEAP_PAGES_NUMBER)
			heapBlocks[priorBlockIndex + heapBlocks[priorBlockIndex].blockPages].priorBlockIndex
			 = priorBlockIndex;
	} else if (heapBlocks[priorBlockIndex].blockPages > 0 && blockIndex != 0) {
		////combine the three free blocks as a one free block
		//then the prior block is free block
		heapBlocks[priorBlockIndex].blockPages += abs(
				heapBlocks[blockIndex].blockPages);
		//remove the combined block
		heapBlocks[blockIndex].blockPages = 0;
		heapBlocks[blockIndex].priorBlockIndex = 0;
		//update the next block prior if exist
		//check if the next block exist or not
		if(priorBlockIndex + heapBlocks[priorBlockIndex].blockPages != HEAP_PAGES_NUMBER)
			heapBlocks[priorBlockIndex + heapBlocks[priorBlockIndex].blockPages].priorBlockIndex
			= priorBlockIndex;
	} else if (heapBlocks[nextBlockIndex].blockPages > 0 && nextBlockIndex
			!= HEAP_PAGES_NUMBER) {
		//combine the three free blocks as a one free block
		//then the next block is free block
		heapBlocks[blockIndex].blockPages = abs(
				heapBlocks[blockIndex].blockPages)
				+ heapBlocks[nextBlockIndex].blockPages;
		//remove the combined block
		heapBlocks[nextBlockIndex].blockPages = 0;
		heapBlocks[nextBlockIndex].priorBlockIndex = 0;
		//update the next block prior if exist
		//check if the next block exist or not
		if(blockIndex + heapBlocks[blockIndex].blockPages != HEAP_PAGES_NUMBER)
			heapBlocks[blockIndex + heapBlocks[blockIndex].blockPages].priorBlockIndex
			= blockIndex;
	} else {
		heapBlocks[blockIndex].blockPages *= -1;
	}

	//cprintf("index = %d, add = %x, #pages = %d\n", blockIndex, pageNumberToHeapVA(blockIndex), heapBlocks[blockIndex].blockPages);

}

//=================================================================================//
//============================== BONUS FUNCTION ===================================//
//=================================================================================//
// realloc():

//	Attempts to resize the allocated space at "virtual_address" to "new_size" bytes,
//	possibly moving it in the heap.
//	If successful, returns the new virtual_address, in which case the old virtual_address must no longer be accessed.
//	On failure, returns a null pointer, and the old virtual_address remains valid.

//	A call with virtual_address = null is equivalent to malloc().
//	A call with new_size = zero is equivalent to free().

//  Hint: you may need to use the sys_moveMem(uint32 src_virtual_address, uint32 dst_virtual_address, uint32 size)
//		which switches to the kernel mode, calls moveMem(struct Env* e, uint32 src_virtual_address, uint32 dst_virtual_address, uint32 size)
//		in "memory_manager.c", then switch back to the user mode here
//	the moveMem function is empty, make sure to implement it.

void *realloc(void *virtual_address, uint32 new_size)
{
	//TODO: [PROJECT 2016 - BONUS4] realloc() [User Side]
	// Write your code here, remove the panic and write your code
	panic("realloc() is not implemented yet...!!");

}
