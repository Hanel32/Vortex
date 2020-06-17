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

// global Vortex streamManager
StreamManager streamManager;

// sets up the Vortex exception handler, and prints OS/architecture
StreamManager::StreamManager() {
#ifdef _WIN32
	if ((exceptionHandle = AddVectoredExceptionHandler(1, VectoredPageFaultHandler)) == NULL) 
		ReportError("AddVectoredExceptionHandler", GetLastError());
	GrantLockPagePrivilege(true);
#else
	memset(&sa, '\0', sizeof(sa));
	sigemptyset(&sa.sa_mask);
	sa.sa_flags     = SA_SIGINFO | SA_ONSTACK;
	sa.sa_sigaction = null_ref_handler;
	sigfillset(&(sa.sa_mask));
	if (sigaction(SIGSEGV, &sa, NULL) != 0)
		ReportError("sigaction failed");
#endif
}

// removes the exception handler
StreamManager::~StreamManager() {
#ifdef _WIN32
	if (RemoveVectoredExceptionHandler(exceptionHandle) == 0) 
		ReportError("RemoveVectoredExceptionHandler", GetLastError());
#endif
}

// adds a stream to the streamTree
void StreamManager::AddStream(void *a, void *b, Stream* s) {
	streamTree.Add(a, b, s);
}

// removes a stream from the streamTree
void StreamManager::RemoveStream(void *a) {
	streamTree.Remove(a);
}

// finds a stream in the streamTree
Stream* StreamManager::FindStream(char* faultAddress) {
	return (Stream*)streamTree.Find(faultAddress);
}


#ifdef _WIN32
// Windows Vortex fault handling
LONG CALLBACK StreamManager::VectoredPageFaultHandler(PEXCEPTION_POINTERS ExceptionInfo) {
	DWORD exceptionCode = ExceptionInfo->ExceptionRecord->ExceptionCode;
	if (exceptionCode != EXCEPTION_ACCESS_VIOLATION && exceptionCode != EXCEPTION_GUARD_PAGE) 
		return EXCEPTION_CONTINUE_SEARCH;

	int   faultType    = (int)ExceptionInfo->ExceptionRecord->ExceptionInformation[0];
	char* faultAddress = (char*)ExceptionInfo->ExceptionRecord->ExceptionInformation[1];

	// check if fault address belongs to a known stream
	Stream* stream = streamManager.FindStream(faultAddress);

	// if not pass on to the next handler
	if (stream == NULL)	return EXCEPTION_CONTINUE_SEARCH;		

	if (stream->ProcessFault(faultType, faultAddress)) 
		return EXCEPTION_CONTINUE_EXECUTION;
	else                                               
		return EXCEPTION_CONTINUE_SEARCH;
}

// grants the privilege for the application to map/unmap in userspace
void StreamManager::GrantLockPagePrivilege(BOOL enable) const {
	TOKEN_PRIVILEGES tokenPrivileges;
	HANDLE           tokenHandle;

	// Open token
	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &tokenHandle))
		ReportError("OpenProcessToken Error %d\n", GetLastError());

	// enable
	tokenPrivileges.PrivilegeCount = 1;
	if (enable)
		tokenPrivileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	else
		tokenPrivileges.Privileges[0].Attributes = 0;

	// get LUID
	if (!LookupPrivilegeValue(NULL, SE_LOCK_MEMORY_NAME, &(tokenPrivileges.Privileges[0].Luid)))
		ReportError("LookupPrivilegeValue Error %d\n", GetLastError());

	// Elevate privilege of the process token
	if (!AdjustTokenPrivileges(tokenHandle, FALSE, &tokenPrivileges, 0, NULL, NULL))
		ReportError("AdjustTokenPrivileges Error %d\n", GetLastError());

	if (GetLastError() != ERROR_SUCCESS)
		PRINT("Cannot enable the SE_LOCK_MEMORY_NAME privilege; please check the local policy\n");

	if (!CloseHandle(tokenHandle))
		ReportError("CloseHandle Error %d\n", GetLastError());
}

#else
// Linux Vortex fault handler - exclusively catches segmentation faults
void StreamManager::null_ref_handler(int signo, siginfo_t* info, void* context) {
	// identify the type of fault
	int err = (int)((ucontext_t*)context)->uc_mcontext.gregs[REG_ERR];
	bool write_fault = (err & 0x2);

	// find the stream that this fault belonged to, if any
	char* faultAddress = (char*)info->si_addr;
	Stream* stream = streamManager.FindStream(faultAddress);
	if (stream == NULL) 
		ReportError("NO STREAM FOUND! Write fault? %d at loc %lu\n", write_fault, (uint64_t)faultAddress);

	// if the fault belonged to a stream, pass the exception to the local handler
	if (write_fault) {
		if (stream->ProcessFault(EXCEPTION_WRITE_FAULT, faultAddress)) return;
		else ReportError("UNHANDLED WRITE EXCEPTION ON Vortex STREAM!\n");
	}
	else {
		if (stream->ProcessFault(EXCEPTION_READ_FAULT, faultAddress)) return;
		else ReportError("UNHANDLED READ EXCEPTION ON Vortex STREAM!\n");
	}
}
#endif