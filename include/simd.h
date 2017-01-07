//
// Created by gaddra on 1/6/17.
//

#ifndef VARGAS_SIMD_H
#define VARGAS_SIMD_H

#include "utils.h"
#include "doctest.h"

#include <type_traits>
#include <x86intrin.h>
#include <cstdint>
#include <memory>
#include <vector>

#if !defined(VA_SIMD_USE_SSE) && !defined(VA_SIMD_USE_AVX2) && !defined(VA_SIMD_USE_AVX512)
#error("No SIMD instruction set defined.")
#endif

#ifdef VA_SIMD_USE_AVX2
#define VA_MAX_INT8 32
#define VA_MAX_INT16 16
#endif

#ifdef VA_SIMD_USE_SSE
#if !defined(VA_MAX_INT8)
#define VA_MAX_INT8 16
#endif
#if !defined(VA_MAX_INT16)
#define VA_MAX_INT16 8
#endif
#endif

namespace vargas {

  /**
   * From libsimdpp:
   * https://github.com/p12tic/libsimdpp
   * @tparam T
   * @tparam A
   */
  template<class T, std::size_t A>
  class aligned_allocator {
      static_assert(!(A & (A - 1)), "A is not a power of two");

    public:
      using value_type = T;
      using pointer = T *;
      using const_pointer = const T *;
      using reference = T &;
      using const_reference = const T &;
      using size_type = std::size_t;
      using difference_type = std::ptrdiff_t;

      aligned_allocator() = default;
      aligned_allocator(const aligned_allocator &) = default;
      template<class U>
      aligned_allocator(const aligned_allocator<U, A> &) {}
      ~aligned_allocator() = default;
      aligned_allocator &operator=(const aligned_allocator &) = delete;

      template<class U>
      struct rebind {
          using other = aligned_allocator<U, A>;
      };

      T *address(T &x) const {
          return &x;
      }

      constexpr std::size_t max_size() const {
          return (static_cast<std::size_t>(0) - static_cast<std::size_t>(1)) / sizeof(T);
      }

      // stateless
      bool operator!=(const aligned_allocator &) const { return false; }
      bool operator==(const aligned_allocator &) const { return true; }

      void construct(T *p, const T &t) const {
          void *pv = static_cast<void *>(p);
          new(pv) T(t);
      }

      void destroy(T *p) const {
          p->~T();
      }

      T *allocate(std::size_t n) const {
          if (n == 0) return nullptr;
          if (n > max_size()) throw std::length_error("aligned_allocator<T,A>::allocate() - Integer overflow.");

          const std::size_t al = A < 2 * sizeof(void *) ? 2 * sizeof(void *) : A;

          char *pv = new char[n * sizeof(T) + al];
          std::uintptr_t upv = reinterpret_cast<std::uintptr_t>(pv);
          upv = (upv + al) & ~(al - 1);
          char **aligned_pv = reinterpret_cast<char **>(upv);
          *(aligned_pv - 1) = pv; // original pointer

          return reinterpret_cast<T *>(aligned_pv);
      }

      void deallocate(T *p, std::size_t n) const {
          (void) n;
          if (!p) return;
          char **pptr = reinterpret_cast<char **>(p);
          delete[](*(pptr - 1));
      }

      template<class U>
      T *allocate(std::size_t n, const U *hint) const {
          (void) hint;
          return allocate(n);
      }
  };


  template<typename T, size_t N>
  struct SIMD {
      static_assert(N == 8 || N == 16 || N == 32, "Invalid N in SIMD<T,N>");
      static_assert(std::is_same<T, int8_t>::value || std::is_same<T, int16_t>::value, "Invalid T in SIMD<T,N>");

      using native_t = T;
      using simd_t = typename std::conditional<std::is_same<int8_t, T>::value,
                                               typename std::conditional<N == 16, __m128i,
                                                                         typename std::conditional<N == 32,
                                                                                                   __m256i,
                                                                                                   __m512i>::type>::type,
                                               typename std::conditional<N == 8, __m128i,
                                                                         typename std::conditional<N == 16,
                                                                                                   __m256i,
                                                                                                   __m512i>::type>::type>::type;

      static constexpr size_t length = N;
      static constexpr size_t size = sizeof(native_t) * N;

      SIMD() = default;
      SIMD(const native_t o) {
          *this = o;
      }
      SIMD(const simd_t &o) {
          v = o;
      }


      __RG_STRONG_INLINE__ SIMD<T, N> operator==(const SIMD<T, N> &o) const;
      __RG_STRONG_INLINE__ SIMD<T, N> operator!() const;
      __RG_STRONG_INLINE__ SIMD<T, N> &operator=(const SIMD<T, N>::native_t o);
      __RG_STRONG_INLINE__ SIMD<T, N> operator+(const SIMD<T, N> &o) const;
      __RG_STRONG_INLINE__ SIMD<T, N> operator-(const SIMD<T, N> &o) const;
      __RG_STRONG_INLINE__ SIMD<T, N> operator>(const SIMD<T, N> &o) const;
      __RG_STRONG_INLINE__ SIMD<T, N> operator<(const SIMD<T, N> &o) const;
      __RG_STRONG_INLINE__ SIMD<T, N> operator&(const SIMD<T, N> &o) const;
      __RG_STRONG_INLINE__ SIMD<T, N> operator|(const SIMD<T, N> &o) const;
      __RG_STRONG_INLINE__ SIMD<T, N> &operator=(const SIMD<T, N> &o) {
          v = o.v;
          return *this;
      };
      __RG_STRONG_INLINE__ SIMD<T, N> operator>=(const SIMD<T, N> &o) const {
          return !(*this < o);
      };
      __RG_STRONG_INLINE__ SIMD<T, N> operator<=(const SIMD<T, N> &o) const {
          return !(*this > o);
      };
      __RG_STRONG_INLINE__ SIMD<T, N> operator!=(const SIMD<T, N> &o) const {
          return !(*this == o);
      };
      __RG_STRONG_INLINE__ bool any() const;
      __RG_STRONG_INLINE__ SIMD<T, N> and_not(const SIMD<T, N> &o) const;

      simd_t v;
  };

  template<typename T>
  __RG_STRONG_INLINE__
  typename T::native_t extract(const size_t i, const T &v) {
      return reinterpret_cast<const typename T::native_t *>(&v.v)[i];
  }

  template<typename T>
  __RG_STRONG_INLINE__
  void insert(typename T::native_t elem, const size_t i, T &v) {
      reinterpret_cast<typename T::native_t *>(&v.v)[i] = elem;
  }

  // SSE2
  using int8x16 = SIMD<int8_t, 16>;
  using int16x8 = SIMD<int16_t, 8>;
  // AVX2
  using int8x32 = SIMD<int8_t, 32>;
  using int16x16 = SIMD<int16_t, 16>;

  template<typename T>
  using SIMDVector = std::vector<T, aligned_allocator<T, T::size>>;

  /************************************ 128b ************************************/

  #ifdef VA_SIMD_USE_SSE

  template<>
  int8x16 int8x16::operator==(const int8x16 &o) const {
      return _mm_cmpeq_epi8(v, o.v);
  }
  template<>
  int8x16 int8x16::operator!() const {
      // XOR with all ones
      return _mm_xor_si128(v, _mm_cmpeq_epi8(v, v));
  }
  template<>
  int8x16 &int8x16::operator=(const int8x16::native_t o) {
      v = _mm_set1_epi8(o);
      return *this;
  }
  template<>
  int8x16 int8x16::operator+(const int8x16 &o) const {
      return _mm_adds_epi8(v, o.v);
  }
  template<>
  int8x16 int8x16::operator-(const int8x16 &o) const {
      return _mm_subs_epi8(v, o.v);
  }
  template<>
  int8x16 int8x16::operator>(const int8x16 &o) const {
      return _mm_cmpgt_epi8(v, o.v);
  }
  template<>
  int8x16 int8x16::operator<(const int8x16 &o) const {
      return _mm_cmplt_epi8(v, o.v);
  }
  template<>
  int8x16 int8x16::operator&(const int8x16 &o) const {
      return _mm_and_si128(v, o.v);
  }
  template<>
  int8x16 int8x16::operator|(const int8x16 &o) const {
      return _mm_or_si128(v, o.v);
  }
  template<>
  bool int8x16::any() const {
      return _mm_movemask_epi8(v);
  }
  template<>
  int8x16 int8x16::and_not(const int8x16 &o) const {
      return _mm_andnot_si128(o.v, v);
  }

  __RG_STRONG_INLINE__ int8x16 max(const int8x16 &a, const int8x16 &b) {
      return _mm_max_epi8(a.v, b.v);
  }
  __RG_STRONG_INLINE__ int8x16 blend(const int8x16 &mask, const int8x16 &t, const int8x16 &f) {
      return _mm_blendv_epi8(f.v, t.v, mask.v);
  }


  template<>
  int16x8 int16x8::operator==(const int16x8 &o) const {
      return _mm_cmpeq_epi16(v, o.v);
  }
  template<>
  int16x8 int16x8::operator!() const {
      return _mm_xor_si128(v, _mm_cmpeq_epi16(v, v));
  }
  template<>
  int16x8 &int16x8::operator=(const int16x8::native_t o) {
      v = _mm_set1_epi16(o);
      return *this;
  }
  template<>
  int16x8 int16x8::operator+(const int16x8 &o) const {
      return _mm_adds_epi16(v, o.v);
  }
  template<>
  int16x8 int16x8::operator-(const int16x8 &o) const {
      return _mm_subs_epi16(v, o.v);
  }
  template<>
  int16x8 int16x8::operator>(const int16x8 &o) const {
      return _mm_cmpgt_epi16(v, o.v);
  }
  template<>
  int16x8 int16x8::operator<(const int16x8 &o) const {
      return _mm_cmplt_epi16(v, o.v);
  }
  template<>
  int16x8 int16x8::operator&(const int16x8 &o) const {
      return _mm_and_si128(v, o.v);
  }
  template<>
  int16x8 int16x8::operator|(const int16x8 &o) const {
      return _mm_or_si128(v, o.v);
  }
  template<>
  bool int16x8::any() const {
      return _mm_movemask_epi8(v);
  }
  template<>
  int16x8 int16x8::and_not(const int16x8 &o) const {
      return _mm_andnot_si128(o.v, v);
  }
  __RG_STRONG_INLINE__ int16x8 max(const int16x8 &a, const int16x8 &b) {
      return _mm_max_epi16(a.v, b.v);
  }
  __RG_STRONG_INLINE__ int16x8 blend(const int16x8 &mask, const int16x8 &t, const int16x8 &f) {
      return _mm_blendv_epi8(f.v, t.v, mask.v);
  }

  #endif

  /************************************ 256b ************************************/

  #ifdef VA_SIMD_USE_AVX2

  template<> int8x32 int8x32::operator==(const int8x32 &o) const {
      return _mm256_cmpeq_epi8(v, o.v);
  }
  template<> int8x32 int8x32::operator!() const {
      return _mm256_xor_si256(v, _mm256_cmpeq_epi8(v,v));
  }
  template<> int8x32 &int8x32::operator=(const int8x32::native_t o) {
      v = _mm256_set1_epi8(o);
      return *this;
  }
  template<> int8x32 int8x32::operator+(const int8x32 &o) const {
      return _mm256_adds_epi8(v, o.v);
  }
  template<> int8x32 int8x32::operator-(const int8x32 &o) const {
      return _mm256_subs_epi8(v, o.v);
  }
  template<> int8x32 int8x32::operator>(const int8x32 &o) const {
      return _mm256_cmpgt_epi8(v, o.v);
  }
  template<> int8x32 int8x32::operator<(const int8x32 &o) const {
      return _mm256_cmpgt_epi8(o.v, v);
  }
  template<> int8x32 int8x32::operator&(const int8x32 &o) const {
      return _mm256_and_si256(v, o.v);
  }
  template<> int8x32 int8x32::operator|(const int8x32 &o) const {
      return _mm256_or_si256(v, o.v);
  }
  template<> bool int8x32::any() const {
      return _mm256_movemask_epi8(v);
  }
    template <>
  int8x32 int8x32::and_not(const int8x32 &o) const {
      return _mm256_andnot_si256(o.v, v);
  }
  __RG_STRONG_INLINE__ int8x32 max(const int8x32 &a, const int8x32 &b) {
      return _mm256_max_epi8(a.v, b.v);
  }
  __RG_STRONG_INLINE__ int8x32 blend(const int8x32 &mask, const int8x32 &t, const int8x32 &f) {
      return _mm256_blendv_epi8(f.v, t.v, mask.v);
  }


  template<> int16x16 int16x16::operator==(const int16x16 &o) const {
      return _mm256_cmpeq_epi16(v, o.v);
  }
  template<> int16x16 int16x16::operator!() const {
      return _mm256_xor_si256(v, _mm256_cmpeq_epi16(v,v));
  }
  template<> int16x16 &int16x16::operator=(const int16x16::native_t o) {
      v = _mm256_set1_epi16(o);
      return *this;
  }
  template<> int16x16 int16x16::operator+(const int16x16 &o) const {
      return _mm256_adds_epi16(v, o.v);
  }
  template<> int16x16 int16x16::operator-(const int16x16 &o) const {
      return _mm256_subs_epi16(v, o.v);
  }
  template<> int16x16 int16x16::operator>(const int16x16 &o) const {
      return _mm256_cmpgt_epi16(v, o.v);
  }
  template<> int16x16 int16x16::operator<(const int16x16 &o) const {
      return _mm256_cmpgt_epi16(o.v, v);
  }
  template<> int16x16 int16x16::operator&(const int16x16 &o) const {
      return _mm256_and_si256(v, o.v);
  }
  template<> int16x16 int16x16::operator|(const int16x16 &o) const {
      return _mm256_or_si256(v, o.v);
  }
  template<> bool int16x16::any() const {
      return _mm256_movemask_epi8(v);
  }
  template <>
  int16x16 int16x16::and_not(const int16x16 &o) const {
      return _mm256_andnot_si256(o.v, v);
  }
  __RG_STRONG_INLINE__ int16x16 max(const int16x16 &a, const int16x16 &b) {
      return _mm256_max_epi16(a.v, b.v);
  }
  __RG_STRONG_INLINE__ int16x16 blend(const int16x16 &mask, const int16x16 &t, const int16x16 &f) {
      return _mm256_blendv_epi8(f.v, t.v, mask.v);
  }

  #endif

}

TEST_CASE ("SIMD") {
    vargas::SIMD<int8_t, 16> a, b;
    a = 10;
    b = -4;
    auto c = a - b;
    auto d = a < c;

    for (size_t i = 0; i < 16; ++i) {
        CHECK((int) vargas::extract(i, a) == 10);
        CHECK((int) vargas::extract(i, b) == -4);
        CHECK((int) vargas::extract(i, c) == 14);
        CHECK((uint8_t) vargas::extract(i, d) == 0xFF);
    }

}


#endif //VARGAS_SIMD_H