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

// creates a semaphore and returns a pointer
void* sys::MakeSemaphore(uint64_t init, uint64_t max) {
	SemaType* sem = new SemaType;
#ifdef _WIN32
	if ((*sem = CreateSemaphore(NULL, (LONG)init, (LONG)max, NULL)) == NULL)
		ReportError("CreateSemaphore semFull", GetLastError());
#else
	if (sem_init(sem, 0, (unsigned int)init) == -1)
		ReportError("Error creating semaphore! %d\n", errno);
#endif
	return sem;
}

// waits on the given semaphore
void  sys::WaitSemaphore(SemaType* sem) {
#ifdef _WIN32
	if (WaitForSingleObject(*sem, INFINITE) == WAIT_FAILED)
		ReportError("WaitForSingleObject Write error %d\n", GetLastError());
#else
	sem_wait(sem);
#endif
}

// increments the given semaphore's interal counter
void  sys::FreeSemaphore(SemaType* sem, uint64_t count) {
#ifdef _WIN32
	if(!ReleaseSemaphore(*sem, (LONG)count, NULL))
		ReportError("ReleaseSemaphore WriteFault error %d\n", GetLastError());
#else
	for (uint64_t i = 0; i < count; i++) {
		sem_post(sem);
	}
#endif
}

// creates an event at the given pointer
void sys::MakeEvent(EventType& ev, const char* name) {
#ifdef _WIN32
	if ((ev = CreateEvent(NULL, TRUE, FALSE, name)) == NULL) 
		ReportError("Error creating eventHandle %d\n", GetLastError());
#else
	ev.mutex = (CSType*)MakeCS();
	ev.cond  = (CVType*)MakeCV();
#endif
}

// raises the given event
void  sys::RaiseEvent(EventType& ev) {
#ifdef _WIN32
	if (!SetEvent((HANDLE)ev))
		ReportError("SetEvent failed (%d)\n", GetLastError());
#else
	pthread_mutex_lock(ev.mutex);
	ev.signalled = true;
	pthread_mutex_unlock(ev.mutex);
	pthread_cond_signal(ev.cond);
#endif
}

// resets the given event
void  sys::LowerEvent(EventType& ev) {
#ifdef _WIN32
	ResetEvent(ev);
#else
	pthread_mutex_lock(ev.mutex);
	ev.signalled = false;
	pthread_mutex_unlock(ev.mutex);
#endif
}

// waits on the given event indefinitely
void  sys::WaitEvent(EventType& ev) {
#ifdef _WIN32
	if (WaitForSingleObject(ev, INFINITE) != WAIT_OBJECT_0)
		ReportError("CRITICAL ERROR WAITING FOR WAITEVENT %d\n", GetLastError());
#else
	pthread_mutex_lock(ev.mutex);
	while (!ev.signalled)
		pthread_cond_wait(ev.cond, ev.mutex);
	pthread_mutex_unlock(ev.mutex);
#endif
}

// waits on the given event for the prescribed amount of time
bool  sys::TimedWaitEvent(EventType& ev, uint64_t times) {
#ifdef _WIN32
	unsigned int wait = WaitForSingleObject(ev, (DWORD)times);
	return wait == WAIT_OBJECT_0;
#else
	struct timespec ts;
	struct timeval now;

	gettimeofday(&now, NULL);

	ts.tv_sec = time(NULL) + times / (uint64_t)1e3;
	ts.tv_nsec = now.tv_usec * (uint64_t)1e9 * (times % (uint64_t)1e3);
	ts.tv_sec += ts.tv_nsec / (uint64_t)1e9;
	ts.tv_nsec %= (uint64_t)1e9;

	pthread_mutex_lock(ev.mutex);
	pthread_cond_timedwait(ev.cond, ev.mutex, &ts);
	pthread_mutex_unlock(ev.mutex);

	return ev.signalled;
#endif
}

// deletes the given event
void  sys::DeleteEvent(EventType& ev) {
#ifdef _WIN32
	CloseHandle(ev);
#else
	DeleteCS(ev.mutex);
	DeleteCV(ev.cond);
#endif
}

// creates a critical section and returns a pointer
void* sys::MakeCS() {
	CSType* cs = new CSType;
#ifdef _WIN32
	InitializeCriticalSection(cs);
#else
	if (pthread_mutex_init(cs, NULL) != 0)
		ReportError("Mutex init has failed\n");
#endif
	return cs;
}

// locks the given critical section
void sys::EnterCS(CSType* cs) {
#ifdef _WIN32
	EnterCriticalSection(cs);
#else
	pthread_mutex_lock(cs);
#endif
}

// unlocks the given critical section
void sys::LeaveCS(CSType* cs) {
#ifdef _WIN32
	LeaveCriticalSection(cs);
#else
	pthread_mutex_unlock(cs);
#endif
}

// deletes the given critical section
void sys::DeleteCS(CSType* cs) {
#ifdef _WIN32
	DeleteCriticalSection(cs);
#else
	pthread_mutex_destroy(cs);
#endif
}

// creates a condition variable and returns a pointer
void* sys::MakeCV() {
	CVType* cv = new CVType;
#ifdef _WIN32
	InitializeConditionVariable(cv);
#else
	pthread_cond_init(cv, NULL);
#endif
	return cv;
}

// sleeps on the given condition variable
void sys::SleepCV(CVType* cv, CSType* cs) {
#ifdef _WIN32
	SleepConditionVariableCS(cv, cs, INFINITE);
#else
	pthread_cond_wait(cv, cs);
#endif
}

// wakes the given condition variable
void sys::WakeCV(CVType* cv) {
#ifdef _WIN32
	WakeConditionVariable(cv);
#else
	pthread_cond_signal(cv);
#endif
}

// deletes the given condition variable
void sys::DeleteCV(CVType* cv) {
#ifdef _WIN32
	delete cv;
#else
	pthread_cond_destroy(cv);
#endif
}

// maps the given pages
#ifdef _WIN32
#pragma warning( push )
#pragma warning( disable : 4100)
#endif
void sys::MapPages(char* dest, uint64_t pages, uint64_t pageSize, void* page) {
#ifdef _WIN32
	if (!MapUserPhysicalPages(dest, pages, (BlockType*)page)) 
		ReportError("mapping new block failed with %d\n", GetLastError());
#else
	void* re = mremap(page, pages * pageSize, pages * pageSize, MREMAP_MAYMOVE | MREMAP_FIXED, dest);
	if (re == (void*)-1) 
		ReportError("Error on mremap at %p from %p error %d\n", dest, page, errno);
	page = dest;
#endif
}
#ifdef _WIN32
#pragma warning( pop ) 
#endif

// unmaps the given pages - null in Linux as you cannot both 1) unmap, and 2) retain the PFNs
void sys::UnmapPages(void* page, uint64_t pages) {
#ifdef _WIN32
	if (!MapUserPhysicalPages(page, pages, NULL))
		ReportError("unmapping block failed with %d\n", GetLastError());
#endif
}

// installs guard protection on the given page-aligned address
void sys::InstallGuard(char* addr) {
#ifdef _WIN32
	DWORD oldProtect;
	if (!VirtualProtect(addr, 1, PAGE_NOACCESS, &oldProtect))
		ReportError("guard install failed with %d\n", GetLastError());
#else
	mprotect(addr, 4096, PROT_NONE);
#endif
}

// removes guard protection from the given page-aligned address
void sys::RemoveGuard(char* addr) {
#ifdef _WIN32
	DWORD oldProtect;
	if (!VirtualProtect(addr, 1, PAGE_READWRITE, &oldProtect))
		ReportError("guard removal failed with %d\n", GetLastError());
#else
	mprotect(addr, 4096, PROT_READ|PROT_WRITE);
#endif
}

// allocates static memory
void* sys::AllocateStatic(uint64_t size) {
#ifdef _WIN32
	char* buffer = (char*)VirtualAlloc(NULL, size, MEM_COMMIT, PAGE_READWRITE);
	if (buffer == NULL) printf("ERROR VIRTUALALLOC %d\n", GetLastError());
#else
	void* buffer = mmap(NULL, size, PROT_WRITE | PROT_READ, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
	if (buffer == MAP_FAILED) printf("ERROR MMAP %d\n", GetLastError());
#endif
	memset(buffer, 0, size);
	return buffer;
}

// allocates virtual memory
void* sys::AllocateVirtual(char* start, uint64_t size, int flag) {
	char* temp;
#ifdef _WIN32
	if (flag == (MEM_RESERVE | MEM_PHYSICAL))
		temp = (char*)VirtualAlloc(start, size, flag, PAGE_READWRITE);
	else 
		temp = (char*)VirtualAlloc(start, size, flag, PAGE_NOACCESS);
	if (temp == NULL) ReportError("VirtuaAlloc failed with %d\n", GetLastError());
#else
	temp = (char*) mmap(NULL, size, PROT_NONE, MAP_ANONYMOUS | MAP_PRIVATE | MAP_NORESERVE, -1, 0);
#endif	
	return (void*)temp;
}

// allocates physical pages for virtual memory mapping
#ifdef _WIN32
#pragma warning( push )
#pragma warning( disable : 4100)
void  sys::AllocatePages(uint64_t pageCount, uint64_t pageSize, BlockType* start) {
#else
void* sys::AllocatePages(BlockType* start, uint64_t pagesPresent, uint64_t pagesNeeded, uint64_t pageSize, BlockType* end) {
#endif
#ifdef _WIN32
	auto intendedPages = pageCount;
	if (!AllocateUserPhysicalPages(GetCurrentProcess(), &intendedPages, start))
		ReportError("AllocateUserPhysicalPages with %d\n", GetLastError());

	if (intendedPages != pageCount) 
		ReportError("Not enough memory for AllocateUserPhysicalPages\n");
#else
	if (start == nullptr) {
		start = (char*)mmap(0, pageSize * pagesNeeded, PROT_WRITE | PROT_READ, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
		if (start == MAP_FAILED) ReportError("Could not mmap");
		memset(start, 0, pageSize * pagesNeeded);
	}
	else {
		// create the mapping
		uint64_t newSize = pageSize * pagesNeeded + (end - start);
		BlockType* newPFN = (BlockType*)mmap(0, newSize,
			PROT_WRITE | PROT_READ, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);

		// fetch new pages from the OS
		memset(newPFN + (end - start), 0, pagesNeeded * pageSize);

		// map old pages onto the new location
		mremap(start, (end - start), (end - start),
			MREMAP_MAYMOVE | MREMAP_FIXED, newPFN);

		// map new pages after old pages
		char* dest = newPFN + (pagesPresent * pageSize);
		mremap(newPFN + (end - start), pagesNeeded * pageSize, pagesNeeded * pageSize, 
			MREMAP_MAYMOVE | MREMAP_FIXED, (char*)dest);
		start = newPFN;
	}
	return start;
#endif
}
#ifdef _WIN32
#pragma warning( pop )
#endif

void* sys::AllocAligned(uint64_t size, uint64_t alignment) {
	void* alloc;
#ifdef _WIN32
	alloc = _aligned_malloc(size, alignment);
#else
	alloc = memalign(alignment, size);
#endif
	return alloc;
}

void sys::DeallocAligned(void* ptr) {
#ifdef _WIN32
	_aligned_free(ptr);
#else
	free(ptr);
#endif
}

// deallocates a static mapping
void sys::DeallocateStatic(char* ptr, uint64_t size) {
#ifdef _WIN32
	VirtualFree(ptr, size, MEM_RELEASE);
#else
	munmap(ptr, size);
#endif
}

// frees a virtual mapping
#ifdef _WIN32
#pragma warning( push )
#pragma warning( disable : 4100)
#endif
void sys::FreeVirtual(void* ptr, uint64_t size) {
#ifdef _WIN32
	if (!VirtualFree(ptr, 0, MEM_RELEASE))
		ReportError("Release failed with %d\n", GetLastError());
#else
	munmap(ptr, size);
#endif
}
#ifdef _WIN32
#pragma warning( pop )
#endif

// frees allocated physical pages
#ifdef _WIN32
#pragma warning( push )
#pragma warning( disable : 4100)
#endif
void sys::FreePages(void* ptr, uint64_t pageCount, uint64_t pageSize) {
#ifdef _WIN32
	if (!FreeUserPhysicalPages(GetCurrentProcess(), &pageCount, (BlockType*)ptr))
		ReportError("FreeUserPhysicalPages", GetLastError());
	free(ptr);
#else
	munmap(ptr, pageCount * pageSize);
#endif
}
#ifdef _WIN32
#pragma warning( pop )
#endif

void* sys::StartTimer() {
	TimeType* clock = new TimeType();
#ifdef _WIN32
	QueryPerformanceFrequency(&clock->res);
	QueryPerformanceCounter(&clock->start);
#else
	clock_gettime(CLOCK_MONOTONIC, &clock->start);
#endif
	clock->last = clock->start;
	return clock;
}

// reads the clock and stores the result at the given pointer
double sys::QueryTimer(void* start) {
	TimeType* clock = (TimeType*)start;
#ifdef _WIN32
	LARGE_INTEGER lastTime = clock->last;
	QueryPerformanceCounter(&clock->last);
	double elapsedLast = (double)(clock->last.QuadPart - lastTime.QuadPart) / clock->res.QuadPart;
#else
	struct timespec lastTime = clock->last;
	clock_gettime(CLOCK_MONOTONIC, &clock->last);
	double elapsedLast = ((double)(clock->last.tv_sec - lastTime.tv_sec) * 1e9 + (double)(clock->last.tv_nsec - lastTime.tv_nsec)) / 1e9;
#endif
	return elapsedLast;
}

double sys::EndTimer(void* start) {
	TimeType* clock = (TimeType*)start;
	QueryTimer(clock);
#ifdef _WIN32
	double elapsed = (double)(clock->last.QuadPart - clock->start.QuadPart) / clock->res.QuadPart;
#else
	double elapsed = ((double)(clock->last.tv_sec - clock->start.tv_sec) * 1e9 + (double)(clock->last.tv_nsec - clock->start.tv_nsec)) / 1e9;
#endif
	return elapsed;
}

// sets the core affinity of the given thread
void sys::SetAffinity(uint64_t threadID) {
#ifdef _WIN32
	SetThreadAffinityMask(GetCurrentThread(), 1LLU << (2 * threadID));
#else
	pthread_t thread = pthread_self();
	cpu_set_t cpuset;
	CPU_ZERO(&cpuset);
	CPU_SET(threadID, &cpuset);

	pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cpuset);
#endif
}

// counts the number of MSD zeroes using builtin functions
int sys::BitScan(uint64_t size) {
#ifdef _WIN32
	DWORD inputPower;
	_BitScanReverse64(&inputPower, size);
#else
	int inputPower;
	inputPower = 63 - __builtin_clzll(size);
#endif
	return (int) inputPower;
}

// cpuid usage switch between Linux and Windows
void sys::cpuid(CpuidType* result, int info) {
#ifdef _WIN32
	__cpuid(result, info);
#else
	__get_cpuid(info, result, result + 1, result + 2, result + 3);
#endif
}