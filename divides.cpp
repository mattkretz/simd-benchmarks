/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright © 2019-2024 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
 *                       Matthias Kretz <m.kretz@gsi.de>
 */

#include "bench.h"
#include <climits>

template <int Special>
  struct Benchmark<Special>
  {
    static constexpr Info<2> info = {"Latency", "Throughput"};

    template <class T>
      [[gnu::flatten]]
      static Times<2>
      run()
      {
        using TT = value_type_t<T>;
        constexpr int nbits = CHAR_BIT * sizeof(TT);
        T a0 = T() + std::numeric_limits<value_type_t<T>>::max();
        T a1 = T() + std::numeric_limits<value_type_t<T>>::max();
        T a2 = T() + std::numeric_limits<value_type_t<T>>::max();
        T a3 = T() + std::numeric_limits<value_type_t<T>>::max();
        if constexpr (std::is_integral_v<TT>)
          {
            a0 <<= nbits/2;
            a1 <<= nbits/2;
            a2 <<= nbits/2;
            a3 <<= nbits/2;
          }
        T b = T() + TT(7);
        return {
          0.25 * time_mean<1'000'000>([&] {
            fake_modify(a0, b);
            T r = a0 / b;
            r = a0 / r;
            r = a0 / r;
            r = a0 / r;
            b = r;
          }),
          0.25 * time_mean<1'000'000>([&] {
            fake_modify(a0, a1, a2, a3, b);
            T r0 = a0 / b;
            T r1 = a1 / b;
            T r2 = a2 / b;
            T r3 = a3 / b;
            fake_read(r0, r1, r2, r3);
          })};
      }
  };

int
main()
{
  bench_all<signed char>();
  bench_all<unsigned char>();
  bench_all<signed short>();
  bench_all<unsigned short>();
  bench_all<signed int>();
  bench_all<unsigned int>();
  bench_all<signed long>();
  bench_all<unsigned long>();
  bench_all<float>();
  bench_all<double>();
}
