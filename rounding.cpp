/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright © 2019-2023 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
 *                       Matthias Kretz <m.kretz@gsi.de>
 */

#include <random>
#include "bench.h"

struct Floor { static constexpr char name[] = "std::floor"; };
struct Ceil { static constexpr char name[] = "std::ceil"; };
struct Round { static constexpr char name[] = "std::round"; };
struct Rint { static constexpr char name[] = "std::rint"; };
struct Nearbyint { static constexpr char name[] = "std::nearbyint"; };

MAKE_VECTORMATH_OVERLOAD(floor)
MAKE_VECTORMATH_OVERLOAD(ceil)
MAKE_VECTORMATH_OVERLOAD(round)
MAKE_VECTORMATH_OVERLOAD(rint)
MAKE_VECTORMATH_OVERLOAD(nearbyint)

static std::mt19937 rnd_gen = std::mt19937(1);

template <class T>
  T
  random()
  {
    if constexpr (std::is_floating_point_v<T>)
      {
        std::uniform_real_distribution<T> dis(-10.0, 20.0);
        return dis(rnd_gen);
      }
    else if constexpr (std::experimental::is_simd_v<T>) 
      return T([](int) { return random<typename T::value_type>(); });
    else
      {
        T r = {};
        for (int i = 0; i < sizeof(T) / sizeof(r[0]); ++i)
          r[i] = random<std::remove_reference_t<decltype(r[0])>>();
        return r;
      }
  }

template <bool Latency, class T, class What>
  double
  benchmark()
  {
    std::vector<T> input;
    input.reserve(1024);
    for (int n = input.capacity(); n; --n)
      input.push_back(random<T>());
    T a = T();
    int i = 0;
    return time_mean<5'000'000>([&]() {
             a += input[i];
             i = (i + 1) % input.size();
             fake_modify(a);
             using ::floor;
             using ::ceil;
             using ::round;
             using ::rint;
             using ::nearbyint;
             using std::floor;
             using std::ceil;
             using std::round;
             using std::rint;
             using std::nearbyint;
             T r;
             if constexpr (std::is_same_v<What, Floor>)
               r = floor(a);
             else if constexpr (std::is_same_v<What, Ceil>)
               r = ceil(a);
             else if constexpr (std::is_same_v<What, Round>)
               r = round(a);
             else if constexpr (std::is_same_v<What, Rint>)
               r = rint(a);
             else if constexpr (std::is_same_v<What, Nearbyint>)
               r = nearbyint(a);
             if constexpr (Latency)
               a = r;
             else
               {
                 a = T();
                 fake_read(r);
               }
           });
  }

int
main()
{
  bench_all<float, Floor    >();
  bench_all<float, Ceil     >();
  bench_all<float, Round    >();
  bench_all<float, Rint     >();
  bench_all<float, Nearbyint>();
  bench_all<double, Floor    >();
  bench_all<double, Ceil     >();
  bench_all<double, Round    >();
  bench_all<double, Rint     >();
  bench_all<double, Nearbyint>();
}
