/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright © 2019-2023 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
 *                       Matthias Kretz <m.kretz@gsi.de>
 */

#ifndef BENCH_H_
#define BENCH_H_

#include <experimental/simd>
#include <array>
#include <iostream>
#include <string>

namespace stdx = std::experimental;

template <class T>
  auto
  value_type_t_impl()
  {
    if constexpr(std::is_arithmetic_v<T>)
      return T();
    else if constexpr (stdx::is_simd_v<T>)
      return typename T::value_type();
    else
      return T()[0];
  }

template <class T>
  using value_type_t = decltype(value_type_t_impl<T>());

template <class T>
  struct size
  {
    static inline constexpr int
    value_impl()
    {
      if constexpr(std::experimental::__is_vector_type_v<T>)
        return std::experimental::_VectorTraits<T>::_S_full_size;
      else
        return 1;
    }

    static inline constexpr int value = value_impl();
  };

template <class T, class Abi>
  struct size<stdx::simd<T, Abi>>
  { static inline constexpr int value = stdx::simd_size_v<T, Abi>; };

template <class T>
  constexpr inline int size_v = size<T>::value;

template <class T>
  struct speedup_size : size<T> {};

template <class T, int N>
  struct speedup_size<stdx::simd<T, stdx::simd_abi::fixed_size<N>>>
  { static inline constexpr int value = stdx::__fixed_size_storage_t<T, N>::_S_first_size; };

template <class T>
  constexpr inline int speedup_size_v = speedup_size<T>::value;

template <int N>
  using Info = std::array<const char*, N>;

template <int N>
  using Times = std::array<double, N>;

struct DefaultBenchmark
{ static constexpr Info<2> info = {"Latency", "Throughput"}; };

template <class...>
  struct Benchmark
  { static_assert("The benchmark must specialize this type."); };

struct NoRef {};

template <class T, class... ExtraFlags, class = decltype(T()), class Ref = NoRef,
          class B = Benchmark<ExtraFlags...>, int N = B::info.size()>
  Times<N>
  bench_lat_thr(const char* id, const Ref& ref = {})
  {
    static constexpr char red[] = "\033[1;40;31m";
    static constexpr char green[] = "\033[1;40;32m";
    static constexpr char dgreen[] = "\033[0;40;32m";
    static constexpr char normal[] = "\033[0m";

    const Times<N> results = B::template run<T>();
    std::cout << id;
    for (int i = 0; i < N; ++i)
      {
        double speedup = 1;
        if constexpr (!std::is_same_v<Ref, NoRef>)
          speedup = ref[i] * size_v<T> / results[i];

        std::cout << std::setprecision(3) << std::setw(15) << results[i];
        if (speedup >= speedup_size_v<T> * 0.90 && speedup >= 1.5)
          std::cout << green;
        else if (speedup > 1.1)
          std::cout << dgreen;
        else if (speedup < 0.95)
          std::cout << red;
        std::cout << std::setw(12) << speedup << normal;
      }
    std::cout << std::endl;
    return results;
  }

template <class, class...>
  void
  bench_lat_thr(...) {}

template <std::size_t N>
  using cstr = char[N];

template <class B, std::size_t N>
  void
  print_header(const cstr<N> &id_name)
  {
    std::cout << id_name;
    for (int i = 0; i < B::info.size(); ++i)
      std::cout << std::setw(15) << B::info[i] << std::setw(12) << "Speedup";
    std::cout << '\n';

    char pad[N] = {};
    std::memset(pad, ' ', N - 1);
    pad[N - 1] = '\0';
    std::cout << pad;
    for (int i = 0; i < B::info.size(); ++i)
      std::cout << std::setw(15) << "[cycles/call]" << std::setw(12) << "[per value]";
    std::cout << '\n';
  }

template <class T, class... ExtraFlags>
  void
  bench_all()
  {
    using namespace std::experimental::simd_abi;
    using std::experimental::simd;
    constexpr std::size_t value_type_field = 6;
    constexpr std::size_t abi_field = 24;
    constexpr std::size_t type_field = value_type_field + 2 + abi_field;
    constexpr std::size_t id_size = type_field + (1 + ... + (1 + sizeof(ExtraFlags::name)));
    char id[id_size];
    std::memset(id, ' ', id_size - 1);
    id[id_size - 1] = '\0';
    std::strncpy(id + id_size/2 - 2, "TYPE", 4);
    print_header<Benchmark<ExtraFlags...>>(id);
    std::strncpy(id + id_size/2 - 2, "    ", 4);
    char* const typestr = id;
    char* const abistr  = id + value_type_field + 2;
    id[value_type_field] = ',';
    char* extraflags = id + type_field + 1;

    if constexpr (std::is_same_v<T, float>)
      std::strncpy(typestr, " float", value_type_field);
    else if constexpr (std::is_same_v<T, double>)
      std::strncpy(typestr, "double", value_type_field);
    else if constexpr (std::is_same_v<T, long double>)
      std::strncpy(typestr, "ldoubl", value_type_field);
    else if constexpr (std::is_same_v<T, long long>)
      std::strncpy(typestr, " llong", value_type_field);
    else if constexpr (std::is_same_v<T, unsigned long long>)
      std::strncpy(typestr, "ullong", value_type_field);
    else if constexpr (std::is_same_v<T, long>)
      std::strncpy(typestr, "  long", value_type_field);
    else if constexpr (std::is_same_v<T, unsigned long>)
      std::strncpy(typestr, " ulong", value_type_field);
    else if constexpr (std::is_same_v<T, int>)
      std::strncpy(typestr, "   int", value_type_field);
    else if constexpr (std::is_same_v<T, unsigned>)
      std::strncpy(typestr, "  uint", value_type_field);
    else if constexpr (std::is_same_v<T, short>)
      std::strncpy(typestr, " short", value_type_field);
    else if constexpr (std::is_same_v<T, unsigned short>)
      std::strncpy(typestr, "ushort", value_type_field);
    else if constexpr (std::is_same_v<T, char>)
      std::strncpy(typestr, "  char", value_type_field);
    else if constexpr (std::is_same_v<T, signed char>)
      std::strncpy(typestr, " schar", value_type_field);
    else if constexpr (std::is_same_v<T, unsigned char>)
      std::strncpy(typestr, " uchar", value_type_field);
    else
      std::strncpy(typestr, "??????", value_type_field);

    ([&] {
      std::strncpy(extraflags, ExtraFlags::name, sizeof(ExtraFlags::name) - 1);
      extraflags += sizeof(ExtraFlags::name) + 1;
    }(), ...);

    auto set_abistr = [&](const char* str) {
      std::size_t len = std::strlen(str);
      if (len > abi_field)
        {
          str += len - abi_field;
          len = abi_field;
        }
      const std::size_t ncpy = std::min(abi_field, len);
      std::memcpy(abistr, str, ncpy);
      if (len < abi_field)
        std::memset(abistr + ncpy, ' ', abi_field - ncpy);
    };

    const auto ref = bench_lat_thr<T, ExtraFlags...>(id);
    set_abistr("simd_abi::scalar");
    bench_lat_thr<simd<T, scalar>, ExtraFlags...>(id, ref);
    set_abistr("simd_abi::__sse");
    bench_lat_thr<simd<T, __sse>, ExtraFlags...>(id, ref);
    set_abistr("simd_abi::__avx");
    bench_lat_thr<simd<T, __avx>, ExtraFlags...>(id, ref);
    set_abistr("simd_abi::_Avx512<32>");
    bench_lat_thr<simd<T, _Avx512<32>>, ExtraFlags...>(id, ref);
    set_abistr("simd_abi::_Avx512<64>");
    bench_lat_thr<simd<T, _Avx512<64>>, ExtraFlags...>(id, ref);
    set_abistr("simd_abi::fixed_size<4>");
    bench_lat_thr<simd<T, fixed_size<4>>, ExtraFlags...>(id, ref);
    set_abistr("simd_abi::fixed_size<12>");
    bench_lat_thr<simd<T, fixed_size<12>>, ExtraFlags...>(id, ref);
    set_abistr("simd_abi::fixed_size<16>");
    bench_lat_thr<simd<T, fixed_size<16>>, ExtraFlags...>(id, ref);

    using V16 [[gnu::vector_size(16)]] = T;
    if constexpr (alignof(V16) == sizeof(V16))
      {
        set_abistr("[[gnu::vector_size(16)]]");
        bench_lat_thr<V16, ExtraFlags...>(id, ref);
      }
    using V32 [[gnu::vector_size(32)]] = T;
    if constexpr (alignof(V32) == sizeof(V32))
      {
        set_abistr("[[gnu::vector_size(32)]]");
        bench_lat_thr<V32, ExtraFlags...>(id, ref);
      }
    using V64 [[gnu::vector_size(64)]] = T;
    if constexpr (alignof(V64) == sizeof(V64))
      {
        set_abistr("[[gnu::vector_size(64)]]");
        bench_lat_thr<V64, ExtraFlags...>(id, ref);
      }

    char sep[id_size + 2 * 15 + 2 * 12];
    std::memset(sep, '-', sizeof(sep) - 1);
    sep[sizeof(sep) - 1] = '\0';
    std::cout << sep << std::endl;
  }

template <long Iterations, class F, class... Args>
  double
  time_mean(F&& fun, Args&&... args)
  {
    unsigned int tmp;
    long i = Iterations;
    const auto start = __rdtscp(&tmp);
    for (; i; --i)
      fun(std::forward<Args>(args)...);
    const auto end = __rdtscp(&tmp);
    const double elapsed = end - start;
    return elapsed / Iterations;
  }

template <typename T>
  [[gnu::always_inline]] inline void
  fake_modify1(T& x)
  {
    if constexpr (sizeof(T) >= 16 || std::is_floating_point_v<T>)
      asm volatile("" : "+xm"(x));
    else
      asm volatile("" : "+g"(x));
  }

template <typename... Ts>
  [[gnu::always_inline]] inline void
  fake_modify(Ts&... more)
  { (fake_modify1(more), ...); }

template <typename T>
  [[gnu::always_inline]] inline void
  fake_read1(const T& x)
  {
    if constexpr (sizeof(T) >= 16 || std::is_floating_point_v<T>)
      asm volatile("" ::"xm"(x));
    else
      asm volatile("" ::"g"(x));
  }

template <typename... Ts>
  [[gnu::always_inline]] inline void
  fake_read(const Ts&... more)
  { (fake_read1(more), ...); }

template <typename T>
  [[gnu::always_inline]] inline auto
  subscript_or_value(T x, int i)
  {
    if constexpr(std::is_arithmetic_v<T>)
      return x;
    else
      return x[i];
  }

#define MAKE_VECTORMATH_OVERLOAD(name)                                         \
template <class T, class... More,                                              \
	  class VT = std::experimental::_VectorTraits<T>>                      \
  T name(T a, More... more)                                                    \
  {                                                                            \
    T r;                                                                       \
    for (int i = 0; i < VT::_S_full_size; ++i)                                 \
      r[i] = std::name(a[i], subscript_or_value(more, i)...);                  \
    return r;                                                                  \
  }

#define FUN1(fun)                                                   \
MAKE_VECTORMATH_OVERLOAD(fun)                                       \
typedef struct F_##fun                                              \
{                                                                   \
  static constexpr char name[] = #fun "(x)";                        \
                                                                    \
  template <class T>                                                \
    [[gnu::always_inline]] static auto                              \
    apply(const T& x)                                               \
    {                                                               \
      using ::fun;                                                  \
      using std::fun;                                               \
      return fun(x);                                                \
    }                                                               \
}

#define FUN2(fun)                                                   \
MAKE_VECTORMATH_OVERLOAD(fun)                                       \
typedef struct F_##fun                                              \
{                                                                   \
  static constexpr char name[] = #fun "(x, y)";                     \
                                                                    \
  template <class T, class U>                                       \
    [[gnu::always_inline]] static auto                              \
    apply(const T& x, const U& y)                                   \
    {                                                               \
      using ::fun;                                                  \
      using std::fun;                                               \
      return fun(x, y);                                             \
    }                                                               \
}

#endif  // BENCH_H_
