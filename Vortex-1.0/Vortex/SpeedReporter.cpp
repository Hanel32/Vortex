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

// starts the timer
void SpeedReporter::Start(uint64_t off, uint64_t items) {
	clock       = Syscall.StartTimer();
	prev        = off;
	this->total = items;
}

// makes an intermediate report of program status in MB/s consumed
void SpeedReporter::Report(uint64_t off) {
	double elapsedLast  = Syscall.QueryTimer(clock);
	double elapsedStart = Syscall.EndTimer(clock);
	printf("	[%6.2f sec] average %8.2f MB/s (current %8.2f MB/s) %5.1f%% done\n", elapsedStart, (double) off / elapsedStart / 1e6,
		(double) (off - prev) / elapsedLast / 1e6, (double) off / (double) total * 100);
	prev = off;
}

// reports the final average speed recorded
void SpeedReporter::FinalReport(void) {
#ifdef _WIN32
	uint64_t kernel, user, junk;
	GetProcessTimes(GetCurrentProcess(), (LPFILETIME)&junk, (LPFILETIME)&junk, (LPFILETIME)&kernel, (LPFILETIME)&user);
	double ksec = (double)kernel / 1e7;
	double usec = (double)user / 1e7;
#endif

	double elapsedStart = Syscall.EndTimer(clock);
#ifdef _WIN32
	printf("Finished in %.2f sec, final speed %.2f MB/s (user %.2f sec, kernel %.2f sec)\n",
		elapsedStart, total / elapsedStart / 1e6, usec, ksec);
#else
	printf("Finished in %.2f sec, final speed %.2f MB/s\n",
		elapsedStart, (double) total / elapsedStart / 1e6);
#endif
}
