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

// explicit template instantiation
template class VortexSort<uint64_t>;
template class VortexSort<uint32_t>;
template class VortexSort<uint16_t>;
template class VortexSort<uint8_t >;

// resets the StreamPool and each individual VortexS stream
template <typename ItemType>
void VortexSort<ItemType>::Reset(void) {
	// reset buckets to beginning so that another iteration can be started
	memcpy(p1_buckets, buckets, sizeof(ItemType*) * nBuckets[0]);
	memset(tmpBucketSize, 0, sizeof(tmpBucketSize[0]) * nBuckets[0]);

	// perform StreamPool/VortexS resets
	for (uint64_t i = 0; i < nBuckets[0]; i++)
		streams[i]->Reset();
	sp->Reset();
}

// deletes the streamPool and each individual VortexS stream
template <typename ItemType>
VortexSort<ItemType>::~VortexSort() {
	for (uint64_t i = 0; i < nBuckets[0]; i++) delete streams[i];
	Syscall.DeallocAligned(tmpBucketSize);
	Syscall.DeallocAligned(tmpBuckets);
	Syscall.DeallocAligned(buckets);
	delete sp;
}

// prepares the Vortex sort - allocates stream buckets and memory
template <typename ItemType>
VortexSort<ItemType>::VortexSort(uint64_t size, uint64_t blockSizePower) {
	// calculate input size as a power of two, and designate maximum b
	int      maxPower        = 8;
	int      inputPower      = Syscall.BitScan(size);
	uint64_t roundedSizeDown = 1LLU << inputPower;
	if ((double)size > 4.0 / 3 * (double)roundedSizeDown) 
		inputPower++;

	// check if we're using small keys w.r.t input
	bool smallKeys           = keyBits < inputPower;
	int  idealPowerLastLevel = smallKeys ? 0 : 3;
	int  splitPower          = smallKeys ? keyBits : inputPower;

	// setup the split parameters
	uint64_t depth    = (int)ceil((double)(splitPower - idealPowerLastLevel) / maxPower);
	uint64_t base     = (splitPower - idealPowerLastLevel) / depth;
	uint64_t leftover = (splitPower - idealPowerLastLevel) % depth;
	byteSize          = size * sizeof(ItemType);

	// first set up the uniform case, where it matters
	uint64_t bitSum = 0;
	for (uint64_t i = 0; i < depth; i++) {
		bucketPower[i] = i < leftover ? (int)base + 1 : (int)base;
		bitSum += bucketPower[i];
	}

	// fill in the rest with the default 8-bit splits
	maxDepth = depth;
	while (bitSum <= keyBits) {
		bucketPower[maxDepth] = bucketPower[depth - 1];
		bitSum += bucketPower[maxDepth++];
	}

	// report sort setup information
	chunkSize = 1LLU << 27;
	printf("Sorting %.2f GB, maxPower %d, block %llu KB, chunk %lld MB, maxDepth %lld, splits = (",
		(double)byteSize / (1 << 30), maxPower, 1LLU << (blockSizePower - 10), chunkSize >> 20, maxDepth);
	for (uint64_t i = 0; i < maxDepth; i++) {
		nBuckets[i] = 1LLU << bucketPower[i];
		mask[i]     = nBuckets[i] - 1;
		if (i < depth) printf(" %d ", bucketPower[i]);
	}
	printf(")\n");

	// setup bucket pointers and a stream pool for memory management
	buckets = (ItemType**)Syscall.AllocAligned(sizeof(ItemType*) * nBuckets[0] * maxDepth, 64);
	sp      = new StreamPool(blockSizePower);

	// setup RAM necessary for stream pool
	InitializeRAM(max((int)nBuckets[0], 32), max((int)nBuckets[1], 32));
}

// allocates base RAM necessary, as well as estimated sort overhead to avoid runtime allocation
template <typename ItemType>
void VortexSort<ItemType>::InitializeRAM(uint64_t BucketsL0, uint64_t BucketsL1) {
	// setup L0 & L1 size estimated
	uint64_t bytesPerBucketL0   = (uint64_t)byteSize / BucketsL0;
	uint64_t bytesPerBucketL1   = (uint64_t)bytesPerBucketL0 / (uint64_t)((double)(BucketsL1 * 1.05));
	uint64_t totalBytesNeededL1 = 0, stuck = 0;

	// total block count across all streams = size + leaked blocks during one iteration
	uint64_t bucketReservedMemory = maxDepth * byteSize;
	for (uint64_t i = 0; i < BucketsL0; i++) {
		VortexS* s = new VortexS(bucketReservedMemory, chunkSize, sp, i);
		buckets[i] = (ItemType*)s->GetReadBuf();
		streams.push_back(s);

		// decide on the amount of allocated blocks
		uint64_t x_i = i < nBuckets[0] ? s->GetFirstBlockSize() : 0;
		if (i < BucketsL1)
			totalBytesNeededL1 += x_i * sp->pageSize + RoundUp(bytesPerBucketL1 - x_i * sp->pageSize, sp->blockSize);
		else
			stuck += x_i * sp->pageSize + RoundUp(bytesPerBucketL0 - x_i * sp->pageSize, sp->blockSize);
	}

	// allocate sort memory
	double   ratio          = (double)BucketsL1 / (double)BucketsL0;
	uint64_t pagesNeededNew = (uint64_t)((double)byteSize * ratio + (double)totalBytesNeededL1 + (double)stuck) / sp->pageSize + (4 * sp->pagesPerBlock);
	sp->AdjustPoolPhysicalMemory(pagesNeededNew);
	
	// setup write-combine memory
	p1_buckets    = buckets + nBuckets[0];
	tmpBucketSize = (short*)Syscall.AllocAligned(sizeof(short) * nBuckets[0], 64);
	tmpBuckets    = (ItemType*)Syscall.AllocAligned(sizeof(ItemType) * nBuckets[0] * CACHE_LINE, 64);
	memcpy(p1_buckets, buckets, sizeof(ItemType*) * nBuckets[0]);
	memset(tmpBucketSize, 0, sizeof(tmpBucketSize[0]) * nBuckets[0]);
}

// performs the Vortex radix sort
template <typename ItemType>
void VortexSort<ItemType>::Sort(ItemType* inputBuf, ItemType* outputBuf, uint64_t itemsToSort) {
	// handle the case with few input keys
	if (itemsToSort <= 128) {
		if (itemsToSort > 32) 
			sn.insertionSort2(inputBuf, (int)itemsToSort);
		else 
			(*sn.p[itemsToSort])(inputBuf);
	}

	// split L0 of the input into the Vortex buffers
	SplitInput(inputBuf, itemsToSort);

	// recurse through each bucket in MSD fashion
	BeginRecursion(outputBuf);

	// reset the buckets, freeing all mapped blocks
	Reset();
}

// initially split input across stream buckets
template <typename ItemType>
void VortexSort<ItemType>::SplitInput(ItemType* buf, uint64_t size) {
	uint64_t   shift        = keyBits - bucketPower[0];
	short*     localSize    = tmpBucketSize;
	ItemType** localp1      = p1_buckets;
	ItemType*  localBuckets = tmpBuckets;
	uint64_t   avx_div      = sizeof(__m256i) / sizeof(ItemType);
	uint64_t   sse_div      = sizeof(__m128i) / sizeof(ItemType);

	// split each input item into the stream buffers
	for (uint64_t i = 0; i < size; i++) {
		// prefetch the input data
		_mm_prefetch((char*)(buf + i) + 2048, _MM_HINT_T2);

		// split the key into the write combine buffer
		ItemType  key   = buf[i];
		ItemType  buck  = ItemType(key >> shift);
		short     off   = localSize[buck];
		ItemType* p     = localBuckets + (buck << CACHE_LINE_BITS);
		ItemType* q     = p + off;
		localSize[buck] = short(off + 1);
		*q              = key;

		// if this bucket's temporary buffer is full, dump the cache line
		if (off == CACHE_LINE - 1) {
			// avx dump
			if (cpuId.avx) {
				__m256i* src = (__m256i*)p, *end = src + CACHE_LINE / avx_div,
					*dest = (__m256i*) localp1[buck];
				while (src < end) {
					__m256i x = _mm256_loadu_si256(src++);
					_mm256_stream_si256(dest++, x);
					__m256i y = _mm256_loadu_si256(src++);
					_mm256_stream_si256(dest++, y);
				}
				localp1[buck] = (ItemType*)dest;
			}
			// sse dump
			else {
				__m128i* src = (__m128i*)p, *end = src + CACHE_LINE / sse_div,
					*dest = (__m128i*) localp1[buck];
				while (src < end) {
					__m128i x = _mm_loadu_si128(src++);
					_mm_stream_si128(dest++, x);
					__m128i y = _mm_loadu_si128(src++);
					_mm_stream_si128(dest++, y);
				}
				localp1[buck] = (ItemType*)dest;
			}
			// bucket size reset
			localSize[buck] = 0;
		}
	}
}

// sort recursion base after the first level split
template <typename ItemType>
void VortexSort<ItemType>::BeginRecursion(ItemType* buf) {
	this->output = buf;
	ItemType** p = buckets;
	int shift    = (int)(keyBits - bucketPower[0] - bucketPower[1]);

	// recursively sort each bucket in proper MSD order
	for (uint64_t j = 0; j < nBuckets[0]; j++) {
		// dump leftover items in the write-combine buffer
		int leftover  = tmpBucketSize[j];
		ItemType* src = tmpBuckets + (j << CACHE_LINE_BITS);
		ItemType* tmp = p1_buckets[j];
		Copy(tmp, src, leftover);
		p1_buckets[j] += leftover;

		// check if this bucket requires further split levels
		int64_t sizeNext = p1_buckets[j] - p[j];
		if (sizeNext > 32 && keyBits > bucketPower[0]) {
			// begin recursion on bucket j
			RecursiveSort(p[j], sizeNext, shift, nBuckets[0], 1);
		}
		else {
			// sorting network if bits left to sort
			if (keyBits > bucketPower[0]) (*sn.p[sizeNext])(p[j]);

			// output the sorted items; this also triggers RAM decommit
			Copy(output, p[j], sizeNext);
			output += sizeNext;
		}

		// reset the just-split stream
		streams[j]->Reset();

		// start writing to bucket j from the beginning
		p1_buckets[j] = buckets[j];
	}
}

// recursively sort/split buckets
template <typename ItemType>
void VortexSort<ItemType>::RecursiveSort(ItemType* buf, uint64_t size, int shift, uint64_t off, int level) {
	// setup split mask and this level's bucket pointers
	uint64_t   localMask = mask[level];
	ItemType** p         = buckets + off;
	ItemType** pNext     = p + nBuckets[0];
	memcpy(pNext, p, sizeof(ItemType*) * nBuckets[0]);

	// assure that we don't shift beyond key boundaries
	if (shift < 0) shift = 0;

	// split the input bucket across nBuckets[level] buckets
	for (uint64_t i = 0; i < size; i++) {
		_mm_prefetch((char*)(buf + i) + 2048, _MM_HINT_T2);
		ItemType key  = buf[i];
		uint64_t buck = (key >> shift) & localMask;
		*pNext[buck] ++ = key;
	}

	// if there are key bits left to sort
	if (shift > 0) {
		// handle each bucket that was just produced in proper MSD order
		for (uint64_t j = 0; j < nBuckets[level]; j++) {
			uint64_t sizeNext = pNext[j] - p[j];
			if (sizeNext > 32) {
				// next level of recursion
				RecursiveSort(p[j], sizeNext, shift - (int)bucketPower[level + 1], off + nBuckets[0], level + 1);
			}
			else {
				// sorting network
				(*sn.p[sizeNext])(p[j]);

				// consume the items; this also triggers RAM decommit
				Copy(output, p[j], sizeNext);
				output += sizeNext;
			}
		}
	}
	// otherwise, dump all sorted buckets to output
	else {
		// end of recursion, each bucket contains copies of identical keys
		for (uint64_t j = 0; j < nBuckets[level]; j++) {
			uint64_t sizeNext = pNext[j] - p[j];

			// skip empty buckets
			if (sizeNext == 0) continue;

			// consume the items; this also triggers RAM decommit
			Copy(output, p[j], sizeNext);
			output += sizeNext;
		}
	}
}

// in-order temporal memcpy()
template <typename ItemType>
void __forceinline VortexSort<ItemType>::Copy(ItemType* dst, ItemType* src, uint64_t size) {
	for (uint64_t i = 0; i < size; i++) dst[i] = src[i];
}