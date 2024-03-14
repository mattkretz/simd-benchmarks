/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright © 2023-2024 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
 *                       Matthias Kretz <m.kretz@gsi.de>
 */

#include "bench.h"
#include <random>
#include <bit>
#include <climits>

alignas(64) char mem[64 * 64] = {};

template <typename T>
  static T value = {};

template <typename T>
  inline void
  store(T& x, typename T::mask_type k, size_t offset = 0)
  {
    using Mem = value_type_t<T>;
    Mem* ptr = reinterpret_cast<Mem*>(mem + offset);
    asm("":"+m,g,v,x"(x), "+g,v,x,m"(k));
#if USE_STD_SIMD
    x.copy_to(ptr, k);
#else
    where(k, x).copy_to(ptr, stdx::element_aligned);
#endif
    asm(""::"m"(*mem));
  }

std::random_device rd;
std::mt19937 gen(rd());

template <typename T>
  concept fixed_size
#if USE_STD_SIMD
    = not (requires (const T x) { {auto(__data(x))} -> vec_builtin; }
             or requires (const T x) {
               {auto(__data(x))} -> std::same_as<typename T::value_type>;
             });
#else
    = requires (const T& v,
                void (&fun)(const stdx::fixed_size_simd<typename T::value_type, T::size()>&)) {
      fun(v);
    };
#endif

constexpr auto aligned
#if USE_STD_SIMD
  = std::simd_flag_aligned;
#else
  = stdx::vector_aligned;
#endif

template <int Special>
  struct Benchmark<Special>
  {
    static constexpr Info<4> info
      = {"Random Mask", "Epilogue Mask", "Transform", "read-modify-write"};

    template <typename T>
      static constexpr bool accept = std::default_initializable<T>
                                       and requires { typename T::abi_type; }
                                       and not fixed_size<T>;

    template <class V>
      static Times<4>
      run()
      {
        using T = value_type_t<V>;
#if USE_STD_SIMD
        using U = std::__detail::__make_unsigned_int_t<T>;
        using UV = std::rebind_simd_t<U, V>;
#else
        using U = std::make_unsigned_t<stdx::__int_for_sizeof_t<T>>;
        using UV = stdx::rebind_simd_t<U, V>;
#endif
        constexpr unsigned nbits = sizeof(U) * CHAR_BIT;
        static_assert(std::has_single_bit(nbits));

        constexpr int N = 1024 / sizeof(T);
        using Mem = alignas(64) T[N];

        constexpr int NRandom = 64 * 64;

        constexpr auto mem_alignment
#if USE_STD_SIMD
          = std::simd_alignment_v<V>;
#else
          = stdx::memory_alignment_v<V>;
#endif


        alignas(mem_alignment) T masks[1024 + V::size()] = {};
        std::uniform_int_distribution<unsigned> dist_unsigned;
        for (auto& b : masks)
          b = (dist_unsigned(gen) & 0x100) == 0 ? T(1) : T(0);

        alignas(mem_alignment) T input[1024 + V::size()] = {};
        alignas(mem_alignment) T output[1024 + V::size()] = {};

        std::conditional_t<
          std::is_floating_point_v<T>, std::uniform_real_distribution<T>,
          std::uniform_int_distribution<std::conditional_t<sizeof(T) == 1, short, T>>> dist;

        for (auto& x : input)
          x = dist(gen);

        return {
          time_mean2<800'000>([&](auto& need_more)
          {
            Mem mem = {};
            V obj = {};
            while (need_more)
              {
                asm ("":"+m"(*masks));
                auto&& make_mask = [&]() [[gnu::always_inline]] -> typename V::mask_type {
                  size_t i = (need_more.it * V::size()) % 1024;
                  return V(&masks[i]
#if not USE_STD_SIMD
                             , stdx::element_aligned
#endif
                          ) == T();
                };
                store<V>(obj, make_mask(), 0 * sizeof(V));
              }
          }),
          time_mean2<800'000>([&](auto& need_more)
          {
            Mem mem = {};
            V obj = {};
            UV iota([](U i) { return i; });
            fake_modify(iota);
            while (need_more)
              {
                auto&& make_mask = [&]() [[gnu::always_inline]] -> typename V::mask_type {
                  return
#if not USE_STD_SIMD
                    stdx::static_simd_cast<typename V::mask_type>
#endif
                      (iota < U(need_more.it % V::size()));
                };
                store<V>(obj, make_mask(), 0 * sizeof(V));
              }
          }),
          time_mean2<1'000'000>([&](auto& need_more)
          {
            while (need_more)
              {
                asm ("":"+m"(*output), "+m"(*input));
                size_t i = (need_more.it * V::size()) % 1024;
                V obj(&input[i], aligned);
#if USE_STD_SIMD
                if constexpr (std::is_floating_point_v<T>)
                  obj.copy_to(&output[i], obj > T(0.5), aligned);
                else if constexpr (std::is_signed_v<T>)
                  obj.copy_to(&output[i], obj >= T(), aligned);
                else
                  obj.copy_to(&output[i], obj > T(std::numeric_limits<T>::max() / 2), aligned);
#else
                if constexpr (std::is_floating_point_v<T>)
                  where(obj > 0.5f, obj).copy_to(&output[i], aligned);
                else if constexpr (std::is_signed_v<T>)
                  where(obj >= 0, obj).copy_to(&output[i], aligned);
                else
                  where(obj > std::numeric_limits<T>::max() / 2, obj)
                    .copy_to(&output[i], aligned);
#endif
              }
          }),
          time_mean2<1'000'000>([&](auto& need_more)
          {
            while (need_more)
              {
                asm ("":"+m"(*output), "+m"(*input));
                size_t i = (need_more.it * V::size()) % 1024;
                V obj(&input[i], aligned);
                V out(&output[i], aligned);
#if USE_STD_SIMD
                if constexpr (std::is_floating_point_v<T>)
                  out = std::simd_select(obj > T(0.5), obj, out);
                else if constexpr (std::is_signed_v<T>)
                  out = std::simd_select(obj >= T(), obj, out);
                else
                  out = std::simd_select(obj > T(std::numeric_limits<T>::max() / 2), obj, out);
#else
                if constexpr (std::is_floating_point_v<T>)
                  where(obj > T(0.5), out) = obj;
                else if constexpr (std::is_signed_v<T>)
                  where(obj >= T(), out) = obj;
                else
                  where(obj > T(std::numeric_limits<T>::max() / 2), out) = obj;
#endif
                out.copy_to(&output[i], aligned);
              }
          })
        };
      }
  };

static_assert(accept_type_for_benchmark<simd<float, 1>, Benchmark<0>>);

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
