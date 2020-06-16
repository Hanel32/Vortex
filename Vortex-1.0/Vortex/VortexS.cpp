/*------------------------------------------------------------------------------------------------
 -   Vortex: Extreme-Performance Memory Abstractions for Data-Intensive Streaming Applications   -
 -   Copyright(C) 2020 Carson Hanel, Arif Arman, Di Xiao, John Keech, Dmitri Loguinov            -
 -   Produced via research carried out by the Texas A&M Internet Research Lab                    -
 -																							     -
 -	 This program is free software : you can redistribute it and/or modify						 -
 -	 it under the terms of the GNU General Public License as published by						 -
 -	 the Free Software Foundation, either version 3 of the License, or							 -
 -	 (at your option) any later version.													     -
 -																								 -
 -	 This program is distributed in the hope that it will be useful,							 -
 -	 but WITHOUT ANY WARRANTY; without even the implied warranty of								 -
 -	 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the								 -
 -	 GNU General Public License for more details.												 -
 -																								 -
 -	 You should have received a copy of the GNU General Public License							 -
 -	 along with this program. If not, see < http://www.gnu.org/licenses/>.						 -
 ------------------------------------------------------------------------------------------------*/
#include "stdafx.h"

// initializes a VortexS stream
VortexS::VortexS(uint64_t memoryRequired, uint64_t chunkSize, StreamPool *sp, uint64_t myID) :
	sp(sp), myID(myID) {
	sp->BufferAlloc(memoryRequired, chunkSize, myID, &bc);
	streamManager.AddStream(bc.bufMain, bc.bufMain + bc.reserveSize, this);

	// cache-line stagger; this offset is always less than one page, so the guard is still triggered
	bufUser            = bc.buf + 64 * (myID & 63);
	diff               = (bufUser - bc.bufMain) / sp->blockSize;
	writeFlag          = false;
	lastReadFaultAddr  = 0;
	lastWriteFaultAddr = 0;
}

// deconstructs a VortexS stream
VortexS::~VortexS() {
	Reset();
	streamManager.RemoveStream(bc.bufMain);
#ifdef _WIN32
	// only used for chunking
	sp->BufferFree(&bc);
#endif
}

// retrieves the read buffer
char* VortexS::GetReadBuf(void) {
	return bufUser;
}

// retrieves the write buffer
char* VortexS::GetWriteBuf(void) {
	return bufUser;
}

// retrieve the number of pages in the first block
uint64_t VortexS::GetFirstBlockSize() {
#ifdef _WIN32
	// Windows can save memory on the first block by only mapping needed pages
	uint64_t pageSpacing = (bc.buf - bc.bufMain) >> sp->pageSizePower;
	uint64_t pagesNeeded = sp->pagesPerBlock - (pageSpacing & (sp->pagesPerBlock - 1));
	return   pagesNeeded;
#else
	// Linux requires blockSize aligned memory mappings
	return   sp->pagesPerBlock;
#endif
}

// handles stream write faults
void VortexS::HandleWriteFault(char* faultAddress, char* alignedFaultAddress) {
	// calculate the current block's index
	int64_t index = ((alignedFaultAddress - bc.bufMain) >> sp->blockSizePower) - diff;

	// added for PFN-based mapping
	uint64_t pagesNeeded, pageSpacing;
#ifdef _WIN32
	// Windows can save memory on the first block by only mapping needed pages
	if (index != 0) pageSpacing = (alignedFaultAddress - bc.bufMain) >> sp->pageSizePower;
	else            pageSpacing = (faultAddress - bc.bufMain) >> sp->pageSizePower;
#else
	// Linux requires blockSize aligned memory mappings
	pageSpacing = (alignedFaultAddress - bc.bufMain) >> sp->pageSizePower;
#endif
	if (index != 0) pagesNeeded = sp->pagesPerBlock;
	else            pagesNeeded = GetFirstBlockSize();
	char*           dest        = bc.bufMain + pageSpacing * sp->pageSize;

	// the block with the last read fault
	int64_t lastReadFaultBlock = ((lastReadFaultAddr - bc.bufMain) >> sp->blockSizePower) - diff;

	// set the write flag when installing a guard page immediately after the block with the last guard fault
	if (index == lastReadFaultBlock + 2) writeFlag = true;

	// if the current block is already being used & we hit its guard, put that page back
	map<uint64_t, BlockState*>::iterator prev = physicalBlockMapped.find(index - 1);
	if (prev != physicalBlockMapped.end()) 
		sp->InstallGuard(prev->second->virtualPtr);

	// if the current block is already being used & we hit its guard, put that page back
	map<uint64_t, BlockState*>::iterator cur = physicalBlockMapped.find(index);
	if (cur != physicalBlockMapped.end()) 
		sp->RemoveGuard(cur->second->virtualPtr);

	// otherwise, obtain a new block and map
	else {
		BlockState* pBlock = new (pagesNeeded) BlockState;
#ifdef _WIN32
		sp->GetNewBlock(pagesNeeded, (BlockType*)pBlock->GetPFN());
#else
		pBlock->SetPFN(sp->GetNewBlock(pagesNeeded, (BlockType*)pBlock->GetPFN()));
#endif
		// record block data and map the block
		sp->MapBlock(&bc, dest, pagesNeeded, (BlockType*)pBlock->GetPFN());
		lastMapAddr                = pBlock->virtualPtr = dest;
		pBlock->numPages           = pagesNeeded;
		physicalBlockMapped[index] = pBlock;
	}
}

// handles stream read faults
void VortexS::HandleReadFault(char* faultAddress, char* alignedFaultAddress) {
	// calculate the current block's index
	int64_t index = ((alignedFaultAddress - bc.bufMain) >> sp->blockSizePower) - diff;

	// if the most-recently-used block can be freed, do so
	bool aligned      = faultAddress == alignedFaultAddress;
	bool block_gap    = lastReadFaultAddr == faultAddress - sp->blockSize;
	bool second_block = lastReadFaultAddr == bufUser && faultAddress < lastReadFaultAddr + sp->blockSize;
	if (!writeFlag && (second_block || block_gap) && aligned) {
		map<uint64_t, BlockState*>::iterator it = physicalBlockMapped.find(index - 1);
		if (it != physicalBlockMapped.end()) {
			BlockState* block = it->second;
			sp->UnmapBlock(&bc, block->virtualPtr, block->numPages);
			sp->ReturnFreeBlock(block->numPages, (BlockType*)block->GetPFN());
			delete it->second;
			physicalBlockMapped.erase(it);
		}
	}

	// check if a block is mapped to the current fault location
	map<uint64_t, BlockState*>::iterator it = physicalBlockMapped.find(index);
	if (it == physicalBlockMapped.end())
		ReportError("trying to use an empty block %d in bucket %d\n", index, myID);

	// remove the guard page being faulted on
	BlockState* block = it->second;
	sp->RemoveGuard(block->virtualPtr);

	// set the write flag to false for freeing
	lastReadFaultAddr = faultAddress;
	writeFlag         = false;
}

// directs valid stream access violations
bool VortexS::ProcessFault(int faultType, char* faultAddress) {
	// align the fault address
	uint64_t offset = faultAddress - bc.bufMain;
	offset = (offset >> sp->blockSizePower) << sp->blockSizePower;
	char* alignedFaultAddress = bc.bufMain + offset;
	bc.lastFault = alignedFaultAddress;

	// process the fault
	if (faultType == EXCEPTION_WRITE_FAULT)
		HandleWriteFault(faultAddress, alignedFaultAddress);
	else if (faultType == EXCEPTION_READ_FAULT)
		HandleReadFault(faultAddress, alignedFaultAddress);
	else
		ReportError("invalid fault address %p, alignment puts it out of boundaries\n", faultAddress);
	return true;
}

// resets the stream to the beginning state
void VortexS::Reset(void) {
	// scan for still-mapped blocks 
	map<uint64_t, BlockState*>::iterator it = physicalBlockMapped.begin();
	while (it != physicalBlockMapped.end()) {
		if (it->second != NULL) {
			BlockState* pBlock = (BlockState*)it->second;
#ifdef __linux__
			// neeeded to remove any remaining guard pages - Windows does this implicitly
			Syscall.RemoveGuard(pBlock->virtualPtr);
#endif
			sp->UnmapBlock(&bc, pBlock->virtualPtr, pBlock->numPages);
			sp->ReturnFreeBlock(pBlock->numPages, (BlockType*)pBlock->GetPFN());
			delete it->second;
		}
		it++;
	}
	physicalBlockMapped.clear();
	writeFlag         = false;
	lastReadFaultAddr = 0;
}



