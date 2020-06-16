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
#include "stdint.h"

#ifdef __linux__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#endif
template <typename ItemType>
class SortingNetwork {
public:
	SortingNetwork();
	~SortingNetwork();

	void(*p[33]) (ItemType* d);

	void insertionSort2(ItemType arr[], int length);
	void sort(ItemType *d, int size);

	static inline void sort0(ItemType* d);
	static inline void sort1(ItemType* d);
	static inline void sort2(ItemType *d);
	static inline void sort3(ItemType *d);
	static inline void sort4(ItemType *d);
	static inline void sort5(ItemType *d);
	static inline void sort6(ItemType *d);
	static inline void sort7(ItemType *d);
	static inline void sort8(ItemType *d);
	static inline void sort9(ItemType *d);
	static inline void sort10(ItemType *d);
	static inline void sort11(ItemType *d);
	static inline void sort12(ItemType *d);
	static inline void sort13(ItemType *d);
	static inline void sort14(ItemType *d);
	static inline void sort15(ItemType *d);
	static inline void sort16(ItemType *d);
	static inline void sort17(ItemType *d);
	static inline void sort18(ItemType *d);
	static inline void sort19(ItemType *d);
	static inline void sort20(ItemType *d);
	static inline void sort21(ItemType *d);
	static inline void sort22(ItemType *d);
	static inline void sort23(ItemType *d);
	static inline void sort24(ItemType *d);
	static inline void sort25(ItemType *d);
	static inline void sort26(ItemType *d);
	static inline void sort27(ItemType *d);
	static inline void sort28(ItemType *d);
	static inline void sort29(ItemType *d);
	static inline void sort30(ItemType *d);
	static inline void sort31(ItemType *d);
	static inline void sort32(ItemType *d);
};
#ifdef __linux__
#pragma GCC diagnostic pop
#endif