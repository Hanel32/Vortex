<p align="center">
  <img src="https://github.com/chingy1510/Vortex_Test/blob/master/Vortex%20Charts/cloth.jpg?raw=true">
</p>

# Vortex: Extreme-Performance Streaming and Radix Sorting

[![GPLv3 license](https://img.shields.io/badge/License-GPLv3-blue.svg?style=plastic&logo=appveyor)](LICENSE)

This repository contains the implementation of the streaming and sorting algorithms described in the [**Vortex paper**](https://dl.acm.org/doi/10.1145/3373376.3378527) which was developed by the [**Texas A&M University Internet Research Lab**](http://irl.cs.tamu.edu/projects/streams/) and published in the [**ACM ASPLOS 2020**](https://asplos-conference.org/) conference hosted virtually in Lausanne, Switzerland. The ACM-hosted virtual presentation of the paper can be viewed [**here**](https://www.youtube.com/watch?v=yF9j_KgH1_U).

## Abstract
Many applications in data analytics, information retrieval, and cluster computing process huge amounts of information. The complexity of involved algorithms and massive scale of data require a programming model that can not only offer a simple abstraction for inputs larger than RAM, but also squeeze maximum performance out of the available hardware. While these are usually conflicting goals, we show that this does not have to be the case for sequentially-processed data, i.e., in streaming applications. We develop a set of algorithms called Vortex that force the application to generate access violations (i.e., page faults) during processing of the stream, which are transparently handled in such a way that creates an illusion of an infinite buffer that fits into a regular C/C++ pointer. This design makes Vortex by far the simplest-to-use and fastest platform for various types of streaming I/O, inter-thread data transfer, and key shuffling. We introduce several such applications -- file I/O wrapper, bounded producer-consumer pipeline, vanishing array, key-partitioning engine, and novel in-place radix sort that is 3-4 times faster than the best prior approaches.

## Performance

As a quick preview of Vortex sorting and streaming performance, in the tables below we analyze the Vortex 1) single-threaded sorting performance, 2) batched producer-consumer throughput, 3) file I/O speed. All machines were outfitted with 32 GB of RAM. For a richer discussion and broader set of Vortex abstraction benchmarks, please refer to the [**ASPLOS publication**](https://dl.acm.org/doi/10.1145/3373376.3378527), which extensively discusses Vortex streaming and sorting.

<table align="center">
<thead>
  <tr>
    <th colspan="8">Out-Of-Place Sort Speeds (M keys/s, 64-bit keys)</th>
  </tr>
</thead>
<tbody>
  <tr>
    <td align="center" rowspan="2"><b><sub>Sort</sub></b></td>
    <td align="center" rowspan="2"><b><sub>Year</sub></b></td>
    <td align="center" colspan="3"><b><sub>8 GB of keys</sub></b></td>
    <td align="center" colspan="3"><b><sub>24 GB of keys</sub></b></td>
  </tr>
  <tr>
    <td align="center"><b><sub>i7-3930K</sub></b></td>
    <td align="center"><b><sub>i7-4930K</sub></b></td>
    <td align="center"><b><sub>i7-7820X</sub></b></td>
    <td align="center"><b><sub>i7-3930K</sub></b></td>
    <td align="center"><b><sub>i7-4930K</sub></b></td>
    <td align="center"><b><sub>i7-7820X</sub></b></td>
  </tr>
  <tr>
    <td><b><a href="https://www.cubic.org/docs/download/radix_ar_2011.cpp"><sub>Reinald-Harris-Rohrer Sort (LSD)</sub></a></b></td>
    <td align="center"><sub>2011</sub></td>
    <td align="center"><sub>24.6</sub></td>
    <td align="center"><sub>24.7</sub></td>
    <td align="center"><sub>39.3</sub></td>
    <td align="center" colspan="3" rowspan="6"><b><sub>cannot run</sub></b></td>
  </tr>
  <tr>
    <td><b><a href="http://www.cs.columbia.edu/~orestis/sigmod14I.pdf"><sub>Columbia Sort (LSD)</sub></a></b></td>
    <td align="center"><sub>2014</sub></td>
    <td align="center"><sub>23.8</sub></td>
    <td align="center"><sub>26.4</sub></td>
    <td align="center"><sub>42.0</sub></td>
  </tr>
  <tr>
    <td><b><a href="https://github.com/ramou/dfr"><sub>Diverting Fast Radix (LSD)</sub></a></b></td>
    <td align="center"><sub>2016</sub></td>
    <td align="center"><sub>19.2</sub></td>
    <td align="center"><sub>23.3</sub></td>
    <td align="center"><sub>34.0</sub></td>
  </tr>
  <tr>
    <td><b><a href="https://github.com/refresh-bio/RADULS"><sub>RADULS (MSD)</sub></a></b></td>
    <td align="center"><sub>2017</sub></td>
    <td align="center"><sub>25.2</sub></td>
    <td align="center"><sub>29.4</sub></td>
    <td align="center"><sub>41.0</sub></td>
  </tr>
  <tr>
    <td><b><a href="https://github.com/refresh-bio/RADULS"><sub>RADULS2 (MSD)</sub></a></b></td>
    <td align="center"><b><sub>2017</sub></b></td>
    <td align="center"><b><sub>43.6</sub></b></td>
    <td align="center"><b><sub>58.2</sub></b></td>
    <td align="center"><b><sub>67.0</sub></b></td>
  </tr>
  <tr>
    <td><b><a href="https://software.intel.com/content/www/us/en/develop/tools/integrated-performance-primitives.html"><sub>Intel IPP</sub></a></b></td>
    <td align="center"><sub>2019</sub></td>
    <td align="center"><sub>9.3</sub></td>
    <td align="center"><sub>9.8</sub></td>
    <td align="center"><sub>42.3</sub></td>
  </tr>
</tbody>
</table>

<table align="center">
<thead>
  <tr>
    <th colspan="8"><b>In-Place Sort Speeds (M keys/s, 64-bit keys)</b></th>
  </tr>
</thead>
<tbody>
  <tr>
    <td align="center" rowspan="2"><b><sub>Sort</sub></b></td>
    <td align="center" rowspan="2"><b><sub>Year</sub></b></td>
    <td align="center" colspan="3"><b><sub>8 GB of keys</sub></b></td>
    <td align="center" colspan="3"><b><sub>24 GB of keys</sub></b></td>
  </tr>
  <tr>
    <td align="center"><b><sub>i7-3930K</sub></b></td>
    <td align="center"><b><sub>i7-4930K</sub></b></td>
    <td align="center"><b><sub>i7-7820X</sub></b></td>
    <td align="center"><b><sub>i7-3930K</sub></b></td>
    <td align="center"><b><sub>i7-4930K</sub></b></td>
    <td align="center"><b><sub>i7-7820X</sub></b></td>
  </tr>
  <tr>
    <td><b><a href="https://github.com/gorset/radix"><sub>Gorset Sort (MSD)</sub></a></b></td>
    <td align="center"><sub>2011</sub></td>
    <td align="center"><sub>17.4</sub></td>
    <td align="center"><sub>17.9</sub></td>
    <td align="center"><sub>22.0</sub></td>
    <td align="center"><sub>20.7</sub></td>
    <td align="center"><sub>21.7</sub></td>
    <td align="center"><sub>25.8</sub></td>
  </tr>
  <tr>
    <td><b><a href="http://www.cs.columbia.edu/~orestis/sigmod14I.pdf"><sub>Columbia Sort (MSD)</sub></a></b></td>
    <td align="center"><sub>2014</sub></td>
    <td align="center"><sub>19.2</sub></td>
    <td align="center"><sub>22.5</sub></td>
    <td align="center"><sub>26.4</sub></td>
    <td align="center"><sub>18.4</sub></td>
    <td align="center"><sub>21.2</sub></td>
    <td align="center"><sub>26.0</sub></td>
  </tr>
  <tr>
    <td><b><a href="https://github.com/skarupke/ska_sort"><sub>Ska Sort (MSD)</sub></a></b></td>
    <td align="center"><sub>2017</sub></td>
    <td align="center"><sub>24.1</sub></td>
    <td align="center"><sub>25.2</sub></td>
    <td align="center"><sub>31.5</sub></td>
    <td align="center"><sub>23.9</sub></td>
    <td align="center"><sub>25.9</sub></td>
    <td align="center"><sub>31.6</sub></td>
  </tr>
  <tr>
    <td><b><a href="https://github.com/SaschaWitt/ips4o"><sub>IPS4o (Quicksort)</sub></a></b></td>
    <td align="center"><sub>2017</sub></td>
    <td align="center"><sub>21.8</sub></td>
    <td align="center"><sub>23.1</sub></td>
    <td align="center"><sub>27.6</sub></td>
    <td align="center"><sub>21.3</sub></td>
    <td align="center"><sub>22.4</sub></td>
    <td align="center"><sub>26.9</sub></td>
  </tr>
  <tr>
    <td><b><a href="https://github.com/omarobeya/parallel-inplace-radixsort"><sub>Regions Sort (MSD)</sub></a></b></td>
    <td align="center"><sub>2019</sub></td>
    <td align="center"><sub>16.8</sub></td>
    <td align="center"><sub>19.3</sub></td>
    <td align="center"><sub>26.2</sub></td>
    <td align="center"><sub>25.3</sub></td>
    <td align="center"><sub>29.7</sub></td>
    <td align="center"><sub>38.6</sub></td>
  </tr>
  <tr>
    <td><b><a href="https://en.cppreference.com/w/cpp/algorithm/sort"><sub>std::sort</sub></a></b></td>
    <td align="center"><sub>2019</sub></td>
    <td align="center"><sub>9.5</sub></td>
    <td align="center"><sub>9.7</sub></td>
    <td align="center"><sub>11.6</sub></td>
    <td align="center"><sub>9.0</sub></td>
    <td align="center"><sub>9.2</sub></td>
    <td align="center"><sub>10.9</sub></td>
  </tr>
  <tr>
    <td><b><a href="https://software.intel.com/content/www/us/en/develop/tools/threading-building-blocks.html"><sub>Intel TBB</sub></a></b></td>
    <td align="center"><sub>2019</sub></td>
    <td align="center"><sub>8.5</sub></td>
    <td align="center"><sub>8.6</sub></td>
    <td align="center"><sub>10.7</sub></td>
    <td align="center"><sub>8.0</sub></td>
    <td align="center"><sub>8.2</sub></td>
    <td align="center"><sub>10.1</sub></td>
  </tr>
  <tr>
    <td><b><sub>Vortex Sort (MSD)</sub></b></td>
    <td align="center"><b><sub>2020</sub></b></td>
    <td align="center"><b><sub>71</sub></b></td>
    <td align="center"><b><sub>84</sub></b></td>
    <td align="center"><b><sub>127</sub></b></td>
    <td align="center"><b><sub>68</sub></b></td>
    <td align="center"><b><sub>80</sub></b></td>
    <td align="center"><b><sub>121</sub></b></td>
  </tr>
</tbody>
</table>

<table align="center">
<thead>
  <tr>
    <th colspan="8"><b>Batched Producer-Consumer Rate (GB/s)</b></th>
  </tr>
  <tr>
    <td rowspan="2" align="center"><b><sub>Framework</sub></b></td>
    <td colspan="3" align="center"><b><sub>Two cores</sub></b></td>
    <td colspan="3" align="center"><b><sub>All cores</sub></b></td>
    <td rowspan="2" align="center"><b><sub>RAM</sub></b></td>
  </tr>
  <tr>
    <td align="center"><b><sub>i7-3930K</sub></b></td>
    <td align="center"><b><sub>i7-4930K</sub></b></td>
    <td align="center"><b><sub>i7-7820X</sub></b></td>
    <td align="center"><b><sub>i7-3930K</sub></b></td>
    <td align="center"><b><sub>i7-4930K</sub></b></td>
    <td align="center"><b><sub>i7-7820X</sub></b></td>
  </tr>
</thead>
<tbody>
  <tr>
    <td><b><a href="https://storm.apache.org/"><sub>Storm</sub></b></td>
    <td align="center"><sub>1.7</sub></td>
    <td align="center"><sub>1.4</sub></td>
    <td align="center"><sub>2.4</sub></td>
    <td align="center"><sub>11.1</sub></td>
    <td align="center"><sub>9.5</sub></td>
    <td align="center"><sub>12.8</sub></td>
    <td align="center"><sub>1.6 GB</sub></td>
  </tr>
  <tr>
    <td><b><a href="https://github.com/TimelyDataflow/timely-dataflow"><sub>Naiad</sub></b></td>
    <td align="center"><sub>2.7</sub></td>
    <td align="center"><sub>3.1</sub></td>
    <td align="center"><sub>4.4</sub></td>
    <td align="center"><sub>7.4</sub></td>
    <td align="center"><sub>7.9</sub></td>
    <td align="center"><sub>13.1</sub></td>
    <td align="center"><sub>65 MB</sub></td>
  </tr>
  <tr>
    <td><b><sub>Queue of Blocks</sub></b></td>
    <td align="center"><sub>6.4</sub></td>
    <td align="center"><sub>7.3</sub></td>
    <td align="center"><sub>11.4</sub></td>
    <td align="center"><sub>17.1</sub></td>
    <td align="center"><sub>16.5</sub></td>
    <td align="center"><sub>24.8</sub></td>
    <td align="center"><sub>24 MB</sub></td>
  </tr>
  <tr>
    <td><b><sub>Vortex-B</sub></b></td>
    <td align="center"><sub>4.3</sub></td>
    <td align="center"><sub>4.4</sub></td>
    <td align="center"><sub>4.6</sub></td>
    <td align="center"><sub>5.1</sub></td>
    <td align="center"><sub>5.2</sub></td>
    <td align="center"><sub>3.9</sub></td>
    <td align="center"><sub>9 MB</sub></td>
  </tr>
  <tr>
    <td><b><sub>Vortex-C</sub></b></td>
    <td align="center"><b><sub>13.5</sub></b></td>
    <td align="center"><b><sub>16.4</sub></b></td>
    <td align="center"><b><sub>23.3</sub></b></td>
    <td align="center"><b><sub>38.3</sub></b></td>
    <td align="center"><b><sub>38.4</sub></b></td>
    <td align="center"><b><sub>65.4</sub></b></td>
    <td align="center"><b><sub>9 MB</sub></b></td>
  </tr>
</tbody>
</table>

<table align="center">
<thead>
  <tr>
    <th colspan="7"><b>File I/O Speed (MB/s)</b></th>
  </tr>
</thead>
<tbody>
  <tr>
    <td rowspan="2" align="center"><b><sub>Framework</sub></b></td>
    <td colspan="2" align="center"><b><sub>24-disk RAID</sub></b></td>
    <td colspan="2" align="center"><b><sub>Samsung Evo 960 SSD</sub></b></td>
    <td rowspan="2" align="center"><b><sub>CPU</sub></b></td>
    <td rowspan="2" align="center"><b><sub>RAM</sub></b></td>
  </tr>
  <tr>
    <td align="center"><b><sub>read</sub></b></td>
    <td align="center"><b><sub>write</sub></b></td>
    <td align="center"><b><sub>read</sub></b></td>
    <td align="center"><b><sub>write</sub></b></td>
  </tr>
  <tr>
    <td><b><a href="https://en.cppreference.com/w/cpp/io/basic_fstream"><sub>std::fstream</sub></b></td>
    <td align="center"><sub>43</sub></td>
    <td align="center"><sub>88</sub></td>
    <td align="center"><sub>51</sub></td>
    <td align="center"><sub>140</sub></td>
    <td align="center"><sub>8%</sub></td>
    <td align="center"><sub>2 MB</sub></td>
  </tr>
  <tr>
    <td><b><a href="https://docs.microsoft.com/en-us/windows/win32/api/memoryapi/nf-memoryapi-mapviewoffile"><sub>Windows MapViewOfFile</sub></b></td>
    <td align="center"><sub>69</sub></td>
    <td align="center"><sub>147</sub></td>
    <td align="center"><sub>1,161</sub></td>
    <td align="center"><sub>*</sub></td>
    <td align="center"><sub>8%</sub></td>
    <td align="center"><sub>32 GB</sub></td>
  </tr>
  <tr>
    <td><b><a href="https://man7.org/linux/man-pages/man2/mmap.2.html"><sub>Linux mmap</sub></b></td>
    <td align="center"><sub>1,892</sub></td>
    <td align="center"><sub>1,170</sub></td>
    <td align="center"><sub>1,917</sub></td>
    <td align="center"><sub>641</sub></td>
    <td align="center"><sub>3%</sub></td>
    <td align="center"><sub>30 GB</sub></td>
  </tr>
  <tr>
    <td><b><sub>Vortex-A</sub></b></td>
    <td align="center"><sub>2,235</sub></td>
    <td align="center"><sub>1,547</sub></td>
    <td align="center"><sub>1,272</sub></td>
    <td align="center"><sub>651</sub></td>
    <td align="center"><sub>8%</sub></td>
    <td align="center"><sub>5 MB</sub></td>
  </tr>
  <tr>
    <td><b><sub>Vortex-B</sub></b></td>
    <td align="center"><sub>2,231</sub></td>
    <td align="center"><sub>2,394</sub></td>
    <td align="center"><sub>3,211</sub></td>
    <td align="center"><sub>650</sub></td>
    <td align="center"><sub>8%</sub></td>
    <td align="center"><sub>5 MB</sub></td>
  </tr>
  <tr>
    <td><b><sub>Vortex-C</sub></b></td>
    <td align="center"><b><sub>2,238</sub></b></td>
    <td align="center"><b><sub>2,399</sub></b></td>
    <td align="center"><b><sub>3,266</sub></b></td>
    <td align="center"><b><sub>674</sub></b></td>
    <td align="center"><b><sub>1%</sub></b></td>
    <td align="center"><b><sub>5 MB</sub></b></td>
  </tr>
</tbody>
</table>

## Getting Started

Vortex can be compiled on Windows and Linux, each explained in a separate section below.

### Windows

***Recommended Setup:***
- **OS:** [**Windows Server 2016**](https://www.microsoft.com/en-us/evalcenter/evaluate-windows-server-2016/)
- **Compiler:** [**Visual Studio 2019 v16.3.10**](https://docs.microsoft.com/en-us/visualstudio/releases/2019/release-notes-v16.3)

1. **For compilation**, VS 2019 16.3.10 ***or earlier*** is recommended. Later versions of the compiler have problems avoiding conditional branches in sorting networks, for which we implemented a fix; however, there is no guarantee that future versions of VS won't break. A more long-term solution would be conversion of sorting networks to assembly, which we may release in the future.

2. **Before compiling** on Visual Studio make sure to check that the project is set for **x64 Release**.

3. **To manage memory in userspace,** Windows requires that the [**SE_LOCK privilege be enabled**](https://docs.microsoft.com/en-us/sql/database-engine/configure-windows/enable-the-lock-pages-in-memory-option-windows?view=sql-server-ver15). It is *not* sufficient to just be an administrator.

4. **The code itself must run with elevated privileges,** which can be done by any of the following: a) running Visual Studio as administrator; b) right clicking the executable generated by Visual Studio and selecting "Run as Administrator"; or c) disabling the [**Run All Administrators in Admin Approval Mode**](https://docs.microsoft.com/en-us/windows/security/threat-protection/security-policy-settings/user-account-control-run-all-administrators-in-admin-approval-mode) local security policy. Additionally, the project manifest can be modified to [**automatically execute**](https://docs.microsoft.com/en-us/cpp/build/reference/manifestuac-embeds-uac-information-in-manifest?view=vs-2019) the program with admin permissions.

5. **For maximum speed,** Windows Server 2016 ***or older*** is recommended. In newer kernels, [**MapUserPhysicalPages()**](https://docs.microsoft.com/en-us/windows/win32/api/memoryapi/nf-memoryapi-mapuserphysicalpages) (i.e., the kernel API utilized by Vortex to map/unmap pages) is much slower, e.g., by 50% in Server 2019 (build 17763) and 60% in Windows 10 (build 19035). Additionally, both of these operating systems experience memory "rot" after some uptime, which causes memory-intensive benchmarks (e.g., producer-consumer, sorting) to take a ~30% performance hit. This can be fixed by rebooting the system.

### Linux

***Recommended Setup:***
- **OS:** [**Ubuntu v18.10**](http://old-releases.ubuntu.com/releases/18.10/)
- **Compiler:** [**GCC v9.3**](https://linuxconfig.org/how-to-install-gcc-the-c-compiler-on-ubuntu-18-04-bionic-beaver-linux)

1. **If earlier versions of gcc than v9.3 are used,** \_xgetbv() may need to be [**invoked manually via inline assembly**](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=71659). This function is used to detect presence of AVX, but is not critical to the rest of Vortex. It can thus be commented out and cpuId.avx be hardcoded to either true or false.

2. **Linux mremap() does not support remapping across VMA**s. This leads to a small increase in memory overhead during sorting compared to Windows.

3. **Due to time limitations,** file I/O benchmarks have not been ported to Linux due to the extra effort needed to rewrite the overlapped I/O model of Windows.

4. **Vortex can be compiled and run via the commandline** with the included makefile in directory Vortex-1.0/Vortex. Simply move the folder Vortex-1.0/Vortex onto a Linux server, and *cd* to the Vortex directory. 

```
$ cd Vortex
```

5. **From within the directory**, issue the *make* command. 

```
$ make
```

6. **All done, and ready to run!** Once Vortex is compiled, enter command ./Vortex into the commandline to view usage.

```
$ ./Vortex
```

## Additional Considerations

1. **Mitigation of Spectre/Meltdown** bugs affects the speed of Vortex bucket sort and various memory-intensive benchmarks. As such, it is recommended that these features be disabled to achieve peak performance.

2. **The bucket sort prefers to use AVX offload** during write-combine for a small gain in speed; however, certain motherboards run AVX at lower frequencies, which can make AVX slower than SSE. One option for overcoming this is to set the AVX offset to zero in BIOS. The other is to assign cpuId.avx = false and let Vortex fall back to SSE intrinsics.

3. **If sorting large inputs (e.g., 256 GB),** two levels of write-combine might be beneficial, especially on multi-socket Xeons; however, this is currently not implemented. Additionally, big arrays may need 5-level paging (e.g., Intel Ice Lake) to avoid running out of virtual memory. Future work will look into this further.

4. **While the main aim of Vortex sort is multi-GB datasets,** small inputs (e.g., a few hundred bytes) are supported; however, they are not meant to be top-speed or memory-efficient.

5. **For completeness,** the code is templated to handle 8/16/32-bit keys in addition to regular 64-bit keys, but these use cases are not well-developed, extensively-tested, or super-efficient.

6. **Vortex sort can handle non-uniform keys** without overflowing memory or running into problems; however, the nature of MSD radix sort guarantees that skewed distributions will incur a performance drop. Future work will examine how to overcome these issues and offer additional improvements (e.g., sorting key-value pairs & variable-size keys).

## License

This project is licensed under the GPLv3.0 License - see the [**LICENSE**](LICENSE) file for details

## Authors

[**Carson Hanel**](http://irl.cs.tamu.edu/people/carson/), [**Arif Arman**](http://irl.cs.tamu.edu/people/arif/), [**Di Xiao**](http://irl.cs.tamu.edu/people/di/), [**John Keech**](http://irl.cs.tamu.edu/people/), [**Dmitri Loguinov**](http://irl.cs.tamu.edu/people/dmitri/)
