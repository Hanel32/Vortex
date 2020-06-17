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
class StreamManager {
	IntervalTree streamTree;
#ifdef _WIN32
	static LONG CALLBACK VectoredPageFaultHandler(PEXCEPTION_POINTERS ExceptionInfo);
	void                 GrantLockPagePrivilege(BOOL enable) const;
	PVOID                exceptionHandle;
#else
	static void          null_ref_handler(int signo, siginfo_t* info, void* context);
	struct sigaction     sa;
#endif
public:
	StreamManager();
	void	AddStream(void *a, void *b, Stream* s);
	Stream* FindStream(char* faultAddress);
	void	RemoveStream(void *a);
	~StreamManager();
};

extern StreamManager streamManager;

