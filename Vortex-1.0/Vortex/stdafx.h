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
#ifdef  _WIN32
#include <intrin.h>		// MMX
#include <zmmintrin.h>	// AVX-512
#endif
#include <emmintrin.h>	// SSE
#include <immintrin.h>	// AVX
#include <mmintrin.h>
#include <algorithm>
#include <stdexcept>
#include <iostream>
#include <thread>
#include <vector>
#include <memory>
#include <math.h>
#include <stack>
#include <map>
#include <set>

using namespace std;

#include "SystemFunctions.h"
#ifdef __linux__
// differences in printf formatting between OS versions
// the default configuration is for Windows, thus ignore -Wformat
#pragma GCC diagnostic ignored "-Wformat"
#endif

#ifdef _WIN32
 // note: do not omit frame pointers in project properties; this slows things down!
EXTERN_C IMAGE_DOS_HEADER __ImageBase;
#endif

#include "common.h"             
#include "IntervalTree.h"        
#include "cpuid_custom.h"        
#include "SortingNetwork64.h"    
#include "Stream.h"              

#include "StreamPool.h"        
#include "StreamManager.h"       
#include "VortexC.h"            
#include "VortexS.h"             

#include "VortexSort.h"
#include "IOwrapper.h"
#include "SpeedReporter.h"
#include "Benchmarks.h"