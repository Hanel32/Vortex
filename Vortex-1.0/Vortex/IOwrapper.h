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
#ifdef _WIN32
#define IOWRAPPER_READ	0
#define IOWRAPPER_WRITE	1

class IOWrapper {
	static constexpr double version = 1.1;
	char     filename[MAX_PATH+1];
	uint64_t blocks;
	char*    buf;
	HANDLE   fh;
	uint64_t filesize;
	VortexC* s;

	bool     eof;
	int      type;
	thread*  run;

	OVERLAPPED *ol;
	bool*    finished, bDeleteStream;
	DWORD*   bytesDeposited;
	uint64_t blockSize;

	void	 Run(void);
	void	 MoveOneBuffer(uint64_t off, int curBlock);
	DWORD	 GetResult(int curBlock);

	int		 SetSize(uint64_t fileLen);
	int		 FileSeek(uint64_t pos);
	int		 FileTell(uint64_t* pos);
	int		 SetVolumePrivilege(void);
public:
	IOWrapper ();
	~IOWrapper();
	bool     SaveFilename(char* filename);
	char*	 OpenRead(char* filename, VortexC* sExternal, int blockSizePower, int L, int M, int N);
	char*	 OpenWrite(char* filename, uint64_t size, VortexC* sExternal, int blockSizePower, int L, int M, int N);

	void	 CloseWriter(uint64_t bytesWritten);
	uint64_t GetSize(void) { return filesize; }
	VortexC* GetStream(void) { return s; }
	void	 WaitUntilDone(void);
};
#endif