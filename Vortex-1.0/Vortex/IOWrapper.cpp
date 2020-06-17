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

#ifdef _WIN32
IOWrapper::IOWrapper () {
	SetVolumePrivilege ();
	run           = NULL;
	bDeleteStream = false;
}

IOWrapper::~IOWrapper() {
	if (run != NULL)  { 
		run->join(); 
		delete run; 
	}
	if (bDeleteStream) delete s;
}

bool IOWrapper::SaveFilename(char* fname) {
	if (strlen(fname) > MAX_PATH) {
		printf("%s: filename too long, above %d\n", __FUNCTION__, MAX_PATH);
		return false;
	}

	// save a copy since the file needs to be reopened at the end to set its size
	strcpy_s(filename, strlen(fname), fname);
	return true;
}

char *IOWrapper::OpenRead(char* fname, VortexC* sExternal, int blockSizePower, int L, int M, int N) {
	if (!SaveFilename(fname)) return NULL;
	type = IOWRAPPER_READ;

	fh = CreateFile(fname, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_NO_BUFFERING | FILE_FLAG_SEQUENTIAL_SCAN | FILE_FLAG_OVERLAPPED, NULL);

	if (fh == INVALID_HANDLE_VALUE) {
		printf("%s: CreateFile failed with %d\n", __FUNCTION__, GetLastError());
		return NULL;
	}

	LARGE_INTEGER fsize;
	GetFileSizeEx(fh, &fsize);
	filesize = fsize.QuadPart;
	if (sExternal == NULL) {
		s = new VortexC(filesize, blockSizePower, M, L, N);
		bDeleteStream = true;
	}
	else
		s = sExternal;

	buf    = s->GetWriteBuf();
	blocks = max(s->GetProducerComeback(), 1);		// cannot be zero, even if L = 0 in the stream

	// start the reader thread
	run = new thread(&IOWrapper::Run, this);
	return s->GetReadBuf();
}

char *IOWrapper::OpenWrite(char* fname, uint64_t size, VortexC* sExternal, int blockSizePower, int L, int M, int N) {
	if (!SaveFilename(fname)) return NULL;

	type = IOWRAPPER_WRITE;
	fh = CreateFile(fname, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
		CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_NO_BUFFERING | FILE_FLAG_SEQUENTIAL_SCAN | FILE_FLAG_OVERLAPPED, NULL);

	if (fh == INVALID_HANDLE_VALUE) {
		printf("%s: CreateFile failed with %d\n", __FUNCTION__, GetLastError());
		return NULL;
	}

	if (sExternal == NULL) {
		s = new VortexC(size, blockSizePower, M, L, N);
		bDeleteStream = true;
	}
	else
		s = sExternal;

	filesize = size;
	buf = s->GetReadBuf();
	SetSize(filesize);
	blocks = max(s->GetConsumerComeback(), 1);	// cannot be zero, even if M = 0 in the stream
	// start the writer thread
	run = new thread(&IOWrapper::Run, this);
	return s->GetWriteBuf();
}

void IOWrapper::Run (void) {
	blockSize      = s->GetBlockSize();
	ol             = new OVERLAPPED[blocks];
	finished       = new bool[blocks];
	bytesDeposited = new DWORD [blocks];
	eof            = false;

	uint64_t off = 0;
	memset(ol, 0, blocks*sizeof(OVERLAPPED));					// MSDN says must set this to zero
	// note that this loop may quit early and only a subset of the ol structs will have a valid event
	int pendingBlocks = 0;
	for (int i = 0; i < blocks && off < filesize; i++) {
		ol [i].hEvent = CreateEvent (NULL, true, false, NULL);
		MoveOneBuffer (off, i);
		off += blockSize;
		pendingBlocks++;
	}

	int curBlock = 0;
	// the idea is that we need to move off past the end of file by the number of pending blocks
	while (off < filesize + blockSize * pendingBlocks && !eof) {
		GetResult (curBlock);

		if (off < filesize)
			MoveOneBuffer (off, curBlock);

		curBlock = (curBlock + 1) % blocks;
		off += blockSize;
	}

	if (type == IOWRAPPER_READ)
		s->FinishedWrite ();			// need to release the last block to consumer; can also be done implicitly with a page fault
	 
	for (int i = 0; i < blocks && ol[i].hEvent; i++)		// skip zero events as they have not been initialized
		CloseHandle (ol [i].hEvent);

	delete[] ol;
	delete[] finished;
	delete[] bytesDeposited;

	if (type == IOWRAPPER_WRITE) {
		CloseHandle(fh);
		// set the correct file size; since we used FILE_FLAG_NO_BUFFERING, the file needs to be reopened
		fh = CreateFile(filename, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
			OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, NULL);
		if (fh == INVALID_HANDLE_VALUE)
			ReportError("Unable to reopen, CreateFile failed with %d\n", __FUNCTION__, GetLastError());
		SetSize(filesize);
	}

	CloseHandle (fh);
}

void IOWrapper::MoveOneBuffer(uint64_t off, int curBlock) {
	BOOL err;
	DWORD bytesTransferred;

	// request goes into the next block
	ol[curBlock].Offset = off & 0xffffffff;
	ol[curBlock].OffsetHigh = off >> 32;

	if (type == IOWRAPPER_READ) {
		// force a page fault to map the next block in stream; otherwise, ReadFile fails with 998
		buf[off + 1] = 0;
		err = ReadFile(fh, buf + off, (DWORD)blockSize, &bytesTransferred, ol + curBlock);
	}
	else {
		// force a page fault to map the next block in stream; volatile is needed to avoid compiler optimizations
#pragma warning(disable : 4189)
		volatile char x = buf[off + 1];

		err = WriteFile(fh, buf + off, (DWORD)blockSize, &bytesTransferred, ol + curBlock);
	}

	if (err == FALSE) {
		int code = GetLastError();
		if (type == IOWRAPPER_READ && code == ERROR_HANDLE_EOF) {
			printf("ReadFile encountered EOF on block %d\n", curBlock);
			eof = true;
			return;
		}
		else if (code != ERROR_IO_PENDING) {
			printf("%s: ReadFile/WriteFile failed with %d\n", __FUNCTION__, code);
			exit(-1);
		}

		finished[curBlock] = false;
		bytesDeposited[curBlock] = 0;
	}
	else {
		printf("ReadFile completed synchronously on block %d\n", curBlock);
		finished[curBlock] = true;
		bytesDeposited[curBlock] = bytesTransferred;
	}
}

DWORD IOWrapper::GetResult(int curBlock)
{
	BOOL err;
	DWORD bytesTransferred;
	if (finished[curBlock] == false) {
		if ((err = GetOverlappedResult(fh, ol + curBlock, &bytesTransferred, TRUE)) == FALSE) {
			int code = GetLastError();
			if (type == IOWRAPPER_READ && code == ERROR_HANDLE_EOF) {
				printf("GetOverlappedResult encountered EOF on block %d\n", curBlock);
				eof = true;
			}
			else {
				printf("%s: GetOverlappedResult failed with %d\n", __FUNCTION__, code);
				exit(-1);
			}
		}
	}
	else {
		bytesTransferred = bytesDeposited[curBlock];
	}

	return bytesTransferred;
}


void IOWrapper::CloseWriter(uint64_t bytesWritten) {
	filesize = bytesWritten;	// Run() is hanging on last incomplete block; it gets written, then file is shrunk
	s->FinishedWrite();
	WaitUntilDone();
}

void IOWrapper::WaitUntilDone(void) {
	if (run != NULL) {
		run->join();
		delete run;
		run = NULL;
	}
}

int IOWrapper::SetSize (uint64_t fileLen) {
	// save current position
	uint64_t pos;
	FileTell (&pos);

	// jump forward
	if (FileSeek (fileLen) == 0) {
		printf ("FileSeek failed with error %d\n", GetLastError ());
		return false;
	}

	// force the end of file to be here
	// fails on files larger than 16.3TB (roughly)
	if (SetEndOfFile (fh) == 0)	{
		printf ("SetEndOfFile failed with error %d\n", GetLastError ());
		return false;
	}

	// make sure it sticks
	if (SetFileValidData (fh, fileLen) == 0) {
		printf ("SetFileValidData failed with error %d\n", GetLastError ());
		return false;
	}

	// restore the original position in case we need to continue writing there
	FileSeek (pos);

	return true;
}

int IOWrapper::FileSeek (uint64_t pos) {
	LARGE_INTEGER tmp;
	tmp.QuadPart = pos;
	return SetFilePointerEx (fh, tmp, NULL, FILE_BEGIN);
}

int IOWrapper::FileTell (uint64_t *pos) {
	LARGE_INTEGER tmp;
	tmp.QuadPart = 0;

	return SetFilePointerEx (fh, tmp, (LARGE_INTEGER*)pos, FILE_CURRENT);
}

int IOWrapper::SetVolumePrivilege (void) {
	HANDLE Token;
	TOKEN_PRIVILEGES NewPrivileges;
	LUID LuidPrivilege;

	if (OpenProcessToken (GetCurrentProcess (), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &Token) == 0) {
		printf ("OpenProcessToken failed with %d\n", GetLastError ());
		return false;
	}

	if (LookupPrivilegeValue (NULL, SE_MANAGE_VOLUME_NAME, &LuidPrivilege) == 0) {
		printf ("LookupPrivilegeValue failed with %d\n", GetLastError ());
		return false;
	}

	NewPrivileges.PrivilegeCount           = 1;
	NewPrivileges.Privileges[0].Luid       = LuidPrivilege;
	NewPrivileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

	if (AdjustTokenPrivileges (Token, FALSE, &NewPrivileges, 0, NULL, NULL) == 0) {
		printf ("AdjustTokenPrivileges failed with %d\n", GetLastError ());
		return false;
	}

	if (CloseHandle (Token) == 0) {
		printf ("CloseHandle failed with %d\n", GetLastError ());
		return false;
	}

	return true;
}
#endif