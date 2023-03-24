/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright © 2019-2023 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
 *                       Matthias Kretz <m.kretz@gsi.de>
 */

#include "bench.h"

struct VecInit
{
  template <class T>
    static constexpr T
    rhs_init()
    {
      if constexpr (std::experimental::is_simd_v<T>)
        return T([](auto i) constexpr { return (int(i) % 31) + 1; });
      else if constexpr (std::experimental::__is_vector_type_v<T>)
        return std::experimental::__generate_vector<T>(
                 [](auto i) constexpr { return (int(i) % 31) + 1; });
      else
        return 6;
    }
};

struct IntInit
{
  template <class>
    static constexpr int
    rhs_init()
    { return 6; }
};

template <bool X>
  struct Left
  { static constexpr bool left = X; };

template <bool X>
  struct Const
  { static constexpr bool rhs_const = X; };

struct LeftVec
: Left<true>, Const<false>, VecInit
{ static constexpr char name[9] = "`x << v`"; };

struct LeftInt
: Left<true>, Const<false>, IntInit
{ static constexpr char name[9] = "`x << i`"; };

struct LeftVecConst
: Left<true>, Const<true>, VecInit
{ static constexpr char name[15] = "`x << const v`"; };

struct LeftIntConst
: Left<true>, Const<true>, IntInit
{ static constexpr char name[9] = "`x << 6`"; };

struct RightVec
: Left<false>, Const<false>, VecInit
{ static constexpr char name[9] = "`x >> v`"; };

struct RightInt
: Left<false>, Const<false>, IntInit
{ static constexpr char name[9] = "`x >> i`"; };

struct RightVecConst
: Left<false>, Const<true>, VecInit
{ static constexpr char name[15] = "`x >> const v`"; };

struct RightIntConst
: Left<false>, Const<true>, IntInit
{ static constexpr char name[9] = "`x >> 6`"; };

template <class What>
  struct Benchmark<What> : DefaultBenchmark
  {
    template <bool Latency, class T>
      [[gnu::noinline]]
      static double
      do_benchmark()
      {
        T a_init = T() + 23;
        auto b_init = What::template rhs_init<T>();
        if constexpr (Latency)
          return 0.25 * time_mean2<4'000'000, 10>([&](auto& need_more)
          {
            auto a = a_init;
            auto b = What::template rhs_init<T>();
            while (need_more)
              {
                fake_modify(a);
                if constexpr (not What::rhs_const)
                  fake_modify(b);
                a = What::left ? a << b : a >> b;
                fake_modify(a);
                if constexpr (not What::rhs_const)
                  fake_modify(b);
                a = What::left ? a << b : a >> b;
                fake_modify(a);
                if constexpr (not What::rhs_const)
                  fake_modify(b);
                a = What::left ? a << b : a >> b;
                fake_modify(a);
                if constexpr (not What::rhs_const)
                  fake_modify(b);
                a = What::left ? a << b : a >> b;
              }
            fake_read(a);
          });
        else
          return 0.125 * time_mean2<3'000'000, 10>([&](auto& need_more)
          {
            while (need_more)
              {
                auto a = a_init;
                auto b = What::template rhs_init<T>();
                fake_modify(a);
                if constexpr (not What::rhs_const)
                  fake_modify(b);
                const T r0 = What::left ? a << b : a >> b;
                fake_modify(a);
                if constexpr (not What::rhs_const)
                  fake_modify(b);
                const T r1 = What::left ? a << b : a >> b;
                fake_modify(a);
                if constexpr (not What::rhs_const)
                  fake_modify(b);
                const T r2 = What::left ? a << b : a >> b;
                fake_modify(a);
                if constexpr (not What::rhs_const)
                  fake_modify(b);
                const T r3 = What::left ? a << b : a >> b;
                fake_modify(a);
                if constexpr (not What::rhs_const)
                  fake_modify(b);
                const T r4 = What::left ? a << b : a >> b;
                fake_modify(a);
                if constexpr (not What::rhs_const)
                  fake_modify(b);
                const T r5 = What::left ? a << b : a >> b;
                fake_modify(a);
                if constexpr (not What::rhs_const)
                  fake_modify(b);
                const T r6 = What::left ? a << b : a >> b;
                fake_modify(a);
                if constexpr (not What::rhs_const)
                  fake_modify(b);
                const T r7 = What::left ? a << b : a >> b;
                fake_read(r0, r1, r2, r3, r4, r5, r6, r7);
              }
          });
      }

    template <class T>
      [[gnu::flatten]]
      static Times<2>
      run()
      { return {do_benchmark<true, T>(), do_benchmark<false, T>()}; }
  };

template <class T>
  void
  all_shifts()
  {
    bench_all<T, LeftVec>();
    bench_all<T, LeftVecConst>();
    bench_all<T, LeftInt>();
    bench_all<T, LeftIntConst>();
    bench_all<T, RightVec>();
    bench_all<T, RightVecConst>();
    bench_all<T, RightInt>();
    bench_all<T, RightIntConst>();
  }

template <class What>
  void
  all_integers()
  {
    bench_all<signed char, What>();
    bench_all<unsigned char, What>();
    bench_all<signed short, What>();
    bench_all<unsigned short, What>();
    bench_all<signed int, What>();
    bench_all<unsigned int, What>();
    bench_all<signed long, What>();
    bench_all<unsigned long, What>();
  }

int
main()
{
  all_integers<LeftVec>();
  all_integers<LeftVecConst>();
  all_integers<LeftInt>();
  all_integers<LeftIntConst>();
  all_integers<RightVec>();
  all_integers<RightVecConst>();
  all_integers<RightInt>();
  all_integers<RightIntConst>();
}
