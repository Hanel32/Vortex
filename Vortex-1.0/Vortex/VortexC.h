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
#pragma once
#include "SystemFunctions.h"
#include <inttypes.h>

class StreamPool;
class VortexC : public Stream {
	// base stream structures
	map<uint64_t, BlockState*> blockState;
	BufferConfig bcReader, bcWriter;
	StreamPool*  sp;

	// stream variables
	uint64_t	 size;			   // actual stream size; can be smaller than bc.reserveSize
	uint64_t     comeBackProducer; // M - max distance in blocks from which the producer can come back
	uint64_t	 comeBackConsumer; // L - max distance in blocks from which the consumer can come back
	int64_t		 curReadOff;
	char*		 writeBuf;
	char*		 readBuf;

	// producer consumer semaphores  
	SemaType*	 semFull;
	SemaType*	 semEmpty;
	char*		 lastWriterPosition;
	CSType*      cs;

	// internal fault handling
	void		 HandleWriteFault(char* faultAddress, char* alignedFaultAddress);
	void		 HandleReadFault(char* alignedFaultAddress);
	bool		 IsReaderAddress(char* addr);
public:
	VortexC(uint64_t size, uint64_t blockSizePower, uint64_t comeBackConsumer, uint64_t comeBackProducer, uint64_t writeAhead);
	bool		 ProcessFault(int faultType, char* faultAddress);
	void		 FinishedWrite(void);
	void		 FinishedRead(void);
	char*		 GetReadBuf(void);
	char*		 GetWriteBuf(void);
	void		 Reset(void);		  

	uint64_t	 GetProducerComeback (void) { return comeBackProducer;  }
	uint64_t	 GetConsumerComeback (void) { return comeBackConsumer;  }
	uint64_t	 GetBlockSize(void);
	uint64_t	 GetSize(void);
	~VortexC();
};
