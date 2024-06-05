/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright © 2019-2024 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
 *                       Matthias Kretz <m.kretz@gsi.de>
 */

#ifndef BENCH_H_
#define BENCH_H_

#if __cpp_lib_simd >= 202402L
#define USE_STD_SIMD 1
#else
#define USE_STD_SIMD 0
#include <experimental/simd>
#endif

#include <array>
#include <bit>
#include <concepts>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <stdfloat>

#if USE_STD_SIMD == 0
namespace stdx
{
  using namespace std::experimental;
  using namespace std::experimental::__proposed;
}
#endif

#if USE_STD_SIMD
template <typename T, int W = 0>
  using simd = std::conditional_t<W == 0,
                                  std::simd<T>,
                                  std::simd<T, W == 0 ? 1 : W>>;
#else
template <typename T, int W>
  struct simd_deduce_impl;

template <typename T, int W>
  requires requires { typename stdx::simd_abi::deduce_t<T, W>; }
  struct simd_deduce_impl<T, W>
  { using type = stdx::simd<T, stdx::simd_abi::deduce_t<T, W>>; };

// invalid ABI
template <typename T, int W>
  struct simd_deduce_impl
  { using type = stdx::simd<T, int>; };

template <typename T>
  struct simd_deduce_impl<T, 0>
  { using type = stdx::native_simd<T>; };

template <typename T, int W = 0>
  using simd = typename simd_deduce_impl<T, W>::type;
#endif

template <class T>
  auto
  value_type_t_impl()
  {
    if constexpr (requires { typename T::value_type; })
      return typename T::value_type();
    else if constexpr (requires(T x) { x[0]; })
      return T()[0];
    else
      return T();
  }

template <class T>
  using value_type_t = decltype(value_type_t_impl<T>());

/**
 * Alias for a vector builtin with given value type and total sizeof.
 */
template <typename T, size_t Bytes>
  requires (std::has_single_bit(Bytes))
  using vec_builtin_type_bytes [[gnu::vector_size(Bytes)]] = T;

/**
 * Constrain to any vector builtin with given value type and optional width.
 */
template <typename T, typename ValueType, int W = sizeof(T) / sizeof(ValueType)>
  concept vec_builtin_of
    = not std::is_scalar_v<T> and W >= 1 and sizeof(T) == W * sizeof(ValueType)
        and std::same_as<vec_builtin_type_bytes<ValueType, sizeof(T)>, T>
        and requires(T& v, ValueType x) { v[0] = x; };

/**
 * Constrain to any vector builtin.
 */
template <typename T>
  concept vec_builtin = not std::is_class_v<T> and vec_builtin_of<T, value_type_t<T>>;

/**
 * The width (number of value_type elements) of the given vector builtin.
 */
template <vec_builtin T>
  inline constexpr int width_of = sizeof(T) / sizeof(value_type_t<T>);

template <class T>
  inline constexpr int size_v = [] {
    if constexpr (requires { { T::size() } -> std::integral; })
      return T::size();
    else if constexpr(vec_builtin<T>)
      return width_of<T>;
    else
      return 1;
  }();

template <typename T>
  inline constexpr int speedup_size_v = [] {
    if constexpr (size_v<T> == 1)
      return 1;
    else
      {
        using Native = simd<value_type_t<T>>;
        if constexpr (sizeof(T) > sizeof(Native))
          return Native::size();
        else
          return size_v<T>;
      }
  }();

template <typename T>
  struct is_character_type
  : std::bool_constant<false>
  {};

template <typename T>
  inline constexpr bool is_character_type_v = is_character_type<T>::value;

template <typename T>
  struct is_character_type<const T>
  : is_character_type<T>
  {};

template <typename T>
  struct is_character_type<T&>
  : is_character_type<T>
  {};

template <> struct is_character_type<char> : std::bool_constant<true> {};
template <> struct is_character_type<wchar_t> : std::bool_constant<true> {};
template <> struct is_character_type<char8_t> : std::bool_constant<true> {};
template <> struct is_character_type<char16_t> : std::bool_constant<true> {};
template <> struct is_character_type<char32_t> : std::bool_constant<true> {};

#if USE_STD_SIMD
template <typename T, typename Abi>
std::ostream& operator<<(std::ostream& s, std::basic_simd<T, Abi> const& v)
{
  using U = std::conditional_t<sizeof(T) == 1, int,
                               std::conditional_t<is_character_type_v<T>,
                                                  std::__detail::__make_unsigned_int_t<T>, T>>;
  s << '[' << U(v[0]);
  for (int i = 1; i < v.size(); ++i)
    s << ", " << U(v[i]);
  return s << ']';
}

template <std::size_t B, typename Abi>
std::ostream& operator<<(std::ostream& s, std::basic_simd_mask<B, Abi> const& v)
{
  s << '<';
  for (int i = 0; i < v.size(); ++i)
    s << int(v[i]);
  return s << '>';
}
#else
template <typename T, typename Abi>
std::ostream& operator<<(std::ostream& s, stdx::simd<T, Abi> const& v)
{
  using U = std::conditional_t<sizeof(T) == 1, int,
                               std::conditional_t<is_character_type_v<T>,
                                                  std::make_unsigned_t<
                                                    stdx::__int_for_sizeof_t<T>>, T>>;
  s << '[' << U(v[0]);
  for (unsigned i = 1; i < v.size(); ++i)
    s << ", " << U(v[i]);
  return s << ']';
}

template <typename T, typename Abi>
std::ostream& operator<<(std::ostream& s, stdx::simd_mask<T, Abi> const& v)
{
  s << '<';
  for (unsigned i = 0; i < v.size(); ++i)
    s << int(v[i]);
  return s << '>';
}
#endif

template <vec_builtin V>
  std::ostream& operator<<(std::ostream& s, V v)
  { return s << simd<value_type_t<V>, width_of<V>>(v); }

template <int N>
  using Info = std::array<const char*, N>;

template <int N>
  struct Times
  {
    std::array<double, N> cycles_per_call;
    int size;

    using DoubleN = double[N];

    constexpr
    Times(std::floating_point auto... init)
    : cycles_per_call{init...}, size(-1)
    { static_assert(sizeof...(init) == N); }

    constexpr
    Times(const std::array<double, N>& init, int init_size)
    : cycles_per_call(init), size(init_size)
    {}

    constexpr double
    operator[](int i) const
    { return cycles_per_call[i]; }
  };

template <int Special, class...>
  struct Benchmark
  { static_assert("The benchmark must specialize this type."); };

struct NoRef
{ static constexpr int size = -1; };

template <typename T, typename B>
  concept accept_type_for_benchmark
    = std::default_initializable<T>
        and (not requires { { auto(not B::template accept<T>) } -> std::convertible_to<bool>; }
               or B::template accept<T>);

template <class T, class B, class Ref = NoRef>
  requires (not accept_type_for_benchmark<T, B>)
  Ref
  bench_lat_thr(const char*, const Ref& ref = {})
  { return ref; }

template <class T, class B, class Ref = NoRef>
  requires accept_type_for_benchmark<T, B>
  Times<B::info.size()>
  bench_lat_thr(const char* id, const Ref& ref = {})
  {
    constexpr int N = B::info.size();
    static constexpr char red[] = "\033[1;40;31m";
    static constexpr char green[] = "\033[1;40;32m";
    static constexpr char dgreen[] = "\033[0;40;32m";
    static constexpr char normal[] = "\033[0m";

    const Times<N> results = { B::template run<T>().cycles_per_call, size_v<T> };
    std::cout << id;
    for (int i = 0; i < N; ++i)
      {
        double speedup = 1;
        if constexpr (!std::is_same_v<Ref, NoRef>)
          speedup = ref[i] * size_v<T> / (results[i] * ref.size);

        std::cout << std::setprecision(3) << std::setw(15) << results[i];
        if (speedup_size_v<T> <= ref.size)
          {
            if (speedup >= 1.1)
              std::cout << green;
            else if (speedup > 0.995)
              std::cout << dgreen;
            else
              std::cout << red;
          }
        else if (speedup >= speedup_size_v<T> * 0.90 / ref.size && speedup >= 1.5)
          std::cout << green;
        else if (speedup > 1.1)
          std::cout << dgreen;
        else if (speedup < 0.95)
          std::cout << red;
        std::cout << std::setw(12) << speedup << normal;
      }
    std::cout << std::endl;
    if constexpr (std::same_as<Ref, NoRef>)
      return results;
    else
      return ref;
  }

template <std::size_t N>
  using cstr = char[N];

template <class B, std::size_t N>
  void
  print_header(const cstr<N> &id_name)
  {
    std::cout << id_name;
    for (unsigned i = 0; i < B::info.size(); ++i)
      std::cout << ' ' << std::setw(14) << B::info[i] << std::setw(12) << "Speedup";
    std::cout << '\n';

    char pad[N] = {};
    std::memset(pad, ' ', N - 1);
    pad[N - 1] = '\0';
    std::cout << pad;
    for (unsigned i = 0; i < B::info.size(); ++i)
      std::cout << std::setw(15) << "[cycles/call]" << std::setw(12) << "[per value]";
    std::cout << '\n';
  }

template <class T, class... ExtraFlags>
  void
  bench_all()
  {
    static_assert((std::same_as<decltype(ExtraFlags::name[0]), const char&> and ...));
    using B = Benchmark<0, ExtraFlags...>;
    constexpr std::size_t value_type_field = 6;
    constexpr std::size_t abi_field = 12;
    constexpr std::size_t type_field = value_type_field + 2 + abi_field;
    constexpr std::size_t id_size = type_field + (1 + ... + (1 + sizeof(ExtraFlags::name)));
    char id[id_size];
    std::memset(id, ' ', id_size - 1);
    id[id_size - 1] = '\0';
    std::memcpy(id + id_size/2 - 2, "TYPE", 4);
    print_header<B>(id);
    std::memcpy(id + id_size/2 - 2, "    ", 4);
    char* const typestr = id;
    char* const abistr  = id + value_type_field + 2;
    id[value_type_field] = ',';

    if constexpr (std::is_same_v<T, float>)
      std::memcpy(typestr, " float", value_type_field);
    else if constexpr (std::is_same_v<T, double>)
      std::memcpy(typestr, "double", value_type_field);
    else if constexpr (std::is_same_v<T, long double>)
      std::memcpy(typestr, "ldoubl", value_type_field);
#ifdef __STDCPP_FLOAT16_T__
    else if constexpr (std::is_same_v<T, std::float16_t>)
      std::memcpy(typestr, " flt16", value_type_field);
#endif
#ifdef __STDCPP_FLOAT32_T__
    else if constexpr (std::is_same_v<T, std::float32_t>)
      std::memcpy(typestr, " flt32", value_type_field);
#endif
#ifdef __STDCPP_FLOAT64_T__
    else if constexpr (std::is_same_v<T, std::float64_t>)
      std::memcpy(typestr, " flt64", value_type_field);
#endif
    else if constexpr (std::is_same_v<T, long long>)
      std::memcpy(typestr, " llong", value_type_field);
    else if constexpr (std::is_same_v<T, unsigned long long>)
      std::memcpy(typestr, "ullong", value_type_field);
    else if constexpr (std::is_same_v<T, long>)
      std::memcpy(typestr, "  long", value_type_field);
    else if constexpr (std::is_same_v<T, unsigned long>)
      std::memcpy(typestr, " ulong", value_type_field);
    else if constexpr (std::is_same_v<T, int>)
      std::memcpy(typestr, "   int", value_type_field);
    else if constexpr (std::is_same_v<T, unsigned>)
      std::memcpy(typestr, "  uint", value_type_field);
    else if constexpr (std::is_same_v<T, short>)
      std::memcpy(typestr, " short", value_type_field);
    else if constexpr (std::is_same_v<T, unsigned short>)
      std::memcpy(typestr, "ushort", value_type_field);
    else if constexpr (std::is_same_v<T, char>)
      std::memcpy(typestr, "  char", value_type_field);
    else if constexpr (std::is_same_v<T, signed char>)
      std::memcpy(typestr, " schar", value_type_field);
    else if constexpr (std::is_same_v<T, unsigned char>)
      std::memcpy(typestr, " uchar", value_type_field);
#if __cpp_char8_t >= 201811L
    else if constexpr (std::is_same_v<T, char8_t>)
      std::memcpy(typestr, " char8", value_type_field);
#endif
    else if constexpr (std::is_same_v<T, char16_t>)
      std::memcpy(typestr, "char16", value_type_field);
    else if constexpr (std::is_same_v<T, char32_t>)
      std::memcpy(typestr, "char32", value_type_field);
    else if constexpr (std::is_same_v<T, wchar_t>)
      std::memcpy(typestr, " wchar", value_type_field);
    else
      std::memcpy(typestr, "??????", value_type_field);

    if constexpr (sizeof...(ExtraFlags) > 0)
      {
        char* extraflags = id + type_field + 1;
        ([&] {
          std::memcpy(extraflags, ExtraFlags::name, sizeof(ExtraFlags::name) - 1);
          extraflags += sizeof(ExtraFlags::name) + 1;
        }(), ...);
      }

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

    using V16 [[gnu::vector_size(16)]] = T;
    auto bench_gnu_vectors = [&](const auto ref0) {
      const auto ref16 = [&] {
        if constexpr (alignof(V16) == sizeof(V16))
          {
            set_abistr(("[[" + std::to_string(16 / sizeof(T)) + "]]").c_str());
            return bench_lat_thr<V16, B>(id, ref0);
          }
        else
          return ref0;
      }();
      const auto ref32 = [&] {
#ifdef __AVX__
        using V32 [[gnu::vector_size(32)]] = T;
        if constexpr (alignof(V32) == sizeof(V32))
          {
            set_abistr(("[[" + std::to_string(32 / sizeof(T)) + "]]").c_str());
            return bench_lat_thr<V32, B>(id, ref16);
          }
        else
#endif
          return ref16;
      }();
      return [&] {
#ifdef __AVX512F__
        using V64 [[gnu::vector_size(64)]] = T;
        if constexpr (alignof(V64) == sizeof(V64))
          {
            set_abistr(("[[" + std::to_string(64 / sizeof(T)) + "]]").c_str());
            return bench_lat_thr<V64, B>(id, ref32);
          }
        else
#endif
          return ref32;
      }();
    };

    set_abistr("");
    const auto ref0 = bench_lat_thr<T, B>(id);
    set_abistr("1");
    const auto ref1 = bench_lat_thr<simd<T, 1>, B>(id, ref0);

    constexpr bool use_gnu_reference
      = std::is_same_v<decltype(ref1), const NoRef> and alignof(V16) == sizeof(V16);
    const auto ref = [&] {
      if constexpr (use_gnu_reference)
        return bench_gnu_vectors(ref1);
      else
        return ref1;
    }();

    [&]<int... Is>(std::integer_sequence<int, Is...>) {
      ([&] {
        constexpr int N = 2 << Is;
        if constexpr (std::constructible_from<simd<T, N>>)
          {
            set_abistr(std::to_string(N).c_str());
            bench_lat_thr<simd<T, N>, B>(id, ref);
          }
      }(), ...);
    }(std::make_integer_sequence<int, 8>());

    if constexpr (requires { { B::more_types[0] } -> std::same_as<const char* const&>; })
      {
        constexpr int N = B::more_types.size();
        [&]<int... Is>(std::integer_sequence<int, Is...>) {
          ([&] {
            set_abistr(B::more_types[Is]);
            bench_lat_thr<T, Benchmark<Is + 1, ExtraFlags...>>(id, ref);
          }(), ...);
        }(std::make_integer_sequence<int, N>());
      }

    if constexpr (not use_gnu_reference)
      bench_gnu_vectors(ref);

    char sep[id_size + 2 * 15 + 2 * 12];
    std::memset(sep, '-', sizeof(sep) - 1);
    sep[sizeof(sep) - 1] = '\0';
    std::cout << sep << std::endl;
  }

template <long Iterations = 50'000, int Retries = 20, class F>
  [[gnu::noinline]]
  double
  time_mean2(F&& fun)
  {
    struct {
      //double mean = 0;
      double minimum = std::numeric_limits<double>::max();
      unsigned long tsc_start = 0;
      int todo = Retries;
      long it = 1;

      [[gnu::always_inline]]
      operator bool() &
      {
        if (--it > 0) [[likely]]
          return true;

        unsigned int tmp = 0;
        const auto tsc_end = __rdtscp(&tmp);
        if (todo < Retries) [[likely]]
          {
            const double elapsed = tsc_end - tsc_start;
            const double one_mean = elapsed / Iterations;
            //mean += one_mean / Retries;
            if (one_mean < minimum)
              {
                minimum = one_mean;
                todo = Retries;
              }
          }
        if (todo) [[likely]]
          {
            --todo;
            it = Iterations + 1;
            tsc_start = __rdtscp(&tmp);
            return true;
          }
        return false;
      }
    } collector;
    fun(collector);
    return collector.minimum;
  }

template <long Iterations = 50'000, int Retries = 20, class F, class... Args>
  double
  time_mean(F&& fun, Args&&... args)
  {
    double minimum = std::numeric_limits<double>::max();
    for (int tries = 0; tries < Retries; ++tries)
      {
        unsigned int tmp;
        long i = Iterations;
        const auto start = __rdtscp(&tmp);
        for (; i; --i)
          fun(std::forward<Args>(args)...);
        const auto end = __rdtscp(&tmp);
        const double elapsed = end - start;
        if (elapsed < minimum)
          {
            minimum = elapsed;
            tries = -1;
          }
      }
    return minimum / Iterations;
  }

template <typename T>
  [[gnu::always_inline]] inline void
  fake_modify_one(T& x)
  {
    if constexpr (std::is_floating_point_v<T>)
      asm volatile("" : "+v,x"(x));
    else if constexpr (requires { {auto(__data(x))} -> vec_builtin; })
      fake_modify_one(__data(x));
    else if constexpr (requires { {auto(x._M_data)} -> vec_builtin; })
      fake_modify_one(x._M_data);
    else if constexpr (requires { {auto(__data(x)[0])} -> vec_builtin; })
      {
        for (auto& d : __data(x))
          fake_modify_one(d);
      }
#if not USE_STD_SIMD
    else if constexpr (stdx::is_simd_v<T> || stdx::is_simd_mask_v<T>)
      {
        auto& d = stdx::__data(x);
        if constexpr (requires {{d._S_tuple_size};})
          stdx::__for_each(d, [](auto, auto& element) {
            fake_modify_one(element);
          });
        else if constexpr (requires {{d._M_data};})
          fake_modify_one(d._M_data);
        else
          fake_modify_one(d);
      }
#endif
    else if constexpr (std::is_array_v<T>
                         or requires(void (&test)(typename T::value_type&)) {
                           {test(*std::begin(x))};
                         })
      {
        for (auto& y : x)
          fake_modify_one(y);
      }
    else if constexpr (sizeof(x) >= 16)
      asm volatile("" : "+v,x"(x));
    else
      asm volatile("" : "+v,x,g"(x));
  }

template <typename... Ts>
  [[gnu::always_inline]] inline void
  fake_modify(Ts&... more)
  { (fake_modify_one(more), ...); }

template <typename T>
  [[gnu::always_inline]] inline void
  fake_read_one(const T& x)
  {
    if constexpr (std::is_floating_point_v<T>)
      asm volatile("" ::"v,x"(x));
    else if constexpr (requires { {auto(__data(x))} -> vec_builtin; })
      fake_read_one(__data(x));
    else if constexpr (requires { {auto(x._M_data)} -> vec_builtin; })
      fake_read_one(x._M_data);
    else if constexpr (requires { {auto(__data(x)[0])} -> vec_builtin; })
      {
        for (const auto& d : __data(x))
          fake_read_one(d);
      }
#if not USE_STD_SIMD
    else if constexpr (stdx::is_simd_v<T> || stdx::is_simd_mask_v<T>)
      {
        const auto& d = stdx::__data(x);
        if constexpr (requires {{d._S_tuple_size};})
          stdx::__for_each(d, [](auto, const auto& element) {
            fake_read_one(element);
          });
        else if constexpr (requires {{d._M_data};})
          fake_read_one(d._M_data);
        else
          fake_read_one(d);
      }
#endif
    else if constexpr (std::is_array_v<T>
                         or requires(void (&test)(const typename T::value_type&)) {
                           {test(*std::begin(x))};
                         })
      {
        for (const auto& y : x)
          fake_read_one(y);
      }
    else if constexpr (sizeof(x) >= 16)
      {
        static_assert(vec_builtin<T>);
        asm volatile("" ::"v,x"(x));
      }
    else
      asm volatile("" ::"v,x,g"(x));
  }

template <typename... Ts>
  [[gnu::always_inline]] inline void
  fake_read(const Ts&... more)
  { (fake_read_one(more), ...); }

template <typename T, std::size_t N>
  using carray = T[N];

template <long Iterations = 50'000, int Retries = 20, typename T, std::size_t N>
  double
  time_latency(carray<T, N>& data, auto&& process_one, auto&& fake_one)
  {
    for (auto& x : data)
      fake_modify_one(x);

    const double dt = time_mean<Iterations, Retries>([&] [[gnu::always_inline]] {
                        process_one(data[0]);
                      }) - time_mean<Iterations, Retries>([&] [[gnu::always_inline]] {
                             fake_one(data[0]);
                           });

    if (dt >= 0.98)
      return dt;

    std::cerr << "impossible latency value: " << dt << '\n';
    std::abort();
  }

template <long Iterations = 50'000, int Retries = 20, typename T, std::size_t N>
  double
  time_throughput(carray<T, N>& data, auto&& process_one, auto&& fake_one)
  {
    for (auto& x : data)
      fake_modify_one(x);

    const double dt
      = (time_mean2<Iterations, Retries>([&](auto& need_more) {
           [&]<std::size_t... Is>(std::index_sequence<Is...>) {
             while (need_more)
               (process_one(data[Is]), ...);
           }(std::make_index_sequence<N>());
         }) - time_mean2<Iterations, Retries>([&](auto& need_more) {
                [&]<std::size_t... Is>(std::index_sequence<Is...>) {
                  while (need_more)
                    (fake_one(data[Is]), ...);
                }(std::make_index_sequence<N>());
              }))
          / N;

    return dt;
  }

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
template <vec_builtin T, class... More>                                        \
  T name(T a, More... more)                                                    \
  {                                                                            \
    T r {};                                                                    \
    for (int i = 0; i < width_of<T>; ++i)                                      \
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
