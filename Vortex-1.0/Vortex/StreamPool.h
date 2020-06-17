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
#pragma once
#include "SystemFunctions.h"

class StreamPool {
	BlockType* PFN;
	uint64_t   pageCount;
	uint64_t   tail;
	uint64_t   colorShift;
	void	   ExpandPhysicalMemory(uint64_t totalPageCount);
public:
	CSType*    cs;
	uint64_t   blockSize, pagesPerBlock, pageSize;
	uint64_t   blockSizePower, pageSizePower;
	int64_t	   minAvailableBlocks;	
	uint64_t   blockCount;

	StreamPool(uint64_t blockSizePower);
	~StreamPool();

	void	   AdjustPoolPhysicalMemory(uint64_t totalPageCount);
	void	   Reset(void);

	uint64_t   CountFreeBlocks() { return tail; }
	BlockType* GetNewBlock(uint64_t pages, BlockType* pagePtr);
	void	   ReturnFreeBlock(uint64_t pages, BlockType* pagePtr);

	void	   MapBlock(BufferConfig *bc, char* virtualPtr, uint64_t numPages, BlockType* PFN);
	void	   UnmapBlock(BufferConfig* bc, char* blockAddress, uint64_t pages);
	void	   RemoveGuard(char* blockAddress);
	void	   InstallGuard(char* blockAddress);
	  
	void	   InitializePool(void);
	void	   BufferAlloc(uint64_t memoryRequired, uint64_t chunkSize, uint64_t color, BufferConfig* bc);

#ifdef _WIN32
	// chunking is only needed on Windows
	void	   ConvertChunkToPhysical(BufferConfig* bc, char* ptr);
	void	   FreeChunk(BufferConfig* bc, char* ptr);
	void	   BufferFree(BufferConfig* bc);
#endif
};
