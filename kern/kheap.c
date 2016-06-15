#include <inc/memlayout.h>
#include <kern/kheap.h>
#include <kern/memory_manager.h>

//my helper global variables
uint32 kernelInside = KERNEL_HEAP_START;
int nextBlock = 0;

//my helper global structure
struct keep{
	//keep track with the blocks info.
	uint32* blockStartVirtualAddress;
	uint32 blockSize;
};

//my helper global arrays
struct keep keepBlocks[512];

//2016: NOTE: All kernel heap allocations are multiples of PAGE_SIZE (4KB)

void* kmalloc(unsigned int size)
{
	//TODO: [PROJECT 2016 - Kernel Dynamic Allocation/Deallocation] kmalloc()
	// Write your code here, remove the panic and write your code
	//panic("kmalloc() is not implemented yet...!!");

	//NOTE: Allocation is continuous increasing virtual address
	//NOTE: All kernel heap allocations are multiples of PAGE_SIZE (4KB)
	//refer to the project documentation for the detailed steps

	//check the size
	if(size >= KERNEL_HEAP_MAX - kernelInside) return NULL;
	//--------------why not !-------------------------------
	//if(kernelInside + size >= KERNEL_HEAP_MAX) return NULL;

	//good!, let's begin
	//uint32 endVA = ROUNDUP((uint32) kernelInside + size, PAGE_SIZE);
	uint32 va;
	for(va = kernelInside; va < kernelInside + size; va += PAGE_SIZE){
		//(1) allocate a free frame
		struct Frame_Info* frameInfo = NULL;
		if(allocate_frame(&frameInfo) == E_NO_MEM) return NULL;

		//(2) map the page to the allocated frame
		if(map_frame(ptr_page_directory, frameInfo, (void*) va,
				PERM_WRITEABLE | PERM_PRESENT) == E_NO_MEM)
			return NULL;
	}

	//keep the block in my keepBlock array
	keepBlocks[nextBlock].blockStartVirtualAddress = (uint32*) kernelInside;
	keepBlocks[nextBlock++].blockSize = size;

	//TODO: [PROJECT 2016 - BONUS1] Implement a Kernel allocation strategy
	// Instead of the continuous allocation/deallocation, implement one of
	// the strategies NEXT FIT, BEST FIT, .. etc


	//change this "return" according to your answer
	//uint32 ret = kernelInside;
	kernelInside = va;
	return (void*) keepBlocks[nextBlock - 1].blockStartVirtualAddress;
}

void kfree(void* virtual_address)
{
	//TODO: [PROJECT 2016 - Kernel Dynamic Allocation/Deallocation] kfree()
	// Write your code here, remove the panic and write your code
	//panic("kfree() is not implemented yet...!!");

	//get the size of the given allocation using its address
	//refer to the project documentation for the detailed steps

	//validate the virtual address
	if((uint32) virtual_address < KERNEL_HEAP_START || (uint32) virtual_address >= KERNEL_HEAP_MAX) return;

	//check the given virtual address
	uint32* pageTable = NULL;
	struct Frame_Info* frameInfo = NULL;
	frameInfo = get_frame_info(ptr_page_directory, virtual_address, &pageTable);

	if(frameInfo == NULL) return;


	//good!, then searching about the block
	int blockIndex = findBlock(virtual_address);
	if(blockIndex == -1) return;

	//now we can unmap this block, see it's easy
	uint32 va;
	for(va = (uint32) virtual_address; va < (uint32) virtual_address + keepBlocks[blockIndex].blockSize;
			va += PAGE_SIZE)
		//unmap block pages
		unmap_frame(ptr_page_directory, (void*) va);

	//delete the unmaped block from the keepBlock array
	int i;
	for(i = blockIndex; i < nextBlock - 1; i++)
		keepBlocks[i] = keepBlocks[i + 1];

	nextBlock--;

	//TODO: [PROJECT 2016 - BONUS1] Implement a Kernel allocation strategy
	// Instead of the continuous allocation/deallocation, implement one of
	// the strategies NEXT FIT, BEST FIT, .. etc

}

unsigned int kheap_virtual_address(unsigned int physical_address)
{
	//TODO: [PROJECT 2016 - Kernel Dynamic Allocation/Deallocation] kheap_virtual_address()
	// Write your code here, remove the panic and write your code
	//panic("kheap_virtual_address() is not implemented yet...!!");

	//return the virtual address corresponding to given physical_address
	//refer to the project documentation for the detailed steps

	//sacn the whole kernel heap to find the mapped virtual address
	uint32 va;
	for(va = KERNEL_HEAP_START; va 	< (uint32) keepBlocks[nextBlock - 1].blockStartVirtualAddress
	 + keepBlocks[nextBlock - 1].blockSize; va += PAGE_SIZE){
		//use kheap_physical_address
		uint32 framePhysicalAddress = kheap_physical_address(va);
		if(framePhysicalAddress == physical_address)
			return va;

	}


	//change this "return" according to your answer
	return 0;
}

unsigned int kheap_physical_address(unsigned int virtual_address)
{
	//TODO: [PROJECT 2016 - Kernel Dynamic Allocation/Deallocation] kheap_physical_address()
	// Write your code here, remove the panic and write your code
	//panic("kheap_physical_address() is not implemented yet...!!");

	//return the physical address corresponding to given virtual_address
	//refer to the project documentation for the detailed steps
	uint32* pageTable = NULL;
	struct Frame_Info* frameInfo = NULL;
	frameInfo = get_frame_info(ptr_page_directory, (void*) virtual_address, &pageTable);

	if(frameInfo != NULL)
		return (uint32) to_physical_address(frameInfo);

	//change this "return" according to your answer
	return 0;
}

int findBlock(void* virtualAddress){
	int index;
	for(index = 0; index < nextBlock; index++)
		if(keepBlocks[index].blockStartVirtualAddress == (uint32*) virtualAddress)
			return index;

	return -1;
}
