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
 
// write-combine cacheline size
#define CACHE_LINE_BITS	6
#define CACHE_LINE	   (1 << CACHE_LINE_BITS)

template <typename ItemType>
class VortexSort {
	// checks for AVX and set up const key variables
	static const int keyBits    = sizeof(ItemType) * 8;
	static const int itemStride = (1LLU << 14) / keyBits;

	// stream internals
	SortingNetwork<ItemType> sn;
	vector<VortexS*>    streams;
	VortexCpuId           cpuId;
	ItemType*	         output;
	uint64_t           byteSize;
	uint64_t           maxDepth;
	uint64_t		  chunkSize;
	uint64_t	  mask[keyBits];
	int    bucketPower[keyBits];

	// bucket pointers
	ItemType** buckets;
	ItemType** p1_buckets;			

	// write-combine variables
	ItemType* tmpBuckets;
	short*	  tmpBucketSize;
public: 
	StreamPool* sp;
	uint64_t	nBuckets[keyBits];
	VortexSort(uint64_t size, uint64_t blockSizePower);
	void		InitializeRAM(uint64_t BucketsL0, uint64_t bucketsL1);
	void		Sort(ItemType* inputBuf, ItemType* outputBuf, uint64_t itemsToSort);
	void		SplitInput(ItemType* buf, uint64_t size);
	void		BeginRecursion(ItemType* output);
	void		RecursiveSort(ItemType* buf, uint64_t size, int shift, uint64_t off, int level);
	void        Copy(ItemType* dst, ItemType* src, uint64_t size);
	void		Reset(void);
	~VortexSort();
};