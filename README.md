# stdx::simd Benchmarks

[![DOI](https://zenodo.org/badge/771078921.svg)](https://zenodo.org/doi/10.5281/zenodo.10810248)
[![fair-software.eu](https://img.shields.io/badge/fair--software.eu-%E2%97%8F%20%20%E2%97%8F%20%20%E2%97%8B%20%20%E2%97%8F%20%20%E2%97%8B-orange)](https://fair-software.eu)

This repository collects benchmarks for `std::experimental::simd`. The 
benchmarks are a set of micro-benchmarks that try to capture *latency* and 
*throughput* of individual operations on a `stdx::simd`. For reference, the 
same operation is also benchmarked on the `simd::value_type` itself and on GCC 
vector builtins (using `[[gnu::vector_size(N)]]`).

## Usage

1. Set `$CXX` to the compiler you want use for the benchmarks. E.g.
```bash
export CXX=/usr/bin/g++-12
```

2. Call `./run.sh --help` to learn how to invoke individual benchmarks.
   The `run.sh` script can be called from any working directory and will not 
   change the working directory. This may be useful for testing uninstalled 
   compilers.
