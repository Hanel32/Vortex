/*--------------------------------------------------------------------------------------------
 - Vortex: Extreme-Performance Memory Abstractions for Data-Intensive Streaming Applications -
 - Copyright(C) 2020 Carson Hanel, Arif Arman, Di Xiao, John Keech, Dmitri Loguinov          -
 - Produced via research carried out by the Texas A&M Internet Research Lab                  -
 -                                                                                           -
 - This program is free software : you can redistribute it and/or modify                     -
 - it under the terms of the GNU General Public License as published by                      -
 - the Free Software Foundation, either version 3 of the License, or                         -
 - (at your option) any later version.                                                       -
 -                                                                                           -
 - This program is distributed in the hope that it will be useful,                           -
 - but WITHOUT ANY WARRANTY; without even the implied warranty of                            -
 - MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the                               -
 - GNU General Public License for more details.                                              -
 -                                                                                           -
 - You should have received a copy of the GNU General Public License                         -
 - along with this program. If not, see < http://www.gnu.org/licenses/>.                     -
 --------------------------------------------------------------------------------------------*/
#include "stdafx.h"

// sets up the StreamPool
StreamPool::StreamPool(uint64_t blockSizePower) : blockSizePower(blockSizePower) {
	cs                 = (CSType*)Syscall.MakeCS();
	minAvailableBlocks = INT64_MAX;	 // lowest # of free blocks during execution of the pool
	pageSizePower      = 12;         // request 4-KB pages 
	blockCount         = 0;		     // no physical blocks
	pageCount          = 0;		     // no pages allocated yet
	tail               = 0;          // top of the page stack
	InitializePool();
}

// used by streams to expand physical memory allocated, requires mutex
void StreamPool::AdjustPoolPhysicalMemory(uint64_t totalPageCount) {
	// assure that no other threads push/pop/adjust memory
	Syscall.EnterCS(cs);

	// if the StreamPool currently doesn't have enough pages, expand
	if (totalPageCount > pageCount) 
		ExpandPhysicalMemory(totalPageCount);

	// yield to other threads
	Syscall.LeaveCS(cs);
}

// private function - used within the StreamPool to expand the amount of physical memory allocated
void StreamPool::ExpandPhysicalMemory(uint64_t totalPageCount) {
	// see if enough memory is available, or more needs to be allocated
	uint64_t extraPageCount = totalPageCount - pageCount;

#ifdef _WIN32
	// assure we allocate enough space for all PFNs
	uint64_t bytes = totalPageCount * sizeof(BlockType);
	if (blockCount == 0) PFN = (BlockType*)malloc(bytes);
	else PFN = (BlockType*)realloc(PFN, bytes);

	// check if malloc/realloc was successful
	if (!PFN) ReportError("failed malloc/realloc with error %d\n", GetLastError());
#endif

	// allocate all pages at the top of the StreamPool stack
#ifdef _WIN32
	// Windows-style physical page allocation
	Syscall.AllocatePages(extraPageCount, pageSize, PFN + tail);
#else
	// Linux-style physical page allocation
	PFN = (BlockType*)Syscall.AllocatePages(PFN,                         // start of page array
		                                    tail,                        // # of pages on stack
		                                    extraPageCount,              // # of pages needed
		                                    pageSize,                    // page byte size
		                                    PFN + pageCount * pageSize); // end of page array
#endif
	// record the total number of pages, moving the PFN stack tail
	pageCount   = totalPageCount;
	blockCount  = totalPageCount / pagesPerBlock;
	tail       += extraPageCount;
}

// return StreamPool resources to the OS
StreamPool::~StreamPool() {
	Syscall.FreePages(PFN, pageCount, pageSize);
	Syscall.DeleteCS(cs);
}

// resets the record of the minimum pages on the stack
void StreamPool::Reset(void) {
	minAvailableBlocks = INT64_MAX;
}

// pushes numPages page frame numbers onto the PFN stack
void StreamPool::ReturnFreeBlock(uint64_t numPages, BlockType* pagePtr) {
	// assure that no other threads push/pop/adjust memory
	Syscall.EnterCS(cs);

#ifdef _WIN32
	// stash the pages from the block; Push()
	memcpy(PFN + tail, pagePtr, numPages * sizeof(BlockType));
#else
	// map the pages back to the VM space; Push()
	mremap(pagePtr, numPages * pageSize, numPages * pageSize, MREMAP_MAYMOVE | MREMAP_FIXED, PFN + tail * pageSize);
#endif
	tail += numPages;

	// yield to other threads
	Syscall.LeaveCS(cs);
}

// pops numPages page frame numbers from the PFN stack
BlockType* StreamPool::GetNewBlock(uint64_t numPages, BlockType* pagePtr) {
	// assure that no other threads push/pop/adjust memory
	Syscall.EnterCS(cs);

	// if the pool is too empty, replenish it by allocating an extra block
	if (tail < numPages) 
		ExpandPhysicalMemory(pageCount + numPages - tail);

#ifdef _WIN32
	// grab the pages from the stack; Pop()
	memcpy(pagePtr, PFN + tail - numPages, numPages * sizeof(BlockType));
#else
	pagePtr = (char*) PFN + (tail - numPages) * pageSize;
#endif
	tail              -= numPages;
	minAvailableBlocks = min(tail, pageCount);

	// yield to other threads
	Syscall.LeaveCS(cs);

	// return the pagePtr (for Linux)
	return pagePtr;
}

// maps a physical block to virtual memory
void StreamPool::MapBlock(BufferConfig *bc, char* blockAddress, uint64_t numPages, BlockType* pages) {
#ifdef _WIN32
	// check for chunking
	uint64_t chunk = (blockAddress - bc->bufMain) / bc->chunkSize;
	if (chunk != 0) {
		if (bc->lockChunkTree) Syscall.EnterCS(bc->chunkCS);
		map<uint64_t, uint64_t>::iterator it = bc->chunkTree.find(chunk);
		if (it == bc->chunkTree.end()) {
			ConvertChunkToPhysical(bc, blockAddress);
			bc->chunkTree.insert({ chunk, 1 });
		}
		else it->second++;
		if (bc->lockChunkTree) Syscall.LeaveCS(bc->chunkCS);
	}
#endif
	// map the block
	Syscall.MapPages(blockAddress, numPages, pageSize, pages);
}

// unmaps a physical block from virtual memory
void StreamPool::UnmapBlock(BufferConfig* bc, char* blockAddress, uint64_t pages) {
	// unmap the block
	Syscall.UnmapPages(blockAddress, pages);
#ifdef _WIN32
	// we want to keep chunk 0 in-place
	uint64_t chunk      = (blockAddress - bc->bufMain) / bc->chunkSize;
	uint64_t chunkFault = (bc->lastFault - bc->bufMain) / bc->chunkSize;
	if (chunk > 0) {
		if (bc->lockChunkTree) Syscall.EnterCS(bc->chunkCS);
		map<uint64_t, uint64_t>::iterator it = bc->chunkTree.find(chunk);
		if (--it->second == 0 && chunk < chunkFault) {
			FreeChunk(bc, bc->bufMain + chunk * bc->chunkSize);
			bc->chunkTree.erase(it);
		}
		if (bc->lockChunkTree) Syscall.LeaveCS(bc->chunkCS);
	}
#endif
}

// removes a guard page
void StreamPool::RemoveGuard(char* blockAddress) {
	Syscall.RemoveGuard(blockAddress);
}

// sets up a guard page
void StreamPool::InstallGuard(char* blockAddress) {
	Syscall.InstallGuard(blockAddress);
}

// initializes the pool block parameters
void StreamPool::InitializePool(void) {
	// block size must be a multiple of page size
	pageSize      = 1LLU << pageSizePower;
	blockSize     = RoundUp(1LLU << blockSizePower, pageSize);
	pagesPerBlock = blockSize / pageSize;
}

#define MAX_COLORS		1024
// allocates the virtual memory and physical pages for blocks for Vortex
void StreamPool::BufferAlloc(uint64_t memoryRequired, uint64_t chunkSize, uint64_t color, BufferConfig* bc) {
	// must round up the memory *before* adding stagger
	uint64_t alignedMemoryRequired = RoundUp(memoryRequired + pageSize, blockSize);
	bc->reserveSize                = alignedMemoryRequired + pageSize * MAX_COLORS;

#ifdef _WIN32
	// make sure chunks are no larger than total space
	if (chunkSize == memoryRequired) bc->chunkSize = bc->reserveSize;			      
	else                             bc->chunkSize = min(chunkSize, bc->reserveSize); 

	// reserve size must be a multiple of chunk size
	bc->reserveSize = RoundUp(bc->reserveSize, bc->chunkSize);
#else
	// Only Windows requires chunking
	bc->chunkSize = bc->reserveSize;
#endif

	// protect against simultaneous stream starts, in which case the re-reserve loops fails in older OSes
	Syscall.EnterCS(cs);

	// one allocation for the entire space; no chunking
	if (bc->chunkSize == bc->reserveSize) 		   
		bc->bufMain = (char*) Syscall.AllocateVirtual(NULL, bc->reserveSize, MEM_RESERVE | MEM_PHYSICAL);
#ifdef _WIN32
	// chunked allocation
	else {
		bc->bufMain = (char*) Syscall.AllocateVirtual(NULL, bc->reserveSize, MEM_RESERVE);
		HANDLE heap = GetProcessHeap();
		HeapLock(heap);
		VirtualFree(bc->bufMain, 0, MEM_RELEASE);

		// re-reserve the space in chunks
		for (uint64_t i = 0; i < bc->reserveSize / bc->chunkSize; i++) {
			char* buf;
			if (i == 0) 
				buf = (char*)Syscall.AllocateVirtual(bc->bufMain + i * bc->chunkSize, 
					bc->chunkSize, MEM_PHYSICAL | MEM_RESERVE);
			else        
				buf = (char*)Syscall.AllocateVirtual(bc->bufMain + i * bc->chunkSize, 
					bc->chunkSize, MEM_RESERVE); 
		}
		HeapUnlock(heap);
	}
#endif
	Syscall.LeaveCS(cs);

	// aim to have page color equal to the one requested by the user
	int kernelColor = ((uint64_t)bc->bufMain >> 12) & (MAX_COLORS - 1);

	// correct pages for stepped first block size (e.g., 256, 255, 254,...,1)
	if (color == 0) colorShift = kernelColor;

	// coloring is needed for bucket sort on Sandy/Ivy Bridge with the 8+8+8 pattern
	bc->buf = bc->bufMain + ((color + colorShift - kernelColor) & (MAX_COLORS - 1)) * pageSize;
}


#ifdef _WIN32
// converts a chunk of virtual memory from MEM_RESERVE to MEM_RESERVE | MEM_PHYSICAL to allow page mapping
void StreamPool::ConvertChunkToPhysical(BufferConfig* bc, char* ptr) {
	// do nothing if chunking is not present
	if (bc->chunkSize == bc->reserveSize) return;

	// round down the pointer to chunk boundary
	uint64_t offset  = RoundDown (ptr - bc->bufMain, bc->chunkSize);
	char* alignedPtr = bc->bufMain + offset;

	// acquire the heap lock
	HANDLE heap = GetProcessHeap();
	HeapLock(heap);

	// free the space & re-reserve with MEM_PHYSICAL for page mapping
	Syscall.FreeVirtual(alignedPtr, bc->chunkSize);
	Syscall.AllocateVirtual(alignedPtr, bc->chunkSize, MEM_RESERVE | MEM_PHYSICAL);

	// yield the heap
	HeapUnlock(heap);
}

// frees the given chunk wherein ptr lies
void StreamPool::FreeChunk(BufferConfig* bc, char* ptr) {
	// acquire the heap lock
	HANDLE heap = GetProcessHeap();
	HeapLock(heap);

	// release this chunk & re-reserve the space
	Syscall.FreeVirtual(ptr, bc->chunkSize);
	Syscall.AllocateVirtual(ptr, bc->chunkSize, MEM_RESERVE);

	// yield the heap
	HeapUnlock(heap);
}

// frees all chunks back to the kernel
void StreamPool::BufferFree(BufferConfig* bc) {
	// discard the chunks; free all reserved chunks no matter their state
	for (uint64_t i = 0; i < bc->reserveSize / bc->chunkSize; i++) 
		Syscall.FreeVirtual(bc->bufMain + i * bc->chunkSize, 0);

	// clear the given chunk tree
	bc->chunkTree.clear();
}
#endif
