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

// Vortex cpuid setup
VortexCpuId::VortexCpuId() {
	CpuidType result[4];
	Syscall.cpuid(result, 1);

	// ---------- check for AVX -----------
	// https://github.com/Mysticial/FeatureDetector/blob/master/src/x86/cpu_x86.cpp
	bool osUsesXSAVE_XRSTORE = (result[2] & (1 << 27)) != 0;
	bool cpuAVXSuport        = (result[2] & (1 << 28)) != 0;
	if (osUsesXSAVE_XRSTORE && cpuAVXSuport) {
		uint64_t xcrFeatureMask = _xgetbv(_XCR_XFEATURE_ENABLED_MASK);
		avx = (xcrFeatureMask & 0x6) == 0x6;
	}
}