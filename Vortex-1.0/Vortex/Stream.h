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

// stream buffer configuration
class BufferConfig {
public:
	CSType* chunkCS;
	uint64_t reserveSize, chunkSize;
	map<uint64_t, uint64_t>	chunkTree;
	char* lastFault;
	char* bufMain;
	char* buf;
	bool lockChunkTree;
	BufferConfig(bool lock) {
	    lockChunkTree = lock;
	    if (lockChunkTree) chunkCS = (CSType*)Syscall.MakeCS();
	}
	~BufferConfig() {
	    if (lockChunkTree) Syscall.DeleteCS(chunkCS);
	}
};

// stream block wrapper
class BlockState {
public:
	char*      virtualPtr = nullptr;
	uint64_t   numPages   = 0;
#ifdef _WIN32
	void*      GetPFN(void) { return (void*)(this + 1); }
#else
	void*      GetPFN(void) { return virtualPtr; }
	void       SetPFN(char* ptr) { virtualPtr = ptr; }
#endif
	void* operator new (size_t len, uint64_t pagesNeeded) {
		return malloc(len + pagesNeeded * sizeof(char*));
	}
	void operator delete(void *p) { free(p); }
};

// pure virtual base class of Stream 
class Stream {
public:
	virtual bool  ProcessFault(int faultType, char* faultAddress) = 0;			// entry point from the exception handler 
	virtual char* GetReadBuf(void)                                = 0;
	virtual char* GetWriteBuf(void)                               = 0;
	virtual void  Reset(void)                                     = 0;
	virtual void  FinishedWrite(void)                             = 0;
	virtual      ~Stream() {}
};


