/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright © 2019-2023 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
 *                       Matthias Kretz <m.kretz@gsi.de>
 */

#include "bench.h"

FUN2(scalbn) Scalbn;

template <class What>
  struct Benchmark<What> : DefaultBenchmark
  {
    template <class T>
      [[gnu::flatten]]
      static Times<2>
      run()
      {
        T inputs[64] = {};
        for (int i = 0; i < 64; ++i)
          inputs[i] = T() + i * .83f - 16;
        unsigned i = 0;
        int n = 8;
        fake_modify(n);
        if constexpr (Latency)
          {
            T a = inputs[i];
            return time_mean<5'000'000>([&]() {
                     T r = What::apply(a, n);
                     fake_read(r);
                     if constexpr (stdx::is_simd_v<T>)
                       {
                         a = -1.e38f;
                         where(r < 1.e38f, a) = r;
                       }
                     else
                       a = r < 1.e38f ? r : -1.e38f;
                   });
          }
        else
          {
            T a0 = inputs[0];
            T a1 = inputs[1];
            T a2 = inputs[2];
            T a3 = inputs[3];
            int n0 = 8,
                n1 = -9,
                n2 = 100,
                n3 = 2000;
            fake_modify(n0, n1, n2, n3);
            return 0.25 * time_mean<5'000'000>([&]() {
                            T r0 = What::apply(a0, n0);
                            T r1 = What::apply(a1, n1);
                            T r2 = What::apply(a2, n2);
                            T r3 = What::apply(a3, n3);
                            fake_read(r0, r1, r2, r3);
                            fake_modify(a0, a1, a2, a3);
                          });
          }
      }
  };

int
main()
{
  bench_all<float, Scalbn>();
  bench_all<double, Scalbn>();
}
