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
#define READER_SUM		0
#define READER_JUMP		1
#define WRITER_CONSTANT	2
#define WRITER_LCG		3
#define MAX_PRINTOUT	1024
#ifdef _WIN32
#define PRINT(fmt, ...) { char buf_PRINT[MAX_PRINTOUT] = "%s: "; strcat_s(buf_PRINT, MAX_PRINTOUT, fmt); printf (buf_PRINT, __FUNCTION__, ##__VA_ARGS__); }
#else
#define PRINT(fmt, ...) { char buf_PRINT[MAX_PRINTOUT] = "%s: "; strcat(buf_PRINT, fmt); printf (buf_PRINT, __FUNCTION__, ##__VA_ARGS__); }
#endif
#define ReportError(fmt, ...) { PRINT(fmt, ##__VA_ARGS__); getchar(); exit(-1); }

// rounds up a value to the nearest base
inline uint64_t RoundUp(uint64_t value, uint64_t base) {
	return ((value + base - 1) / base) * base;
}

// rounds down a value to the nearest base
inline uint64_t RoundDown(uint64_t value, uint64_t base) {
	return (value / base) * base;
}