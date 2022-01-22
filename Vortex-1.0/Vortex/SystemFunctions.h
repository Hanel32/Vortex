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
// Hybrid System Types and Functions Definitions
#include <inttypes.h>
#include <stdio.h>

#ifdef _WIN32
// Windows definitions and headers
#include <Windows.h>
struct TimeType {
	LARGE_INTEGER start;
	LARGE_INTEGER last;
	LARGE_INTEGER res;
};
typedef ULONG_PTR          BlockType;
typedef CRITICAL_SECTION   CSType;
typedef CONDITION_VARIABLE CVType;
typedef HANDLE             EventType;
typedef HANDLE             SemaType;
typedef BlockType          OffType;
typedef int                CpuidType;
#else
// Linux definitions and headers
#include <stdlib.h>
#include <malloc.h>
#include <pthread.h>
#include <semaphore.h>
#include <errno.h>
#include <sys/signal.h>
#include <sys/signalfd.h>
#include <sys/mman.h>
#include <string.h>    
#include <unistd.h>
#include <math.h>
#include <sys/time.h>
#include <sys/times.h>
#include <algorithm>
#include <unistd.h>
// custom linux event
struct EventType {
	bool signalled = false;
	pthread_mutex_t* mutex;
	pthread_cond_t*  cond;
};
// linux time struct to emulate windows
struct TimeType {
	struct timespec start;
	struct timespec last;
};
typedef char            BlockType;
typedef pthread_mutex_t CSType;
typedef pthread_cond_t  CVType;
typedef sem_t           SemaType;
typedef uint64_t        OffType;
typedef uint            CpuidType;

// Windows-Linux hybridization definitions
#define Sleep(uint64_t)       usleep(uint64_t)
#define GetCurrentThreadId()  pthread_self()
#define INFINITE              INFINITY
#define BOOL                  bool
#define EXCEPTION_READ_FAULT  0
#define EXCEPTION_WRITE_FAULT 1
#define __restrict            __restrict__
#define GetLastError()        errno
#define __forceinline         // null definition to avoid compiler warnings

// Windows virtual memory definitions
// NOTE: not applicable to Linux - just for code compatability
#define MEM_RESERVE  0
#define MEM_PHYSICAL 1
#endif

// hybrid system functions for Windows/Linux
class sys {
#ifdef __linux__
	struct timespec timer;
#endif
public:
	// semaphore interactions
	void*  MakeSemaphore(uint64_t init, uint64_t max);
	void   WaitSemaphore(SemaType* sem);
	void   FreeSemaphore(SemaType* sem, uint64_t count);

	// event interactions
	void   MakeEvent(EventType& ev, const char* name);
	void   RaiseEvent(EventType& ev);
	void   LowerEvent(EventType& ev);
	void   WaitEvent(EventType& ev);
	bool   TimedWaitEvent(EventType& ev, uint64_t time);
	void   DeleteEvent(EventType& ev);

	// critical section interactions
	void*  MakeCS();
	void   EnterCS(CSType* cs);
	void   LeaveCS(CSType* cs);
	void   DeleteCS(CSType* ev);

	// condition Variable interactions
	void*  MakeCV();
	void   SleepCV(CVType* cv, CSType* cs);
	void   WakeCV(CVType* cv);
	void   DeleteCV(CVType* cv);

	// page mapping
	void   MapPages(char* dest, uint64_t pages, uint64_t pageSize, void* page);
	void   UnmapPages(void* page, uint64_t size);

	//  static memory allocation
	void*  AllocateStatic(uint64_t size);
	void   DeallocateStatic(char* ptr, uint64_t size);

	// virtual memory Space Allocation
	void*  AllocateVirtual(char* start, uint64_t size, int flag);
	void   FreeVirtual(void* ptr, uint64_t size);

	// physical page allocation - pages handled differently between OSes
#ifdef _WIN32
	void   AllocatePages(uint64_t pageCount, uint64_t pageSize, BlockType* start);
#else
	void*  AllocatePages(BlockType* start, uint64_t pagesPresent, uint64_t pagesNeeded, uint64_t pageSize, BlockType* end);
#endif
	void   FreePages(void* ptr, uint64_t pageCount, uint64_t blockSize);

	// aligned memory allocation
	void*  AllocAligned(uint64_t size, uint64_t alignment);
	void   DeallocAligned(void* ptr);

	// guard page handling
	void   InstallGuard(char* addr);
	void   RemoveGuard(char* addr);

	// high performance timer
	void*  StartTimer();
	double QueryTimer(void* curTime);
	double EndTimer(void* start);

	// affinity mask
	void   SetAffinity(uint64_t thread);

	// bit scan
	int    BitScan(uint64_t size);

	// cpuid
	void   cpuid(CpuidType* result, int info);
};

// globally available system functions
static sys Syscall;
