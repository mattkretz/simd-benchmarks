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

template <bool Latency, class T, class What>
  double
  benchmark()
  {
    T a_init = T() + 23;
    auto b_init = What::template rhs_init<T>();
    return time_mean<50'000'000>([&]() {
             auto a = a_init;
             auto b = b_init;
             fake_modify(a);
             if constexpr (What::rhs_const)
               {
                 constexpr auto tmp = What::template rhs_init<T>();
                 b = tmp;
               }
             else
               fake_modify(b);
             T r = What::left ? a << b : a >> b;
             if constexpr (Latency)
               {
                 a_init = r;
                 if constexpr (!What::rhs_const)
                   {
                     fake_modify(b, b_init);
                     b_init = b;
                   }
               }
             else
               fake_read(r);
           });
  }

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
