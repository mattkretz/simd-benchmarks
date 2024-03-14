/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright © 2023-2024 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
 *                       Matthias Kretz <m.kretz@gsi.de>
 */

#include "bench.h"

alignas(64) char mem[64 * 64] = {};

template <typename T>
  static T value = {};

template <typename T>
  inline void
  store(T& x, size_t offset = 0)
  {
    using Mem = value_type_t<T>;
    Mem* ptr = reinterpret_cast<Mem*>(mem + offset);
    fake_modify(x);
#if USE_STD_SIMD
    if constexpr (std::is_simd_v<T>)
      x.copy_to(ptr);
#else
    if constexpr (stdx::is_simd_v<T>)
      x.copy_to(ptr, stdx::element_aligned);
#endif
    else if constexpr (std::is_arithmetic_v<T>)
      ptr[0] = x;
    else
      std::memcpy(ptr, &x, sizeof(x));
    asm(""::"m"(*mem));
  }

template <int Special>
  struct Benchmark<Special>
  {
    static constexpr Info<1> info = {"Throughput"};

    template <class V>
      static Times<1>
      run()
      {
        return {
          0.0625 * time_mean2<400'000>([](auto& need_more) {
                     V obj = {};
                     while (need_more)
                       {
                         store<V>(obj, 0 * sizeof(V));
                         store<V>(obj, 1 * sizeof(V));
                         store<V>(obj, 2 * sizeof(V));
                         store<V>(obj, 3 * sizeof(V));
                         store<V>(obj, 4 * sizeof(V));
                         store<V>(obj, 5 * sizeof(V));
                         store<V>(obj, 6 * sizeof(V));
                         store<V>(obj, 7 * sizeof(V));
                         store<V>(obj, 8 * sizeof(V));
                         store<V>(obj, 9 * sizeof(V));
                         store<V>(obj, 10 * sizeof(V));
                         store<V>(obj, 11 * sizeof(V));
                         store<V>(obj, 12 * sizeof(V));
                         store<V>(obj, 13 * sizeof(V));
                         store<V>(obj, 14 * sizeof(V));
                         store<V>(obj, 15 * sizeof(V));
                       }
                   })
        };
      }
  };

int
main()
{
  bench_all<char>();
  bench_all<short>();
  bench_all<int>();
  bench_all<long long>();
  bench_all<float>();
  bench_all<double>();
}
