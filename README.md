# stdx::simd Benchmarks

This repository collects benchmarks for `std::experimental::simd`. The 
benchmarks are a set of micro-benchmarks that try to capture *latency* and 
*throughput* of individual operations on a `stdx::simd`. For reference, the 
same operation is also benchmarked on the `simd::value_type` itself and on GCC 
vector builtins (using `[[gnu::vector_size(N)]]`).
