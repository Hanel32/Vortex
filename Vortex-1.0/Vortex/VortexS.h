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
class StreamPool;
class VortexS : public Stream {
	// base stream structures
	map<uint64_t, BlockState*> physicalBlockMapped;
	StreamPool*  sp;
	BufferConfig bc;

	// stream variables
	char*		 bufUser;
	char*	     lastReadFaultAddr;
	char*	     lastWriteFaultAddr;
	char*	     lastMapAddr;
	bool		 writeFlag;
	uint64_t     myID;
	uint64_t     diff;

	// internal fault handling
	void         HandleWriteFault(char* realFaultAddress, char* alignedFaultAddress);
	void         HandleReadFault(char* faultAddress, char* alignedFaultAddress);
public:
	VortexS(uint64_t memoryRequired, uint64_t chunkSize, StreamPool* sp, uint64_t myID);
	uint64_t     GetFirstBlockSize();
	bool		 ProcessFault(int faultType, char* faultAddress);
	char*		 GetReadBuf(void);
	char*		 GetWriteBuf(void);
	void		 Reset(void);
	void		 FinishedWrite(void) { /* this should never be called; dummy function for compatibility with Stream */ }
	~VortexS();
};

