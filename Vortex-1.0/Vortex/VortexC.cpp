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

// initializes a VortexC stream
VortexC::VortexC(uint64_t size, uint64_t blockSizePower, uint64_t comeBackConsumer, uint64_t comeBackProducer, uint64_t writeAhead) {
	// setup stream parameters
	this->comeBackConsumer = comeBackConsumer;		// M
	this->comeBackProducer = comeBackProducer;		// L
	this->size             = size;                  // raw input size
	curReadOff             = -1;

	// setup semaphores
	uint64_t fwd = writeAhead + comeBackProducer;
	semEmpty     = (SemaType*) Syscall.MakeSemaphore(fwd, fwd + 1);
	semFull      = (SemaType*) Syscall.MakeSemaphore(0, fwd + 1);

	// create the stream pool
	sp = new StreamPool(blockSizePower);
	sp->AdjustPoolPhysicalMemory((comeBackConsumer + comeBackProducer + writeAhead + 1) * sp->pagesPerBlock);		// M+L+N+1 blocks

	// allocate virtual memory
	bcReader = new BufferConfig(false);
	bcWriter = new BufferConfig(true);
	sp->BufferAlloc(size, size, 0, bcReader);
	sp->BufferAlloc(size, size, 0, bcWriter);
	readBuf  = bcReader->bufMain;
	writeBuf = bcWriter->bufMain;
	lastWriterPosition = 0;

	// add the streams to the handler
	streamManager.AddStream(bcReader->bufMain, bcReader->bufMain + bcReader->reserveSize, this);
	streamManager.AddStream(bcWriter->bufMain, bcWriter->bufMain + bcWriter->reserveSize, this);
	printf("Vortex-C with %lld GB, chunk %.1f GB, block %llu KB, page %llu KB, M %llu, L %llu, N %llu\n",
		bcReader->reserveSize / (1 << 30), (double)bcReader->chunkSize / (1 << 30),
		sp->blockSize / 1024, sp->pageSize / 1024, comeBackConsumer, comeBackProducer, writeAhead);
	cs = (CSType*)Syscall.MakeCS();
}

// deconstructs a VortexC stream
VortexC::~VortexC() {
	Reset();
	streamManager.RemoveStream(bcReader->bufMain);
	streamManager.RemoveStream(bcWriter->bufMain);
#ifdef _WIN32
	// only needed for chunking
	sp->BufferFree(bcReader);
	sp->BufferFree(bcWriter);
#endif
	delete sp;
	delete bcReader;
	delete bcWriter;
	Syscall.DeleteCS(cs);
}

// handles stream write faults
void VortexC::HandleWriteFault(char* faultAddress, char* alignedFaultAddress) {
	uint64_t index = (alignedFaultAddress - writeBuf) >> sp->blockSizePower;

	//printf("Vortex-C write fault in block %lld @ %p\n", index, faultAddress);

	// detect if writer goes back
	if (lastWriterPosition != NULL && faultAddress < lastWriterPosition - (comeBackProducer << sp->blockSizePower))
		ReportError("producer violating comeback L; lastFault %llu, currentFault %p\n", lastWriterPosition, faultAddress);

	lastWriterPosition = faultAddress;
	// added for PFN-based mapping
	uint64_t pagesNeeded, pageSpacing = (faultAddress - writeBuf) >> sp->pageSizePower;
	if (index != 0)       pagesNeeded = sp->pagesPerBlock;
	else                  pagesNeeded = sp->pagesPerBlock - (pageSpacing & (sp->pagesPerBlock - 1));

	// release the next full block
	int64_t writerReleaseOffset = index - (comeBackProducer + 1);
	if (writerReleaseOffset >= 0) 
		Syscall.FreeSemaphore(semFull, 1);
	
	// wait for the next empty block
	Syscall.WaitSemaphore(semEmpty);

	// create a new block allocation
	BlockState* pBlock = new (pagesNeeded) BlockState;
#ifdef _WIN32
	sp->GetNewBlock(pagesNeeded, (BlockType*)pBlock->GetPFN());
#else
	pBlock->SetPFN(sp->GetNewBlock(pagesNeeded, (BlockType*)pBlock->GetPFN()));
#endif
	// record block data and map the block.
	sp->MapBlock(bcWriter, alignedFaultAddress, pagesNeeded, (BlockType*)pBlock->GetPFN());
	pBlock->virtualPtr = alignedFaultAddress;
	pBlock->numPages = pagesNeeded;

	Syscall.EnterCS(cs);
	blockState[index]  = pBlock;
	Syscall.LeaveCS(cs);
}

// handles stream read faults
void VortexC::HandleReadFault(char* alignedFaultAddress) {
	int64_t index = (alignedFaultAddress - readBuf) >> sp->blockSizePower;
	
	while (curReadOff < index) 
	{
		curReadOff++;

		// unmap empty blocks below the lower end of consumer window; must be prior to releasing semEmpty
		int64_t emptyUnmapOffset = curReadOff - (comeBackConsumer + 1);
		if (emptyUnmapOffset >= 0) 
		{
			Syscall.EnterCS(cs);
			map<uint64_t, BlockState*>::iterator it = blockState.find(emptyUnmapOffset);
			if (it == blockState.end())
				ReportError("unmapping an empty block %lld\n", emptyUnmapOffset);		
			BlockState* block = it->second;
			blockState.erase(it);
			Syscall.LeaveCS(cs);

			// unmap the block from where it is currently mapped for reuse

			bool isReader = IsReaderAddress(block->virtualPtr);
			BufferConfig* bc = isReader ? bcReader : bcWriter;
			sp->UnmapBlock(bc, block->virtualPtr, block->numPages);
			sp->ReturnFreeBlock(block->numPages, (BlockType*)block->GetPFN());
			delete block;
		}

		// release the semaphore, alerting the Producer to the available block
		Syscall.FreeSemaphore(semEmpty, 1);

		// wait for writer to provide the next block
		Syscall.WaitSemaphore(semFull);

		// if we're within M blocks of the current read fault, start mapping the blocks
		if (curReadOff + (int64_t) comeBackConsumer >= index) 
		{
			Syscall.EnterCS(cs);
			map<uint64_t, BlockState*>::iterator it = blockState.find(curReadOff);
			if (it == blockState.end())
				ReportError("mapping down empty block %lld\n", curReadOff);			
			Syscall.LeaveCS(cs);

			BlockState* block = it->second;
			
			// unmap from the writer buffer
			sp->UnmapBlock(bcWriter, block->virtualPtr, block->numPages);

			// map to reader buffer
			sp->MapBlock(bcReader, readBuf + curReadOff * sp->blockSize, block->numPages, (BlockType*)block->GetPFN());
			block->virtualPtr = readBuf + curReadOff * sp->blockSize;
			//printf("	mapped down block %lld\n", curReadOff);
		}
	}
}

// directs valid stream access violations
bool VortexC::ProcessFault(int faultType, char* faultAddress) {
	// write fault
	if (faultType == EXCEPTION_WRITE_FAULT) {
		uint64_t offset           = faultAddress - writeBuf;
		offset                    = (offset >> sp->blockSizePower) << sp->blockSizePower;
		char* alignedFaultAddress = writeBuf + offset;
		bcWriter->lastFault       = alignedFaultAddress;
		if (offset >= 0 && offset < bcWriter->reserveSize)
			HandleWriteFault(faultAddress, alignedFaultAddress);
		else
			ReportError("Write offset %lld, while the valid range is [%lld, %lld]\n", offset, 0, bcWriter->reserveSize);
	}
	// read fault
	else if (faultType == EXCEPTION_READ_FAULT) {
		uint64_t offset           = faultAddress - readBuf;
		offset                    = (offset >> sp->blockSizePower) << sp->blockSizePower;
		char* alignedFaultAddress = readBuf + offset;
		bcReader->lastFault       = alignedFaultAddress;
		if (offset >= 0 && offset < bcReader->reserveSize)
			HandleReadFault(alignedFaultAddress);
		else
			ReportError("Read offset %lld, while the valid range is [%lld, %lld]\n", offset, 0, bcReader->reserveSize);
	}
	else
		ReportError("invalid fault address %p, alignment puts it out of boundaries\n", faultAddress);
	return true;
}

// resets the stream to the beginning state
void VortexC::Reset(void) {
	// scan for still-mapped blocks
	Syscall.EnterCS(cs);
	map<uint64_t, BlockState*>::iterator it = blockState.begin();
	while (it != blockState.end()) {
		BlockState* block = it->second;
		it = blockState.erase(it);		// C++11
		Syscall.LeaveCS(cs);

		bool isReader     = IsReaderAddress(block->virtualPtr);
		BufferConfig* bc  = isReader ? bcReader : bcWriter;
		sp->UnmapBlock(bc, block->virtualPtr, block->numPages);
		sp->ReturnFreeBlock(block->numPages, (BlockType*)block->GetPFN());
		delete block;

		Syscall.EnterCS(cs);
	}
	Syscall.LeaveCS(cs);
	curReadOff = -1;
}

// releases the last blocks to the consumer
void VortexC::FinishedWrite(void) {
	// Release L+1 still-pending buffers.
	Syscall.FreeSemaphore(semFull, comeBackProducer + 1);
}

// attempts to read the last byte of the stream
void VortexC::FinishedRead(void) {
#ifdef _WIN32
#pragma warning( push )
#pragma warning(suppress: 4189)
	volatile char a = readBuf[size - 1];
#pragma warning( pop ) 
#else
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
	volatile char a = readBuf[size - 1];
#pragma GCC diagnostic pop
#endif
}

// retrieves the read buffer
char* VortexC::GetReadBuf(void) {
	return readBuf;
}

// retrieves the write buffer
char* VortexC::GetWriteBuf(void) {
	return writeBuf;
}

// retrieves stream byte size
uint64_t VortexC::GetSize(void) {
	return size;
}

// retrieves the size of stream blocks
uint64_t VortexC::GetBlockSize(void) {
	return sp->blockSize;
}

// checks if a pointer is pointing to the read buffer
bool VortexC::IsReaderAddress(char* addr) {
	return addr >= bcReader->bufMain && addr < bcReader->bufMain + bcReader->reserveSize;
}
