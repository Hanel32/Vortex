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
#include "SystemFunctions.h"
class IntervalNode {
public:
	void *a, *b;
	void *userData;
	bool operator() (const IntervalNode &r1, const IntervalNode &r2) const {
		return r1.a < r2.a;
	}
};

class IntervalTree {
	CSType* cs;
	set<IntervalNode, IntervalNode> tree;
	set<IntervalNode, IntervalNode>::iterator walkit;
public:
	IntervalTree() { cs = (CSType*) Syscall.MakeCS(); }
	~IntervalTree() { Syscall.DeleteCS(cs); }
	void  Add(void *a, void *b, void *userData);
	void  Remove(void *a);
	void* Find(void *address);
	void  StartWalkTree(void);
	void  FinishWalkTree(void);
	const IntervalNode* GetNextNode(void);
	void  Lock(void)   { Syscall.EnterCS(cs); }
	void  Unlock(void) { Syscall.LeaveCS(cs); }
};