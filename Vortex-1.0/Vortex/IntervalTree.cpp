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

// adds a stream to the fault handler's tree
void IntervalTree::Add(void *a, void *b, void *userData) {
	IntervalNode node = { a, b, userData };
	Syscall.EnterCS(cs);
	if (!tree.empty()) {
		// perform basic checks
		auto it  = tree.upper_bound(node);
		auto prv = prev(it);

#ifdef _WIN32
		if (prv == tree.end()) {
#else
		if (prv == tree.end() || node.a <= prv->b) {
#endif
			if (it->a < node.b)
				ReportError("[From right] AddStream failed, new stream range overlapping with existing ones %p < %p\n", it->a, b);
		}
		else {
			if (prv->b > node.a)
				ReportError("[From left] AddStream failed, new stream range overlapping with existing ones %p > %p\n", prv->b, a);
		}
	}

	// passed all checks
	tree.insert(node);
	Syscall.LeaveCS(cs);
}

// removes a stream from the fault handler's tree
void IntervalTree::Remove(void *a) {
	IntervalNode dummy = { a, NULL, NULL };
	Syscall.EnterCS(cs);
	auto it = tree.find(dummy);
	if (it == tree.end())
		ReportError("%s: stream not found!\n", __FUNCTION__);

	tree.erase(it);
	Syscall.LeaveCS(cs);
}

// tries to find a stream within the fault handler's tree
void* IntervalTree::Find(void *address) {
	Syscall.EnterCS(cs);
	if (!tree.empty()) {
		IntervalNode dummy = { address, NULL, NULL };
		// find the smallest record that is greater than r
		auto it = tree.upper_bound(dummy);
		// prv is the greatest record that is smaller or equal to r
		auto prevNode = prev(it);

		// if prevNode exists, verify the range
		if (prevNode != tree.end()) {
			if (address >= prevNode->a && address < prevNode->b) {
				Syscall.LeaveCS(cs);
				return prevNode->userData;
			}
		}
	}

	// not found
	Syscall.LeaveCS(cs);
	return NULL;
}

// grabs the mutex, and the tree beginning
void IntervalTree::StartWalkTree(void) {
	Syscall.EnterCS(cs);
	walkit = tree.begin();
}

// used to iterate the tree after the lock is attained
const IntervalNode* IntervalTree::GetNextNode(void) {
	if (walkit == tree.end())
		return NULL;
	else
		return &(*walkit++);
}

// frees the mutex
void IntervalTree::FinishWalkTree(void) {
	Syscall.LeaveCS(cs);
}
