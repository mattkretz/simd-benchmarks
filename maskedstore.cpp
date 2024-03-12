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
    where(k, x).copy_to(ptr, stdx::element_aligned);
    asm(""::"m"(*mem));
  }

std::random_device rd;
std::mt19937 gen(rd());

template <typename T>
  concept fixed_size
    = requires (const T& v,
                void (&fun)(const stdx::fixed_size_simd<typename T::value_type, T::size()>&)) {
      fun(v);
    };

constexpr bool
is_fixed_size(...)
{ return false; }

template <int Special>
  struct Benchmark<Special>
  {
    static constexpr Info<4> info
      = {"Random Mask", "Epilogue Mask", "Transform", "read-modify-write"};

    template <typename T>
      static constexpr bool accept = std::constructible_from<T>
                                       and requires { typename T::abi_type; }
                                       and not fixed_size<T>;

    template <class V>
      static Times<4>
      run()
      {
        using T = value_type_t<V>;
        using U = std::make_unsigned_t<stdx::__int_for_sizeof_t<T>>;
        using UV = stdx::rebind_simd_t<U, V>;
        constexpr unsigned nbits = sizeof(U) * CHAR_BIT;
        static_assert(std::has_single_bit(nbits));

        constexpr int N = 1024 / sizeof(T);
        using Mem = alignas(64) T[N];

        constexpr int NRandom = 64 * 64;

        alignas(stdx::memory_alignment_v<V>) T masks[1024 + V::size()] = {};
        std::uniform_int_distribution<unsigned> dist_unsigned;
        for (auto& b : masks)
          b = (dist_unsigned(gen) & 0x100) == 0 ? T(1) : T(0);

        alignas(stdx::memory_alignment_v<V>) T input[1024 + V::size()] = {};
        alignas(stdx::memory_alignment_v<V>) T output[1024 + V::size()] = {};

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
                auto&& make_mask = [&]() __attribute__((always_inline)) {
                  size_t i = (need_more.it * V::size()) % 1024;
                  return V(&masks[i], stdx::element_aligned) == 0;
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
                auto&& make_mask = [&]() __attribute__((always_inline)) {
                  return stdx::static_simd_cast<typename V::mask_type>(
                           iota < U(need_more.it % V::size()));
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
                V obj(&input[i], stdx::vector_aligned);
                if constexpr (std::is_floating_point_v<T>)
                  where(obj > 0.5f, obj).copy_to(&output[i], stdx::vector_aligned);
                else if constexpr (std::is_signed_v<T>)
                  where(obj >= 0, obj).copy_to(&output[i], stdx::vector_aligned);
                else
                  where(obj > std::numeric_limits<T>::max() / 2, obj)
                    .copy_to(&output[i], stdx::vector_aligned);
              }
          }),
          time_mean2<1'000'000>([&](auto& need_more)
          {
            while (need_more)
              {
                asm ("":"+m"(*output), "+m"(*input));
                size_t i = (need_more.it * V::size()) % 1024;
                V obj(&input[i], stdx::vector_aligned);
                V out(&output[i], stdx::vector_aligned);
                if constexpr (std::is_floating_point_v<T>)
                  where(obj > 0.5f, out) = obj;
                else if constexpr (std::is_signed_v<T>)
                  where(obj >= 0, out) = obj;
                else
                  where(obj > std::numeric_limits<T>::max() / 2, out) = obj;
                out.copy_to(&output[i], stdx::vector_aligned);
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
