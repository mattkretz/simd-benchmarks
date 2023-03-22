/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright © 2019-2023 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
 *                       Matthias Kretz <m.kretz@gsi.de>
 */

#include "bench.h"

MAKE_VECTORMATH_OVERLOAD(hypot)

template <bool Latency, class T>
  double
  benchmark()
  {
    T a = T() + 0x1.fe8222p-10f;
    T b = T() + 0x1.82a4bcp-9f;
    // b = std::numeric_limits<T>::quiet_NaN();
    T c = T() + 0x1.f323e6p2f;
    return time_mean<50'000'000>([&]() {
             using ::hypot;
             using std::hypot;
             using std::experimental::hypot;
             fake_modify(a, b, c);
             T r = hypot(a, b, c);
             if constexpr (Latency)
               a = r;
             else
               fake_read(r);
           });
  }

int
main()
{
  bench_all<float>();
  bench_all<double>();
}
