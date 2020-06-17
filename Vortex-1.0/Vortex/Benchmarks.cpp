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

// explicit template instantiation
template class Benchmark<uint64_t>;
template class Benchmark<uint32_t>;
template class Benchmark<uint16_t>;
template class Benchmark<uint8_t >;

// uniformly random linear congruential generator producer
template <typename ItemType>
__forceinline void WriterLCG(ItemType* p, uint64_t len, uint64_t& x, uint64_t& y, uint64_t& z, uint64_t& w) {
	// LCG additive from https://en.wikipedia.org/wiki/Linear_congruential_generator
	uint64_t a     = 6364136223846793005;
	uint64_t c     = 1442695040888963407;
	uint64_t upper = 64 - sizeof(ItemType) * 8;
	for (uint64_t i = 0; i < len; i += 4) {
		x        = x * a + c;
		y        = y * a + c;
		z        = z * a + c;
		w        = w * a + c;
 		p[i]     = (ItemType)(x >> upper);
		p[i + 1] = (ItemType)(y >> upper);
		p[i + 2] = (ItemType)(z >> upper);
		p[i + 3] = (ItemType)(w >> upper);
	}
}

// constant data producer
template <typename ItemType>
__forceinline void Writer(__m128i* p, uint64_t len) {
	__m128i x = _mm_set1_epi64x(32);
	for (uint64_t i = 0; i < len; i++) 
		_mm_store_si128(p + i, x);
}

// summation consumer
template <typename ItemType>
__forceinline __m128i Reader(__m128i* __restrict p, uint64_t len, __m128i sum) {
	__m128i localSum = sum;
	for (uint64_t i = 0; i < len; i++) {
		__m128i x = _mm_load_si128(p + i);
		localSum  = _mm_add_epi64(localSum, x);
	}
	return localSum;
}

// producer and consumer speed printout 
template <typename ItemType>
void RunLoop(char* ptr, uint64_t size, int type, bool print) {
	// setup pointers and variables for reads or writes
	__m128i* p   = (__m128i*)ptr;
	__m128i  sum = _mm_set1_epi64x(0);
	uint64_t x   = (uint64_t) 1e4, y = (uint64_t) 1e12, z = (uint64_t) 1e18, w = 3;

	// setup timer
	SpeedReporter sr;
	sr.Start(0, size);

	// processes the data in chunks
	uint64_t chunk = print ? 1LLU << 27 : size;
	uint64_t total = 0;
	while (total < size) {
		// process the input in chunks
		uint64_t remainder = min(chunk, size - total);
		uint64_t len       = remainder / sizeof(*p);
		total             += remainder;

		// perform given read/write
		void* start = Syscall.StartTimer();
		if (type == READER_SUM)
			sum = Reader<ItemType>(p, len, sum);
		else if (type == WRITER_CONSTANT)
			Writer<ItemType>(p, len);
		else if (type == WRITER_LCG)
			WriterLCG<ItemType>((ItemType*)p, len * sizeof(*p) / sizeof(ItemType), x, y, z, w);
		double delay = Syscall.EndTimer(start);

		// adjust delay to achieve one-second intervals
		if (delay > 1.6)     chunk >>= 1;
		else if (delay < 0.7)chunk <<= 1;

		// output progress & update the working pointer
		if (print) sr.Report(total);
		p += len;
	}

	// prevents the compiler from optimizing out the reader
	if (type == READER_SUM) {
#ifdef _WIN32
		if (sum.m128i_i64[0] == 24455)
			printf("sum = %llX\n", sum.m128i_i64[0]);
#else
		if (sum[0] == 24455)
			printf("sum = %llX\n", sum[0]);
#endif
	}

	// output the final speed
	if (print) sr.FinalReport();
}

// checks that the sort results in sorted data
template <typename ItemType>
void ConsumerChecker(ItemType* p, uint64_t len) {
	uint64_t failed = 0, prev = p[0];
	for (uint64_t j = 1; j < len; j++) {
		uint64_t cur = p[j];

		// check for out-of-order
		if (prev > cur) failed++;
		prev = cur;
	}
	printf("\tSorted Result: unsorted keys = %lld, processed keys = %lld\n", failed, len);
}

// commandline parameter usage printout
template <typename ItemType>
void Benchmark<ItemType>::Usage(void) {
	printf("\nVortex Usage: \n");
#ifdef _WIN32
	printf("	Vortex file                 <------ file read\n");
	printf("	Vortex file <GB>            <------ file write\n");
	printf("	Vortex /c file1 file2	    <------ file copy\n");
	printf("	Vortex /s <GB> iterations   <------ sort\n");
	printf("	Vortex /p <GB>              <------ producer-consumer\n");
#else
	printf("	./Vortex /s <GB> iterations <------ sort\n");
	printf("	./Vortex /p <GB>            <------ producer-consumer\n");
#endif
	exit(0);
}

// --------------------------------------------------------------- //
//                     Vortex Benchmark Module                     // 
// --------------------------------------------------------------- //
template <typename ItemType>
void Benchmark<ItemType>::Run(int argc, char** argv) {
	int type = -1;
	if (argc > 4 || argc < 2) Usage();
	if (argv[1][0] == '/') {
		if (argv[1][1] == 'p' && argc == 3)
			type = 0;
#ifdef _WIN32
		// disk copy benchmark not available on Linux
		else if (argv[1][1] == 'c' && argc == 3)
			type = 3;
#endif
		else if (argv[1][1] == 's' && argc == 4)
			type = 4;
		else
			Usage();
	}
#ifdef _WIN32
	else type = (argc == 2) ? 1 : 2;
#else
	// disk read/write benchmarks not available on Linux
	else Usage();
#endif

	// Vortex producer-consumer test
	if (type == 0) {
		int GB = atoi(argv[2]);
		if (GB <= 0) Usage();

		// setup input parameters
		printf("Running producer (constant) -> consumer (summation)\n");
		uint64_t memory          = (1LLU << 30) * GB;
		uint64_t blockSizePower  = 21;
		uint64_t M = 0, L = 0, N = 2;
		thread*  pThread;
		thread*  cThread;

		// run the Vortex Producer-Consumer scenario
		VortexC* s = new VortexC(memory, blockSizePower, M, L, N);
		pThread    = new thread(&RunLoop<ItemType>, s->GetWriteBuf(), memory, WRITER_CONSTANT, true);
		cThread    = new thread(&RunLoop<ItemType>, s->GetReadBuf(), memory, READER_SUM, false);

		// finalize all threads
		pThread->join();
		s->FinishedWrite();
		cThread->join();
		s->FinishedRead();
		s->Reset();
	}
#ifdef _WIN32
	// Vortex file reader
	else if (type == 1) {
		printf("Running file reader\n");
		DWORD blockSizePower = 20;
		DWORD M = 0, L = 4, N = 1;

		IOWrapper iow;
		char* buf = iow.OpenRead(argv[1], NULL, blockSizePower, L, M, N);
		thread cThread(&RunLoop<ItemType>, buf, iow.GetSize(), READER_SUM, true);
		cThread.join();
	}
	// Vortex file writer
	else if (type == 2) {
		uint64_t GB = atoi(argv[2]);
		if (GB == 0) Usage();

		printf("Running file writer with %lld GB\n", GB);
		uint64_t memory = GB << 30;
		DWORD  blockSizePower = 20;
		DWORD  M = 4, L = 0, N = 1;

		IOWrapper iow;
		char* buf = iow.OpenWrite(argv[1], memory, NULL, blockSizePower, L, M, N);
		if (buf != NULL) {
			thread pThread(&RunLoop<ItemType>, buf, memory, WRITER_LCG, true);
			pThread.join();
			iow.CloseWriter(memory);
		}
	}
	// Vortex file copy
	else if (type == 3) {
		printf("Running file copy from %s to %s\n", argv[2], argv[3]);
		DWORD blockSizePower = 20;
		DWORD M = 4, L = 4, N = 1;							

		SpeedReporter sr;
		IOWrapper iowIn, iowOut;
		if (iowIn.OpenRead(argv[2], NULL, blockSizePower, L, M, N) != NULL) {
			sr.Start(0, iowIn.GetSize());
			if (iowOut.OpenWrite(argv[3], iowIn.GetSize(), iowIn.GetStream(), blockSizePower, L, M, N) != NULL) {
				iowOut.WaitUntilDone();
				sr.FinalReport();
			}
		}
	}
#endif
	// Vortex-Enabled In-Place MSD Radix Sort.
	else if (type == 4) {
		printf("Running uniform %u GB random sort\n", atoi(argv[2]));
		uint64_t GB             = atoi(argv[2]);
		uint64_t iterations     = atoi(argv[3]);
		uint64_t itemsPerSort   = GB * (1LLU << 30) / sizeof(ItemType);
		uint64_t memory         = itemsPerSort * sizeof(ItemType);
		uint64_t blockSizePower = 20;
		Syscall.SetAffinity(0);

		// setup the Vortex sort buckets
		VortexSort<ItemType>* vs = new VortexSort<ItemType>(itemsPerSort, blockSizePower);

		// prepare the input stream
		ItemType* inputBuf, *outputBuf;
		Stream*   inputS     = new VortexS(memory, memory, vs->sp, vs->nBuckets[0]);
		inputBuf = outputBuf = (ItemType*)inputS->GetReadBuf();

		for (uint64_t i = 0; i < iterations; i++) {
			// produce uniformly random data
			RunLoop<ItemType>((char*)inputBuf, itemsPerSort * sizeof(ItemType), WRITER_LCG, false);

			// run the sort
			void*  start = Syscall.StartTimer();
			vs->Sort(inputBuf, outputBuf, itemsPerSort);
			double elapsed = Syscall.EndTimer(start);

			// output the result
			double speed    = (double)itemsPerSort / elapsed / 1e6;
			double memUsed  = (double)(vs->sp->blockCount << blockSizePower);
			double memIdeal = (double)itemsPerSort * sizeof(ItemType);
			printf("\ttime %.3f sec, speed %.2f M/s, overhead %.2f%%, blocks %lld\n",
				elapsed, speed, (memUsed / memIdeal - 1) * 100, vs->sp->blockCount);

			// check sortedness
			ConsumerChecker(outputBuf, itemsPerSort);

			// reset the input buffer
			inputS->Reset();
		}
		delete inputS;
		delete vs;
	}
}