# igzip wrapper

igzip provides the **fastest** zlib/gzip-compatible compression/decompression in x86 CPU around the world so far (Oct 27, 2021), which is a submodule of [Intel(R) Intelligent Storage Acceleration Library (ISA-L)](https://github.com/intel/isa-l) optimized by many low-level magics including assembly language and AVX512 instructions, to gain best performance especially for Intel(R) x86 platform. Here's the brief intro:

* Supports [RFC 1951](https://datatracker.ietf.org/doc/html/rfc1951) DEFLATE standard like canonical Zlib.
* 4 levels of compression, which affect operation performance and compression ratio.
* Multi-threading for compression, maximum 8 threads could be used.
* Optimized by low-level instructions to meet performance-critical scenarios.

For more details of Zlib solutions of ISA-L, please see here: [Zlib Solutions of Intel(R) ISA-L and Intel(R) IPP](https://www.intel.com/content/www/us/en/developer/articles/technical/intel-isa-l-and-intel-integrated-performance-primitives-zlib-solutions.html).

To provide out-of-the-box compression/decompression functions, we proposed the *igzip wrapper*, which supports the direct transformation between **C-style string** and **gzip file**, based on the awesome ISA-L.

## Supported API

```c++
/* igzip inflate wrapper */
int decompress_file(const char *infile_name, unsigned char *output_string, size_t *output_length);

/* igzip deflate wrapper */
int compress_file(unsigned char *input_string, size_t input_length, const char *outfile_name, int compress_level, int thread_num);
```

## Build

### Prerequisites

* CMake v3.2 or later
* ISA-L v2.30.0 or later
* pthreads

### Build shared library

```bash
git clone git@github.com:ueqri/igzip-wrapper.git
mkdir -p igzip-wrapper/build
cd igzip-wrapper/build
cmake ..
make -j
```

**Note:** the multi-threading support for deflating (i.e. compression) is enabled by default, if you want to build **single thread version**, please add the option like `cmake -DMULTI_THREADED_DEFLATE=OFF ..` instead. As for inflating, only single thread is supported restricted by the nature of gzip format.

### Link the library with your program

* Copy `igzip_wrapper.h` and `libigzipwrap.so` to your program.
* Include the wrapper header in the C/C++ source.
* Link the library when building the program.

## Test

If you want to build tests for inflate & deflate APIs and see the time cost in your machine, replace the previous `cmake ..` with the following command.

```bash
  cmake -DBUILD_TEST=ON ..
```

Usage of the test executables in `build` directory:

```bash
# Test decompression with check file
./inflate <path-to-source-gzip-file> <path-to-uncompressed-check-file>
```

```bash
# Test compression and use inflate API to check
./deflate <path-to-uncompressed-source> <path-to-output-gzip-file>
```

## Benchmark

[A series of comprehensive benchmarks](https://bugs.python.org/issue41566) were done by Ruben Vorderman (thanks @rhpvorderman) of Python community.

<details>
<summary>Details of the benchmarks</summary>

The system was based on Ryzen 5 3600 with 2x16GB DDR4-3200 memory, and running Debian 10.

All benchmarks were performed on a **tmpfs** which lives in memory to prevent I/O bottlenecks, and using [hyperfine](https://github.com/sharkdp/hyperfine) for better analysis.

The test file was a 5 million read [FASTQ file](https://en.wikipedia.org/wiki/FASTQ_format) of 1.6 GB . These type of files are common in bioinformatics at 100+ GB sizes so are a good real-world benchmark.

Also benchmarked [pigz](https://github.com/madler/pigz) on one thread as well, as it implements zlib but in a faster way than gzip. [Zstd](https://github.com/facebook/zstd) was benchmarked as a comparison.

```text
Versions: 
gzip 1.9 (provided by debian)
pigz 2.4 (provided by debian)
igzip 2.25.0 (provided by debian)
libdeflate-gzip 1.6 (compiled by conda-build with the recipe here: https://github.com/conda-forge/libdeflate-feedstock/pull/4)
zstd 1.3.8 (provided by debian)
```

**Compression:** By default level 1 is chosen for all compression benchmarks. Time is average over 10 runs.

```text
COMPRESSION
program            time           size   memory
gzip               23.5 seconds   657M   1.5M
pigz (one thread)  22.2 seconds   658M   2.4M
libdeflate-gzip    10.1 seconds   623M   1.6G (reads entire file in memory)
igzip              4.6 seconds    620M   3.5M
zstd (to .zst)     6.1 seconds    584M   12.1M
```

**Decompression:** All programs decompressed the file created using gzip -1. (Even zstd which can also decompress gzip).

```text
DECOMPRESSION
program            time           memory
gzip               10.5 seconds   744K
pigz (one-thread)  6.7 seconds    1.2M
libdeflate-gzip    3.6 seconds    2.2G (reads in mem before writing)
igzip              3.3 seconds    3.6M
zstd (from .gz)    6.4 seconds    2.2M
zstd (from .zst)   2.3 seconds    3.1M
```

As shown from the above benchmarks, using Intel's Storage Acceleration Libraries may improve performance quite substantially. Offering very fast compression and decompression. This gets igzip in the zstd ballpark in terms of speed while still offering backwards compatibility with gzip.

</details>

From my side, after controlling for other variables, I found the decompression speedup of *igzip(8 threads, v2.30.1)* is ~2x faster than *Zstd(v1.5.0)*, and ~4x faster than *gzip(v1.11)*. And the optimization of compression is even much higher.

## Reference

[Zlib Solutions of Intel(R) Intelligent Storage Acceleration Library and Intel(R) Integrated Performance Primitives](https://www.intel.com/content/www/us/en/developer/articles/technical/intel-isa-l-and-intel-integrated-performance-primitives-zlib-solutions.html)

[Include much faster DEFLATE implementations ISA-L in Python's gzip and zlib libraries](https://bugs.python.org/issue41566)

[Storage acceleration with ISA-L (From Page 9)](https://storageconference.us/2017/Presentations/Tucker-1.pdf)

[ISA-L Update & Usercases Sharing (From Page 17)](https://ci.spdk.io/download/events/2018-summit-prc/08_Liu_Xiaodong_&_Hui_Chunyang_ISA-L_Update_and_Usercase_Sharing_SPDK_Summit_2018_China.pdf)

[lzbench issue of adding ISA-L to comparison](https://github.com/inikep/lzbench/issues/94#issue-794863959)
