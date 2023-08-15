// Copyright 2020-2023 Junekey Jeon
//
// The contents of this file may be used under the terms of
// the Apache License v2.0 with LLVM Exceptions.
//
//    (See accompanying file LICENSE-Apache or copy at
//     https://llvm.org/foundation/relicensing/LICENSE.txt)
//
// Alternatively, the contents of this file may be used under the terms of
// the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE-Boost or copy at
//     https://www.boost.org/LICENSE_1_0.txt)
//
// Unless required by applicable law or agreed to in writing, this software
// is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.


#ifndef JKJ_HEADER_DRAGONBOX
#define JKJ_HEADER_DRAGONBOX

#include <cassert>
#include <cstdint>
#include <cstring>
#include <limits>
#include <type_traits>

#ifdef __has_include
    #if __has_include(<version>)
        #include <version>
    #endif
#endif

////////////////////////////////////////////////////////////////////////////////////////
// Language feature detections.
////////////////////////////////////////////////////////////////////////////////////////

// C++14 constexpr
#if defined(__cpp_constexpr) && __cpp_constexpr >= 201304L
    #define JKJ_HAS_CONSTEXPR14 1
#elif __cplusplus >= 201402L
    #define JKJ_HAS_CONSTEXPR14 1
#elif defined(_MSC_VER) && _MSC_VER >= 1910 && _MSVC_LANG >= 201402L
    #define JKJ_HAS_CONSTEXPR14 1
#else
    #define JKJ_HAS_CONSTEXPR14 0
#endif

#if JKJ_HAS_CONSTEXPR14
    #define JKJ_CONSTEXPR14 constexpr
#else
    #define JKJ_CONSTEXPR14
#endif

// C++17 constexpr lambdas
#if defined(__cpp_constexpr) && __cpp_constexpr >= 201603L
    #define JKJ_HAS_CONSTEXPR17 1
#elif __cplusplus >= 201703L
    #define JKJ_HAS_CONSTEXPR17 1
#elif defined(_MSC_VER) && _MSC_VER >= 1911 && _MSVC_LANG >= 201703L
    #define JKJ_HAS_CONSTEXPR17 1
#else
    #define JKJ_HAS_CONSTEXPR17 0
#endif

// C++17 inline variables
#if defined(__cpp_inline_variables) && __cpp_inline_variables >= 201606L
    #define JKJ_HAS_INLINE_VARIABLE 1
#elif __cplusplus >= 201703L
    #define JKJ_HAS_INLINE_VARIABLE 1
#elif defined(_MSC_VER) && _MSC_VER >= 1912 && _MSVC_LANG >= 201703L
    #define JKJ_HAS_INLINE_VARIABLE 1
#else
    #define JKJ_HAS_INLINE_VARIABLE 0
#endif

#if JKJ_HAS_INLINE_VARIABLE
    #define JKJ_INLINE_VARIABLE inline constexpr
#else
    #define JKJ_INLINE_VARIABLE static constexpr
#endif

// C++17 if constexpr
#if defined(__cpp_if_constexpr) && __cpp_if_constexpr >= 201606L
    #define JKJ_HAS_IF_CONSTEXPR 1
#elif __cplusplus >= 201703L
    #define JKJ_HAS_IF_CONSTEXPR 1
#elif defined(_MSC_VER) && _MSC_VER >= 1911 && _MSVC_LANG >= 201703L
    #define JKJ_HAS_IF_CONSTEXPR 1
#else
    #define JKJ_HAS_IF_CONSTEXPR 0
#endif

#if JKJ_HAS_IF_CONSTEXPR
    #define JKJ_IF_CONSTEXPR if constexpr
#else
    #define JKJ_IF_CONSTEXPR if
#endif

// C++20 std::bit_cast
#if defined(__cpp_lib_bit_cast) && __cpp_lib_bit_cast >= 201806L
    #include <bit>
    #define JKJ_HAS_BIT_CAST 1
#else
    #define JKJ_HAS_BIT_CAST 0
#endif

// C++23 if consteval or C++20 std::is_constant_evaluated
#if defined(__cpp_if_consteval) && __cpp_is_consteval >= 202106L
    #define JKJ_IF_CONSTEVAL if consteval
    #define JKJ_IF_NOT_CONSTEVAL if !consteval
    #define JKJ_CAN_BRANCH_ON_CONSTEVAL 1
#elif defined(__cpp_lib_is_constant_evaluated) && __cpp_lib_is_constant_evaluated >= 201811L
    #define JKJ_IF_CONSTEVAL if (std::is_constant_evaluated())
    #define JKJ_IF_NOT_CONSTEVAL if (!std::is_constant_evaluated())
    #define JKJ_CAN_BRANCH_ON_CONSTEVAL 1
#elif JKJ_HAS_IF_CONSTEXPR
    #define JKJ_IF_CONSTEVAL if constexpr (false)
    #define JKJ_IF_NOT_CONSTEVAL if constexpr (true)
    #define JKJ_CAN_BRANCH_ON_CONSTEVAL 0
#else
    #define JKJ_IF_CONSTEVAL if (false)
    #define JKJ_IF_NOT_CONSTEVAL if (true)
    #define JKJ_CAN_BRANCH_ON_CONSTEVAL 0
#endif

#if JKJ_CAN_BRANCH_ON_CONSTEVAL && JKJ_HAS_BIT_CAST
    #define JKJ_CONSTEXPR20 constexpr
#else
    #define JKJ_CONSTEXPR20
#endif

// Suppress additional buffer overrun check.
// I have no idea why MSVC thinks some functions here are vulnerable to the buffer overrun
// attacks. No, they aren't.
#if defined(__GNUC__) || defined(__clang__)
    #define JKJ_SAFEBUFFERS
    #define JKJ_FORCEINLINE inline __attribute__((always_inline))
#elif defined(_MSC_VER)
    #define JKJ_SAFEBUFFERS __declspec(safebuffers)
    #define JKJ_FORCEINLINE __forceinline
#else
    #define JKJ_SAFEBUFFERS
    #define JKJ_FORCEINLINE inline
#endif

#if defined(__has_builtin)
    #define JKJ_HAS_BUILTIN(x) __has_builtin(x)
#else
    #define JKJ_HAS_BUILTIN(x) false
#endif

#if defined(_MSC_VER)
    #include <intrin.h>
#endif

namespace jkj {
    namespace dragonbox {
        ////////////////////////////////////////////////////////////////////////////////////////
        // Some general utilities for C++11-compatibility
        ////////////////////////////////////////////////////////////////////////////////////////
        namespace detail {
#if !JKJ_HAS_CONSTEXPR17
            template <std::size_t... indices>
            struct index_sequence {};

            template <std::size_t current, std::size_t total, class Dummy, std::size_t... indices>
            struct make_index_sequence_impl {
                using type = typename make_index_sequence_impl<current + 1, total, Dummy, indices...,
                                                               current>::type;
            };

            template <std::size_t total, class Dummy, std::size_t... indices>
            struct make_index_sequence_impl<total, total, Dummy, indices...> {
                using type = index_sequence<indices...>;
            };

            template <std::size_t N>
            using make_index_sequence = typename make_index_sequence_impl<0, N, void>::type;
#endif

            template <class T>
            typename std::add_rvalue_reference<T>::type declval() noexcept;
        }


        ////////////////////////////////////////////////////////////////////////////////////////
        // Some basic features for encoding/decoding IEEE-754 formats.
        ////////////////////////////////////////////////////////////////////////////////////////
        namespace detail {
            template <class T>
            struct physical_bits {
                static constexpr std::size_t value =
                    sizeof(T) * std::numeric_limits<unsigned char>::digits;
            };
            template <class T>
            struct value_bits {
                static constexpr std::size_t value = std::numeric_limits<
                    typename std::enable_if<std::is_unsigned<T>::value, T>::type>::digits;
            };

            template <typename To, typename From>
            JKJ_CONSTEXPR20 To bit_cast(const From& from) {
#if JKJ_HAS_BIT_CAST
                return std::bit_cast<To>(from);
#else
                static_assert(sizeof(From) == sizeof(To), "");
                static_assert(std::is_trivially_copyable<To>::value, "");
                static_assert(std::is_trivially_copyable<From>::value, "");
                To to;
                std::memcpy(&to, &from, sizeof(To));
                return to;
#endif
            }
        }

        // These classes expose encoding specs of IEEE-754-like floating-point formats.
        // Currently available formats are IEEE754-binary32 & IEEE754-binary64.

        struct ieee754_binary32 {
            static constexpr int significand_bits = 23;
            static constexpr int exponent_bits = 8;
            static constexpr int min_exponent = -126;
            static constexpr int max_exponent = 127;
            static constexpr int exponent_bias = -127;
            static constexpr int decimal_digits = 9;
        };
        struct ieee754_binary64 {
            static constexpr int significand_bits = 52;
            static constexpr int exponent_bits = 11;
            static constexpr int min_exponent = -1022;
            static constexpr int max_exponent = 1023;
            static constexpr int exponent_bias = -1023;
            static constexpr int decimal_digits = 17;
        };

        // A floating-point traits class defines ways to interpret a bit pattern of given size as an
        // encoding of floating-point number. This is a default implementation of such a traits
        // class, supporting ways to interpret 32-bits into a binary32-encoded floating-point number
        // and to interpret 64-bits into a binary64-encoded floating-point number. Users might
        // specialize this class to change the default behavior for certain types.
        template <class T>
        struct default_float_traits {
            // I don't know if there is a truly reliable way of detecting
            // IEEE-754 binary32/binary64 formats; I just did my best here.
            static_assert(std::numeric_limits<T>::is_iec559 && std::numeric_limits<T>::radix == 2 &&
                              (detail::physical_bits<T>::value == 32 ||
                               detail::physical_bits<T>::value == 64),
                          "default_ieee754_traits only works for 32-bits or 64-bits types "
                          "supporting binary32 or binary64 formats!");

            // The type that is being viewed.
            using type = T;

            // Refers to the format specification class.
            using format = typename std::conditional<detail::physical_bits<T>::value == 32,
                                                     ieee754_binary32, ieee754_binary64>::type;

            // Defines an unsigned integer type that is large enough to carry a variable of type T.
            // Most of the operations will be done on this integer type.
            using carrier_uint = typename std::conditional<detail::physical_bits<T>::value == 32,
                                                           std::uint32_t, std::uint64_t>::type;
            static_assert(sizeof(carrier_uint) == sizeof(T), "");

            // Number of bits in the above unsigned integer type.
            static constexpr int carrier_bits = int(detail::physical_bits<carrier_uint>::value);

            // Convert from carrier_uint into the original type.
            // Depending on the floating-point encoding format, this operation might not be possible
            // for some specific bit patterns. However, the contract is that u always denotes a
            // valid bit pattern, so this function must be assumed to be noexcept.
            static JKJ_CONSTEXPR20 T carrier_to_float(carrier_uint u) noexcept {
                return detail::bit_cast<T>(u);
            }

            // Same as above.
            static JKJ_CONSTEXPR20 carrier_uint float_to_carrier(T x) noexcept {
                return detail::bit_cast<carrier_uint>(x);
            }

            // Extract exponent bits from a bit pattern.
            // The result must be aligned to the LSB so that there is no additional zero paddings
            // on the right. This function does not do bias adjustment.
            static constexpr unsigned int extract_exponent_bits(carrier_uint u) noexcept {
                static_assert(detail::value_bits<unsigned int>::value > format::exponent_bits, "");
                return static_cast<unsigned int>(u >> format::significand_bits) &
                       ((static_cast<unsigned int>(1) << format::exponent_bits) - 1);
            }

            // Extract significand bits from a bit pattern.
            // The result must be aligned to the LSB so that there is no additional zero paddings
            // on the right. The result does not contain the implicit bit.
            static constexpr carrier_uint extract_significand_bits(carrier_uint u) noexcept {
                return carrier_uint(u &
                                    carrier_uint((carrier_uint(1) << format::significand_bits) - 1));
            }

            // Remove the exponent bits and extract significand bits together with the sign bit.
            static constexpr carrier_uint remove_exponent_bits(carrier_uint u,
                                                               unsigned int exponent_bits) noexcept {
                return u ^ (carrier_uint(exponent_bits) << format::significand_bits);
            }

            // Shift the obtained signed significand bits to the left by 1 to remove the sign bit.
            static constexpr carrier_uint remove_sign_bit_and_shift(carrier_uint u) noexcept {
                return carrier_uint(carrier_uint(u) << 1);
            }

            // The actual value of exponent is obtained by adding this value to the extracted
            // exponent bits.
            static constexpr int exponent_bias =
                1 - (1 << (carrier_bits - format::significand_bits - 2));

            // Obtain the actual value of the binary exponent from the extracted exponent bits.
            static constexpr int binary_exponent(unsigned int exponent_bits) noexcept {
                return exponent_bits == 0 ? format::min_exponent
                                          : int(exponent_bits) + format::exponent_bias;
            }

            // Obtain the actual value of the binary exponent from the extracted significand bits
            // and exponent bits.
            static constexpr carrier_uint binary_significand(carrier_uint significand_bits,
                                                             unsigned int exponent_bits) noexcept {
                return exponent_bits == 0
                           ? significand_bits
                           : (significand_bits | (carrier_uint(1) << format::significand_bits));
            }


            /* Various boolean observer functions */

            static constexpr bool is_nonzero(carrier_uint u) noexcept { return (u << 1) != 0; }
            static constexpr bool is_positive(carrier_uint u) noexcept {
                return u < (carrier_uint(1) << (format::significand_bits + format::exponent_bits));
            }
            static constexpr bool is_negative(carrier_uint u) noexcept { return !is_positive(u); }
            static constexpr bool is_finite(unsigned int exponent_bits) noexcept {
                return exponent_bits != ((1u << format::exponent_bits) - 1);
            }
            static constexpr bool has_all_zero_significand_bits(carrier_uint u) noexcept {
                return (u << 1) == 0;
            }
            static constexpr bool has_even_significand_bits(carrier_uint u) noexcept {
                return u % 2 == 0;
            }
        };

        // Convenient wrappers for floating-point traits classes.
        // In order to reduce the argument passing overhead, these classes should be as simple as
        // possible (e.g., no inheritance, no private non-static data member, etc.; this is an
        // unfortunate fact about common ABI convention).

        template <class T, class Traits = default_float_traits<T>>
        struct float_bits;

        template <class T, class Traits = default_float_traits<T>>
        struct signed_significand_bits;

        template <class T, class Traits>
        struct float_bits {
            using type = T;
            using traits_type = Traits;
            using carrier_uint = typename traits_type::carrier_uint;

            carrier_uint u;

            float_bits() = default;
            constexpr explicit float_bits(carrier_uint bit_pattern) noexcept : u{bit_pattern} {}
            constexpr explicit float_bits(T float_value) noexcept
                : u{traits_type::float_to_carrier(float_value)} {}

            constexpr T to_float() const noexcept { return traits_type::carrier_to_float(u); }

            // Extract exponent bits from a bit pattern.
            // The result must be aligned to the LSB so that there is no additional zero paddings
            // on the right. This function does not do bias adjustment.
            constexpr unsigned int extract_exponent_bits() const noexcept {
                return traits_type::extract_exponent_bits(u);
            }

            // Extract significand bits from a bit pattern.
            // The result must be aligned to the LSB so that there is no additional zero paddings
            // on the right. The result does not contain the implicit bit.
            constexpr carrier_uint extract_significand_bits() const noexcept {
                return traits_type::extract_significand_bits(u);
            }

            // Remove the exponent bits and extract significand bits together with the sign bit.
            constexpr signed_significand_bits<type, traits_type>
            remove_exponent_bits(unsigned int exponent_bits) const noexcept {
                return signed_significand_bits<type, traits_type>(
                    traits_type::remove_exponent_bits(u, exponent_bits));
            }

            // Obtain the actual value of the binary exponent from the extracted exponent bits.
            static constexpr int binary_exponent(unsigned int exponent_bits) noexcept {
                return traits_type::binary_exponent(exponent_bits);
            }
            constexpr int binary_exponent() const noexcept {
                return binary_exponent(extract_exponent_bits());
            }

            // Obtain the actual value of the binary exponent from the extracted significand bits
            // and exponent bits.
            static constexpr carrier_uint binary_significand(carrier_uint significand_bits,
                                                             unsigned int exponent_bits) noexcept {
                return traits_type::binary_significand(significand_bits, exponent_bits);
            }
            constexpr carrier_uint binary_significand() const noexcept {
                return binary_significand(extract_significand_bits(), extract_exponent_bits());
            }

            constexpr bool is_nonzero() const noexcept { return traits_type::is_nonzero(u); }
            constexpr bool is_positive() const noexcept { return traits_type::is_positive(u); }
            constexpr bool is_negative() const noexcept { return traits_type::is_negative(u); }
            constexpr bool is_finite(unsigned int exponent_bits) const noexcept {
                return traits_type::is_finite(exponent_bits);
            }
            constexpr bool is_finite() const noexcept {
                return traits_type::is_finite(extract_exponent_bits());
            }
            constexpr bool has_even_significand_bits() const noexcept {
                return traits_type::has_even_significand_bits(u);
            }
        };

        template <class T, class Traits>
        struct signed_significand_bits {
            using type = T;
            using traits_type = Traits;
            using carrier_uint = typename traits_type::carrier_uint;

            carrier_uint u;

            signed_significand_bits() = default;
            constexpr explicit signed_significand_bits(carrier_uint bit_pattern) noexcept
                : u{bit_pattern} {}

            // Shift the obtained signed significand bits to the left by 1 to remove the sign bit.
            constexpr carrier_uint remove_sign_bit_and_shift() const noexcept {
                return traits_type::remove_sign_bit_and_shift(u);
            }

            constexpr bool is_positive() const noexcept { return traits_type::is_positive(u); }
            constexpr bool is_negative() const noexcept { return traits_type::is_negative(u); }
            constexpr bool has_all_zero_significand_bits() const noexcept {
                return traits_type::has_all_zero_significand_bits(u);
            }
            constexpr bool has_even_significand_bits() const noexcept {
                return traits_type::has_even_significand_bits(u);
            }
        };

        namespace detail {
            ////////////////////////////////////////////////////////////////////////////////////////
            // Bit operation intrinsics.
            ////////////////////////////////////////////////////////////////////////////////////////

            namespace bits {
                // Most compilers should be able to optimize this into the ROR instruction.
                inline JKJ_CONSTEXPR14 std::uint32_t rotr(std::uint32_t n, std::uint32_t r) noexcept {
                    r &= 31;
                    return (n >> r) | (n << (32 - r));
                }
                inline JKJ_CONSTEXPR14 std::uint64_t rotr(std::uint64_t n, std::uint32_t r) noexcept {
                    r &= 63;
                    return (n >> r) | (n << (64 - r));
                }
            }

            ////////////////////////////////////////////////////////////////////////////////////////
            // Utilities for wide unsigned integer arithmetic.
            ////////////////////////////////////////////////////////////////////////////////////////

            namespace wuint {
                // Compilers might support built-in 128-bit integer types. However, it seems that
                // emulating them with a pair of 64-bit integers actually produces a better code,
                // so we avoid using those built-ins. That said, they are still useful for
                // implementing 64-bit x 64-bit -> 128-bit multiplication.

                // clang-format off
#if defined(__SIZEOF_INT128__)
		// To silence "error: ISO C++ does not support '__int128' for 'type name'
		// [-Wpedantic]"
#if defined(__GNUC__)
			__extension__
#endif
			using builtin_uint128_t = unsigned __int128;
#endif
                // clang-format on

                struct uint128 {
                    uint128() = default;

                    std::uint64_t high_;
                    std::uint64_t low_;

                    constexpr uint128(std::uint64_t high, std::uint64_t low) noexcept
                        : high_{high}, low_{low} {}

                    constexpr std::uint64_t high() const noexcept { return high_; }
                    constexpr std::uint64_t low() const noexcept { return low_; }

                    JKJ_CONSTEXPR20 uint128& operator+=(std::uint64_t n) & noexcept {
                        auto const generic_impl = [&] {
                            auto sum = low_ + n;
                            high_ += (sum < low_ ? 1 : 0);
                            low_ = sum;
                        };
                        JKJ_IF_CONSTEVAL {
                            generic_impl();
                            return *this;
                        }
#if JKJ_HAS_BUILTIN(__builtin_addcll)
                        unsigned long long carry{};
                        low_ = __builtin_addcll(low_, n, 0, &carry);
                        high_ = __builtin_addcll(high_, 0, carry, &carry);
#elif JKJ_HAS_BUILTIN(__builtin_ia32_addcarryx_u64)
                        unsigned long long result{};
                        auto carry = __builtin_ia32_addcarryx_u64(0, low_, n, &result);
                        low_ = result;
                        __builtin_ia32_addcarryx_u64(carry, high_, 0, &result);
                        high_ = result;
#elif defined(_MSC_VER) && defined(_M_X64)
                        auto carry = _addcarry_u64(0, low_, n, &low_);
                        _addcarry_u64(carry, high_, 0, &high_);
#else
                        generic_impl();
#endif
                        return *this;
                    }
                };

                inline JKJ_CONSTEXPR20 std::uint64_t umul64(std::uint32_t x, std::uint32_t y) noexcept {
#if defined(_MSC_VER) && defined(_M_IX86)
                    JKJ_IF_NOT_CONSTEVAL { return __emulu(x, y); }
#endif
                    return x * std::uint64_t(y);
                }

                // Get 128-bit result of multiplication of two 64-bit unsigned integers.
                JKJ_SAFEBUFFERS inline JKJ_CONSTEXPR20 uint128 umul128(std::uint64_t x,
                                                                       std::uint64_t y) noexcept {
                    auto const generic_impl = [&]() -> uint128 {
                        auto a = std::uint32_t(x >> 32);
                        auto b = std::uint32_t(x);
                        auto c = std::uint32_t(y >> 32);
                        auto d = std::uint32_t(y);

                        auto ac = umul64(a, c);
                        auto bc = umul64(b, c);
                        auto ad = umul64(a, d);
                        auto bd = umul64(b, d);

                        auto intermediate = (bd >> 32) + std::uint32_t(ad) + std::uint32_t(bc);

                        return {ac + (intermediate >> 32) + (ad >> 32) + (bc >> 32),
                                (intermediate << 32) + std::uint32_t(bd)};
                    };
                    JKJ_IF_CONSTEVAL { return generic_impl(); }
#if defined(__SIZEOF_INT128__)
                    auto result = builtin_uint128_t(x) * builtin_uint128_t(y);
                    return {std::uint64_t(result >> 64), std::uint64_t(result)};
#elif defined(_MSC_VER) && defined(_M_X64)
                    uint128 result;
                    result.low_ = _umul128(x, y, &result.high_);
                    return result;
#else
                    return generic_impl();
#endif
                }

                JKJ_SAFEBUFFERS inline JKJ_CONSTEXPR20 std::uint64_t
                umul128_upper64(std::uint64_t x, std::uint64_t y) noexcept {
                    auto const generic_impl = [&]() -> std::uint64_t {
                        auto a = std::uint32_t(x >> 32);
                        auto b = std::uint32_t(x);
                        auto c = std::uint32_t(y >> 32);
                        auto d = std::uint32_t(y);

                        auto ac = umul64(a, c);
                        auto bc = umul64(b, c);
                        auto ad = umul64(a, d);
                        auto bd = umul64(b, d);

                        auto intermediate = (bd >> 32) + std::uint32_t(ad) + std::uint32_t(bc);

                        return ac + (intermediate >> 32) + (ad >> 32) + (bc >> 32);
                    };
                    JKJ_IF_CONSTEVAL { return generic_impl(); }
#if defined(__SIZEOF_INT128__)
                    auto result = builtin_uint128_t(x) * builtin_uint128_t(y);
                    return std::uint64_t(result >> 64);
#elif defined(_MSC_VER) && defined(_M_X64)
                    return __umulh(x, y);
#else
                    return generic_impl();
#endif
                }

                // Get upper 128-bits of multiplication of a 64-bit unsigned integer and a 128-bit
                // unsigned integer.
                JKJ_SAFEBUFFERS inline JKJ_CONSTEXPR20 uint128 umul192_upper128(std::uint64_t x,
                                                                                uint128 y) noexcept {
                    auto r = umul128(x, y.high());
                    r += umul128_upper64(x, y.low());
                    return r;
                }

                // Get upper 64-bits of multiplication of a 32-bit unsigned integer and a 64-bit
                // unsigned integer.
                inline JKJ_CONSTEXPR20 std::uint64_t umul96_upper64(std::uint32_t x,
                                                                    std::uint64_t y) noexcept {
#if defined(__SIZEOF_INT128__) || (defined(_MSC_VER) && defined(_M_X64))
                    return umul128_upper64(std::uint64_t(x) << 32, y);
#else
                    auto yh = std::uint32_t(y >> 32);
                    auto yl = std::uint32_t(y);

                    auto xyh = umul64(x, yh);
                    auto xyl = umul64(x, yl);

                    return xyh + (xyl >> 32);
#endif
                }

                // Get lower 128-bits of multiplication of a 64-bit unsigned integer and a 128-bit
                // unsigned integer.
                JKJ_SAFEBUFFERS inline JKJ_CONSTEXPR20 uint128 umul192_lower128(std::uint64_t x,
                                                                                uint128 y) noexcept {
                    auto high = x * y.high();
                    auto high_low = umul128(x, y.low());
                    return {high + high_low.high(), high_low.low()};
                }

                // Get lower 64-bits of multiplication of a 32-bit unsigned integer and a 64-bit
                // unsigned integer.
                constexpr std::uint64_t umul96_lower64(std::uint32_t x, std::uint64_t y) noexcept {
                    return x * y;
                }
            }

            ////////////////////////////////////////////////////////////////////////////////////////
            // Some simple utilities for constexpr computation.
            ////////////////////////////////////////////////////////////////////////////////////////

            template <int k, class Int>
            constexpr Int compute_power(Int a) noexcept {
                static_assert(k >= 0, "");
#if JKJ_HAS_CONSTEXPR14
                Int p = 1;
                for (int i = 0; i < k; ++i) {
                    p *= a;
                }
                return p;
#else
                return k == 0       ? 1
                       : k % 2 == 0 ? compute_power<k / 2, Int>(a * a)
                                    : a * compute_power<k / 2, Int>(a * a);
#endif
            }

            template <int a, class UInt>
            constexpr int count_factors(UInt n) noexcept {
                static_assert(a > 1, "");
#if JKJ_HAS_CONSTEXPR14
                int c = 0;
                while (n % a == 0) {
                    n /= a;
                    ++c;
                }
                return c;
#else
                return n % a == 0 ? count_factors<a, UInt>(n / a) + 1 : 0;
#endif
            }

            ////////////////////////////////////////////////////////////////////////////////////////
            // Utilities for fast/constexpr log computation.
            ////////////////////////////////////////////////////////////////////////////////////////

            namespace log {
                static_assert((-1 >> 1) == -1, "right-shift for signed integers must be arithmetic");

                // Compute floor(e * c - s).
                enum class multiply : std::uint32_t {};
                enum class subtract : std::uint32_t {};
                enum class shift : std::size_t {};
                enum class min_exponent : std::int32_t {};
                enum class max_exponent : std::int32_t {};

                template <multiply m, subtract f, shift k, min_exponent e_min, max_exponent e_max>
                constexpr int compute(int e) noexcept {
#if JKJ_HAS_CONSTEXPR14
                    assert(std::int32_t(e_min) <= e && e <= std::int32_t(e_max));
#endif
                    return int((std::int32_t(e) * std::int32_t(m) - std::int32_t(f)) >> std::size_t(k));
                }

                // For constexpr computation.
                // Returns -1 when n = 0.
                template <class UInt>
                constexpr int floor_log2(UInt n) noexcept {
#if JKJ_HAS_CONSTEXPR14
                    int count = -1;
                    while (n != 0) {
                        ++count;
                        n >>= 1;
                    }
                    return count;
#else
                    return n == 0 ? -1 : floor_log2<UInt>(n / 2) + 1;
#endif
                }

                static constexpr int floor_log10_pow2_min_exponent = -2620;
                static constexpr int floor_log10_pow2_max_exponent = 2620;
                constexpr int floor_log10_pow2(int e) noexcept {
                    using namespace log;
                    return compute<multiply(315653), subtract(0), shift(20),
                                   min_exponent(floor_log10_pow2_min_exponent),
                                   max_exponent(floor_log10_pow2_max_exponent)>(e);
                }

                static constexpr int floor_log2_pow10_min_exponent = -1233;
                static constexpr int floor_log2_pow10_max_exponent = 1233;
                constexpr int floor_log2_pow10(int e) noexcept {
                    using namespace log;
                    return compute<multiply(1741647), subtract(0), shift(19),
                                   min_exponent(floor_log2_pow10_min_exponent),
                                   max_exponent(floor_log2_pow10_max_exponent)>(e);
                }

                static constexpr int floor_log10_pow2_minus_log10_4_over_3_min_exponent = -2985;
                static constexpr int floor_log10_pow2_minus_log10_4_over_3_max_exponent = 2936;
                constexpr int floor_log10_pow2_minus_log10_4_over_3(int e) noexcept {
                    using namespace log;
                    return compute<multiply(631305), subtract(261663), shift(21),
                                   min_exponent(floor_log10_pow2_minus_log10_4_over_3_min_exponent),
                                   max_exponent(floor_log10_pow2_minus_log10_4_over_3_max_exponent)>(e);
                }

                static constexpr int floor_log5_pow2_min_exponent = -1831;
                static constexpr int floor_log5_pow2_max_exponent = 1831;
                constexpr int floor_log5_pow2(int e) noexcept {
                    using namespace log;
                    return compute<multiply(225799), subtract(0), shift(19),
                                   min_exponent(floor_log5_pow2_min_exponent),
                                   max_exponent(floor_log5_pow2_max_exponent)>(e);
                }

                static constexpr int floor_log5_pow2_minus_log5_3_min_exponent = -3543;
                static constexpr int floor_log5_pow2_minus_log5_3_max_exponent = 2427;
                constexpr int floor_log5_pow2_minus_log5_3(int e) noexcept {
                    using namespace log;
                    return compute<multiply(451597), subtract(715764), shift(20),
                                   min_exponent(floor_log5_pow2_minus_log5_3_min_exponent),
                                   max_exponent(floor_log5_pow2_minus_log5_3_max_exponent)>(e);
                }
            }

            ////////////////////////////////////////////////////////////////////////////////////////
            // Utilities for fast divisibility tests.
            ////////////////////////////////////////////////////////////////////////////////////////

            namespace div {
                // Replace n by floor(n / 10^N).
                // Returns true if and only if n is divisible by 10^N.
                // Precondition: n <= 10^(N+1)
                // !!It takes an in-out parameter!!
                template <int N>
                struct divide_by_pow10_info;

                template <>
                struct divide_by_pow10_info<1> {
                    static constexpr std::uint32_t magic_number = 6554;
                    static constexpr int shift_amount = 16;
                };

                template <>
                struct divide_by_pow10_info<2> {
                    static constexpr std::uint32_t magic_number = 656;
                    static constexpr int shift_amount = 16;
                };

                template <int N>
                JKJ_CONSTEXPR14 bool check_divisibility_and_divide_by_pow10(std::uint32_t& n) noexcept {
                    // Make sure the computation for max_n does not overflow.
                    static_assert(N + 1 <= log::floor_log10_pow2(31), "");
                    assert(n <= compute_power<N + 1>(std::uint32_t(10)));

                    using info = divide_by_pow10_info<N>;
                    n *= info::magic_number;

                    constexpr auto mask = std::uint32_t(std::uint32_t(1) << info::shift_amount) - 1;
                    bool result = ((n & mask) < info::magic_number);

                    n >>= info::shift_amount;
                    return result;
                }

                // Compute floor(n / 10^N) for small n and N.
                // Precondition: n <= 10^(N+1)
                template <int N>
                JKJ_CONSTEXPR14 std::uint32_t small_division_by_pow10(std::uint32_t n) noexcept {
                    // Make sure the computation for max_n does not overflow.
                    static_assert(N + 1 <= log::floor_log10_pow2(31), "");
                    assert(n <= compute_power<N + 1>(std::uint32_t(10)));

                    return (n * divide_by_pow10_info<N>::magic_number) >>
                           divide_by_pow10_info<N>::shift_amount;
                }

                // Compute floor(n / 10^N) for small N.
                // Precondition: n <= n_max
                template <int N, class UInt, UInt n_max>
                JKJ_CONSTEXPR20 UInt divide_by_pow10(UInt n) noexcept {
                    static_assert(N >= 0, "");

                    // Specialize for 32-bit division by 100.
                    // Compiler is supposed to generate the identical code for just writing
                    // "n / 100", but for some reason MSVC generates an inefficient code
                    // (mul + mov for no apparent reason, instead of single imul),
                    // so we does this manually.
                    JKJ_IF_CONSTEXPR(std::is_same<UInt, std::uint32_t>::value && N == 2) {
                        return std::uint32_t(wuint::umul64(n, std::uint32_t(1374389535)) >> 37);
                    }
                    // Specialize for 64-bit division by 1000.
                    // Ensure that the correctness condition is met.
                    else JKJ_IF_CONSTEXPR(std::is_same<UInt, std::uint64_t>::value && N == 3 &&
                                          n_max <= std::uint64_t(15534100272597517998ull)) {
                        return wuint::umul128_upper64(n, std::uint64_t(2361183241434822607ull)) >> 7;
                    }
                    else {
                        constexpr auto divisor = compute_power<N>(UInt(10));
                        return n / divisor;
                    }
                }
            }
        }

        ////////////////////////////////////////////////////////////////////////////////////////
        // Return types for the main interface function.
        ////////////////////////////////////////////////////////////////////////////////////////

        template <class UInt, bool is_signed, bool trailing_zero_flag>
        struct decimal_fp;

        template <class UInt>
        struct decimal_fp<UInt, false, false> {
            using carrier_uint = UInt;

            carrier_uint significand;
            int exponent;
        };

        template <class UInt>
        struct decimal_fp<UInt, true, false> {
            using carrier_uint = UInt;

            carrier_uint significand;
            int exponent;
            bool is_negative;
        };

        template <class UInt>
        struct decimal_fp<UInt, false, true> {
            using carrier_uint = UInt;

            carrier_uint significand;
            int exponent;
            bool may_have_trailing_zeros;
        };

        template <class UInt>
        struct decimal_fp<UInt, true, true> {
            using carrier_uint = UInt;

            carrier_uint significand;
            int exponent;
            bool may_have_trailing_zeros;
            bool is_negative;
        };

        template <class UInt, bool trailing_zero_flag = false>
        using unsigned_decimal_fp = decimal_fp<UInt, false, trailing_zero_flag>;

        template <class UInt, bool trailing_zero_flag = false>
        using signed_decimal_fp = decimal_fp<UInt, true, trailing_zero_flag>;

        template <class UInt>
        constexpr signed_decimal_fp<UInt, false>
        add_sign_to_unsigned_decimal_fp(bool is_negative, unsigned_decimal_fp<UInt, false> r) noexcept {
            return {r.significand, r.exponent, is_negative};
        }

        template <class UInt>
        constexpr signed_decimal_fp<UInt, true>
        add_sign_to_unsigned_decimal_fp(bool is_negative, unsigned_decimal_fp<UInt, true> r) noexcept {
            return {r.significand, r.exponent, r.may_have_trailing_zeros, is_negative};
        }

        namespace detail {
            template <class UnsignedDecimalFp>
            struct unsigned_decimal_fp_to_signed;

            template <class UInt, bool trailing_zero_flag>
            struct unsigned_decimal_fp_to_signed<unsigned_decimal_fp<UInt, trailing_zero_flag>> {
                using type = signed_decimal_fp<UInt, trailing_zero_flag>;
            };

            template <class UnsignedDecimalFp>
            using unsigned_decimal_fp_to_signed_t =
                typename unsigned_decimal_fp_to_signed<UnsignedDecimalFp>::type;
        }


        ////////////////////////////////////////////////////////////////////////////////////////
        // Computed cache entries.
        ////////////////////////////////////////////////////////////////////////////////////////

        namespace detail {
            template <class FloatFormat, class Dummy = void>
            struct cache_holder;

            template <class Dummy>
            struct cache_holder<ieee754_binary32, Dummy> {
                using cache_entry_type = std::uint64_t;
                static constexpr int cache_bits = 64;
                static constexpr int min_k = -31;
                static constexpr int max_k = 46;
                static constexpr cache_entry_type cache[max_k - min_k + 1] = {
                    0x81ceb32c4b43fcf5, 0xa2425ff75e14fc32, 0xcad2f7f5359a3b3f, 0xfd87b5f28300ca0e,
                    0x9e74d1b791e07e49, 0xc612062576589ddb, 0xf79687aed3eec552, 0x9abe14cd44753b53,
                    0xc16d9a0095928a28, 0xf1c90080baf72cb2, 0x971da05074da7bef, 0xbce5086492111aeb,
                    0xec1e4a7db69561a6, 0x9392ee8e921d5d08, 0xb877aa3236a4b44a, 0xe69594bec44de15c,
                    0x901d7cf73ab0acda, 0xb424dc35095cd810, 0xe12e13424bb40e14, 0x8cbccc096f5088cc,
                    0xafebff0bcb24aaff, 0xdbe6fecebdedd5bf, 0x89705f4136b4a598, 0xabcc77118461cefd,
                    0xd6bf94d5e57a42bd, 0x8637bd05af6c69b6, 0xa7c5ac471b478424, 0xd1b71758e219652c,
                    0x83126e978d4fdf3c, 0xa3d70a3d70a3d70b, 0xcccccccccccccccd, 0x8000000000000000,
                    0xa000000000000000, 0xc800000000000000, 0xfa00000000000000, 0x9c40000000000000,
                    0xc350000000000000, 0xf424000000000000, 0x9896800000000000, 0xbebc200000000000,
                    0xee6b280000000000, 0x9502f90000000000, 0xba43b74000000000, 0xe8d4a51000000000,
                    0x9184e72a00000000, 0xb5e620f480000000, 0xe35fa931a0000000, 0x8e1bc9bf04000000,
                    0xb1a2bc2ec5000000, 0xde0b6b3a76400000, 0x8ac7230489e80000, 0xad78ebc5ac620000,
                    0xd8d726b7177a8000, 0x878678326eac9000, 0xa968163f0a57b400, 0xd3c21bcecceda100,
                    0x84595161401484a0, 0xa56fa5b99019a5c8, 0xcecb8f27f4200f3a, 0x813f3978f8940985,
                    0xa18f07d736b90be6, 0xc9f2c9cd04674edf, 0xfc6f7c4045812297, 0x9dc5ada82b70b59e,
                    0xc5371912364ce306, 0xf684df56c3e01bc7, 0x9a130b963a6c115d, 0xc097ce7bc90715b4,
                    0xf0bdc21abb48db21, 0x96769950b50d88f5, 0xbc143fa4e250eb32, 0xeb194f8e1ae525fe,
                    0x92efd1b8d0cf37bf, 0xb7abc627050305ae, 0xe596b7b0c643c71a, 0x8f7e32ce7bea5c70,
                    0xb35dbf821ae4f38c, 0xe0352f62a19e306f};
            };
#if !JKJ_HAS_INLINE_VARIABLE
            template <class Dummy>
            constexpr typename cache_holder<ieee754_binary32, Dummy>::cache_entry_type
                cache_holder<ieee754_binary32, Dummy>::cache[];
#endif

            template <class Dummy>
            struct cache_holder<ieee754_binary64, Dummy> {
                using cache_entry_type = wuint::uint128;
                static constexpr int cache_bits = 128;
                static constexpr int min_k = -292;
                static constexpr int max_k = 326;
                static constexpr cache_entry_type cache[max_k - min_k + 1] = {
                    {0xff77b1fcbebcdc4f, 0x25e8e89c13bb0f7b}, {0x9faacf3df73609b1, 0x77b191618c54e9ad},
                    {0xc795830d75038c1d, 0xd59df5b9ef6a2418}, {0xf97ae3d0d2446f25, 0x4b0573286b44ad1e},
                    {0x9becce62836ac577, 0x4ee367f9430aec33}, {0xc2e801fb244576d5, 0x229c41f793cda740},
                    {0xf3a20279ed56d48a, 0x6b43527578c11110}, {0x9845418c345644d6, 0x830a13896b78aaaa},
                    {0xbe5691ef416bd60c, 0x23cc986bc656d554}, {0xedec366b11c6cb8f, 0x2cbfbe86b7ec8aa9},
                    {0x94b3a202eb1c3f39, 0x7bf7d71432f3d6aa}, {0xb9e08a83a5e34f07, 0xdaf5ccd93fb0cc54},
                    {0xe858ad248f5c22c9, 0xd1b3400f8f9cff69}, {0x91376c36d99995be, 0x23100809b9c21fa2},
                    {0xb58547448ffffb2d, 0xabd40a0c2832a78b}, {0xe2e69915b3fff9f9, 0x16c90c8f323f516d},
                    {0x8dd01fad907ffc3b, 0xae3da7d97f6792e4}, {0xb1442798f49ffb4a, 0x99cd11cfdf41779d},
                    {0xdd95317f31c7fa1d, 0x40405643d711d584}, {0x8a7d3eef7f1cfc52, 0x482835ea666b2573},
                    {0xad1c8eab5ee43b66, 0xda3243650005eed0}, {0xd863b256369d4a40, 0x90bed43e40076a83},
                    {0x873e4f75e2224e68, 0x5a7744a6e804a292}, {0xa90de3535aaae202, 0x711515d0a205cb37},
                    {0xd3515c2831559a83, 0x0d5a5b44ca873e04}, {0x8412d9991ed58091, 0xe858790afe9486c3},
                    {0xa5178fff668ae0b6, 0x626e974dbe39a873}, {0xce5d73ff402d98e3, 0xfb0a3d212dc81290},
                    {0x80fa687f881c7f8e, 0x7ce66634bc9d0b9a}, {0xa139029f6a239f72, 0x1c1fffc1ebc44e81},
                    {0xc987434744ac874e, 0xa327ffb266b56221}, {0xfbe9141915d7a922, 0x4bf1ff9f0062baa9},
                    {0x9d71ac8fada6c9b5, 0x6f773fc3603db4aa}, {0xc4ce17b399107c22, 0xcb550fb4384d21d4},
                    {0xf6019da07f549b2b, 0x7e2a53a146606a49}, {0x99c102844f94e0fb, 0x2eda7444cbfc426e},
                    {0xc0314325637a1939, 0xfa911155fefb5309}, {0xf03d93eebc589f88, 0x793555ab7eba27cb},
                    {0x96267c7535b763b5, 0x4bc1558b2f3458df}, {0xbbb01b9283253ca2, 0x9eb1aaedfb016f17},
                    {0xea9c227723ee8bcb, 0x465e15a979c1cadd}, {0x92a1958a7675175f, 0x0bfacd89ec191eca},
                    {0xb749faed14125d36, 0xcef980ec671f667c}, {0xe51c79a85916f484, 0x82b7e12780e7401b},
                    {0x8f31cc0937ae58d2, 0xd1b2ecb8b0908811}, {0xb2fe3f0b8599ef07, 0x861fa7e6dcb4aa16},
                    {0xdfbdcece67006ac9, 0x67a791e093e1d49b}, {0x8bd6a141006042bd, 0xe0c8bb2c5c6d24e1},
                    {0xaecc49914078536d, 0x58fae9f773886e19}, {0xda7f5bf590966848, 0xaf39a475506a899f},
                    {0x888f99797a5e012d, 0x6d8406c952429604}, {0xaab37fd7d8f58178, 0xc8e5087ba6d33b84},
                    {0xd5605fcdcf32e1d6, 0xfb1e4a9a90880a65}, {0x855c3be0a17fcd26, 0x5cf2eea09a550680},
                    {0xa6b34ad8c9dfc06f, 0xf42faa48c0ea481f}, {0xd0601d8efc57b08b, 0xf13b94daf124da27},
                    {0x823c12795db6ce57, 0x76c53d08d6b70859}, {0xa2cb1717b52481ed, 0x54768c4b0c64ca6f},
                    {0xcb7ddcdda26da268, 0xa9942f5dcf7dfd0a}, {0xfe5d54150b090b02, 0xd3f93b35435d7c4d},
                    {0x9efa548d26e5a6e1, 0xc47bc5014a1a6db0}, {0xc6b8e9b0709f109a, 0x359ab6419ca1091c},
                    {0xf867241c8cc6d4c0, 0xc30163d203c94b63}, {0x9b407691d7fc44f8, 0x79e0de63425dcf1e},
                    {0xc21094364dfb5636, 0x985915fc12f542e5}, {0xf294b943e17a2bc4, 0x3e6f5b7b17b2939e},
                    {0x979cf3ca6cec5b5a, 0xa705992ceecf9c43}, {0xbd8430bd08277231, 0x50c6ff782a838354},
                    {0xece53cec4a314ebd, 0xa4f8bf5635246429}, {0x940f4613ae5ed136, 0x871b7795e136be9a},
                    {0xb913179899f68584, 0x28e2557b59846e40}, {0xe757dd7ec07426e5, 0x331aeada2fe589d0},
                    {0x9096ea6f3848984f, 0x3ff0d2c85def7622}, {0xb4bca50b065abe63, 0x0fed077a756b53aa},
                    {0xe1ebce4dc7f16dfb, 0xd3e8495912c62895}, {0x8d3360f09cf6e4bd, 0x64712dd7abbbd95d},
                    {0xb080392cc4349dec, 0xbd8d794d96aacfb4}, {0xdca04777f541c567, 0xecf0d7a0fc5583a1},
                    {0x89e42caaf9491b60, 0xf41686c49db57245}, {0xac5d37d5b79b6239, 0x311c2875c522ced6},
                    {0xd77485cb25823ac7, 0x7d633293366b828c}, {0x86a8d39ef77164bc, 0xae5dff9c02033198},
                    {0xa8530886b54dbdeb, 0xd9f57f830283fdfd}, {0xd267caa862a12d66, 0xd072df63c324fd7c},
                    {0x8380dea93da4bc60, 0x4247cb9e59f71e6e}, {0xa46116538d0deb78, 0x52d9be85f074e609},
                    {0xcd795be870516656, 0x67902e276c921f8c}, {0x806bd9714632dff6, 0x00ba1cd8a3db53b7},
                    {0xa086cfcd97bf97f3, 0x80e8a40eccd228a5}, {0xc8a883c0fdaf7df0, 0x6122cd128006b2ce},
                    {0xfad2a4b13d1b5d6c, 0x796b805720085f82}, {0x9cc3a6eec6311a63, 0xcbe3303674053bb1},
                    {0xc3f490aa77bd60fc, 0xbedbfc4411068a9d}, {0xf4f1b4d515acb93b, 0xee92fb5515482d45},
                    {0x991711052d8bf3c5, 0x751bdd152d4d1c4b}, {0xbf5cd54678eef0b6, 0xd262d45a78a0635e},
                    {0xef340a98172aace4, 0x86fb897116c87c35}, {0x9580869f0e7aac0e, 0xd45d35e6ae3d4da1},
                    {0xbae0a846d2195712, 0x8974836059cca10a}, {0xe998d258869facd7, 0x2bd1a438703fc94c},
                    {0x91ff83775423cc06, 0x7b6306a34627ddd0}, {0xb67f6455292cbf08, 0x1a3bc84c17b1d543},
                    {0xe41f3d6a7377eeca, 0x20caba5f1d9e4a94}, {0x8e938662882af53e, 0x547eb47b7282ee9d},
                    {0xb23867fb2a35b28d, 0xe99e619a4f23aa44}, {0xdec681f9f4c31f31, 0x6405fa00e2ec94d5},
                    {0x8b3c113c38f9f37e, 0xde83bc408dd3dd05}, {0xae0b158b4738705e, 0x9624ab50b148d446},
                    {0xd98ddaee19068c76, 0x3badd624dd9b0958}, {0x87f8a8d4cfa417c9, 0xe54ca5d70a80e5d7},
                    {0xa9f6d30a038d1dbc, 0x5e9fcf4ccd211f4d}, {0xd47487cc8470652b, 0x7647c32000696720},
                    {0x84c8d4dfd2c63f3b, 0x29ecd9f40041e074}, {0xa5fb0a17c777cf09, 0xf468107100525891},
                    {0xcf79cc9db955c2cc, 0x7182148d4066eeb5}, {0x81ac1fe293d599bf, 0xc6f14cd848405531},
                    {0xa21727db38cb002f, 0xb8ada00e5a506a7d}, {0xca9cf1d206fdc03b, 0xa6d90811f0e4851d},
                    {0xfd442e4688bd304a, 0x908f4a166d1da664}, {0x9e4a9cec15763e2e, 0x9a598e4e043287ff},
                    {0xc5dd44271ad3cdba, 0x40eff1e1853f29fe}, {0xf7549530e188c128, 0xd12bee59e68ef47d},
                    {0x9a94dd3e8cf578b9, 0x82bb74f8301958cf}, {0xc13a148e3032d6e7, 0xe36a52363c1faf02},
                    {0xf18899b1bc3f8ca1, 0xdc44e6c3cb279ac2}, {0x96f5600f15a7b7e5, 0x29ab103a5ef8c0ba},
                    {0xbcb2b812db11a5de, 0x7415d448f6b6f0e8}, {0xebdf661791d60f56, 0x111b495b3464ad22},
                    {0x936b9fcebb25c995, 0xcab10dd900beec35}, {0xb84687c269ef3bfb, 0x3d5d514f40eea743},
                    {0xe65829b3046b0afa, 0x0cb4a5a3112a5113}, {0x8ff71a0fe2c2e6dc, 0x47f0e785eaba72ac},
                    {0xb3f4e093db73a093, 0x59ed216765690f57}, {0xe0f218b8d25088b8, 0x306869c13ec3532d},
                    {0x8c974f7383725573, 0x1e414218c73a13fc}, {0xafbd2350644eeacf, 0xe5d1929ef90898fb},
                    {0xdbac6c247d62a583, 0xdf45f746b74abf3a}, {0x894bc396ce5da772, 0x6b8bba8c328eb784},
                    {0xab9eb47c81f5114f, 0x066ea92f3f326565}, {0xd686619ba27255a2, 0xc80a537b0efefebe},
                    {0x8613fd0145877585, 0xbd06742ce95f5f37}, {0xa798fc4196e952e7, 0x2c48113823b73705},
                    {0xd17f3b51fca3a7a0, 0xf75a15862ca504c6}, {0x82ef85133de648c4, 0x9a984d73dbe722fc},
                    {0xa3ab66580d5fdaf5, 0xc13e60d0d2e0ebbb}, {0xcc963fee10b7d1b3, 0x318df905079926a9},
                    {0xffbbcfe994e5c61f, 0xfdf17746497f7053}, {0x9fd561f1fd0f9bd3, 0xfeb6ea8bedefa634},
                    {0xc7caba6e7c5382c8, 0xfe64a52ee96b8fc1}, {0xf9bd690a1b68637b, 0x3dfdce7aa3c673b1},
                    {0x9c1661a651213e2d, 0x06bea10ca65c084f}, {0xc31bfa0fe5698db8, 0x486e494fcff30a63},
                    {0xf3e2f893dec3f126, 0x5a89dba3c3efccfb}, {0x986ddb5c6b3a76b7, 0xf89629465a75e01d},
                    {0xbe89523386091465, 0xf6bbb397f1135824}, {0xee2ba6c0678b597f, 0x746aa07ded582e2d},
                    {0x94db483840b717ef, 0xa8c2a44eb4571cdd}, {0xba121a4650e4ddeb, 0x92f34d62616ce414},
                    {0xe896a0d7e51e1566, 0x77b020baf9c81d18}, {0x915e2486ef32cd60, 0x0ace1474dc1d122f},
                    {0xb5b5ada8aaff80b8, 0x0d819992132456bb}, {0xe3231912d5bf60e6, 0x10e1fff697ed6c6a},
                    {0x8df5efabc5979c8f, 0xca8d3ffa1ef463c2}, {0xb1736b96b6fd83b3, 0xbd308ff8a6b17cb3},
                    {0xddd0467c64bce4a0, 0xac7cb3f6d05ddbdf}, {0x8aa22c0dbef60ee4, 0x6bcdf07a423aa96c},
                    {0xad4ab7112eb3929d, 0x86c16c98d2c953c7}, {0xd89d64d57a607744, 0xe871c7bf077ba8b8},
                    {0x87625f056c7c4a8b, 0x11471cd764ad4973}, {0xa93af6c6c79b5d2d, 0xd598e40d3dd89bd0},
                    {0xd389b47879823479, 0x4aff1d108d4ec2c4}, {0x843610cb4bf160cb, 0xcedf722a585139bb},
                    {0xa54394fe1eedb8fe, 0xc2974eb4ee658829}, {0xce947a3da6a9273e, 0x733d226229feea33},
                    {0x811ccc668829b887, 0x0806357d5a3f5260}, {0xa163ff802a3426a8, 0xca07c2dcb0cf26f8},
                    {0xc9bcff6034c13052, 0xfc89b393dd02f0b6}, {0xfc2c3f3841f17c67, 0xbbac2078d443ace3},
                    {0x9d9ba7832936edc0, 0xd54b944b84aa4c0e}, {0xc5029163f384a931, 0x0a9e795e65d4df12},
                    {0xf64335bcf065d37d, 0x4d4617b5ff4a16d6}, {0x99ea0196163fa42e, 0x504bced1bf8e4e46},
                    {0xc06481fb9bcf8d39, 0xe45ec2862f71e1d7}, {0xf07da27a82c37088, 0x5d767327bb4e5a4d},
                    {0x964e858c91ba2655, 0x3a6a07f8d510f870}, {0xbbe226efb628afea, 0x890489f70a55368c},
                    {0xeadab0aba3b2dbe5, 0x2b45ac74ccea842f}, {0x92c8ae6b464fc96f, 0x3b0b8bc90012929e},
                    {0xb77ada0617e3bbcb, 0x09ce6ebb40173745}, {0xe55990879ddcaabd, 0xcc420a6a101d0516},
                    {0x8f57fa54c2a9eab6, 0x9fa946824a12232e}, {0xb32df8e9f3546564, 0x47939822dc96abfa},
                    {0xdff9772470297ebd, 0x59787e2b93bc56f8}, {0x8bfbea76c619ef36, 0x57eb4edb3c55b65b},
                    {0xaefae51477a06b03, 0xede622920b6b23f2}, {0xdab99e59958885c4, 0xe95fab368e45ecee},
                    {0x88b402f7fd75539b, 0x11dbcb0218ebb415}, {0xaae103b5fcd2a881, 0xd652bdc29f26a11a},
                    {0xd59944a37c0752a2, 0x4be76d3346f04960}, {0x857fcae62d8493a5, 0x6f70a4400c562ddc},
                    {0xa6dfbd9fb8e5b88e, 0xcb4ccd500f6bb953}, {0xd097ad07a71f26b2, 0x7e2000a41346a7a8},
                    {0x825ecc24c873782f, 0x8ed400668c0c28c9}, {0xa2f67f2dfa90563b, 0x728900802f0f32fb},
                    {0xcbb41ef979346bca, 0x4f2b40a03ad2ffba}, {0xfea126b7d78186bc, 0xe2f610c84987bfa9},
                    {0x9f24b832e6b0f436, 0x0dd9ca7d2df4d7ca}, {0xc6ede63fa05d3143, 0x91503d1c79720dbc},
                    {0xf8a95fcf88747d94, 0x75a44c6397ce912b}, {0x9b69dbe1b548ce7c, 0xc986afbe3ee11abb},
                    {0xc24452da229b021b, 0xfbe85badce996169}, {0xf2d56790ab41c2a2, 0xfae27299423fb9c4},
                    {0x97c560ba6b0919a5, 0xdccd879fc967d41b}, {0xbdb6b8e905cb600f, 0x5400e987bbc1c921},
                    {0xed246723473e3813, 0x290123e9aab23b69}, {0x9436c0760c86e30b, 0xf9a0b6720aaf6522},
                    {0xb94470938fa89bce, 0xf808e40e8d5b3e6a}, {0xe7958cb87392c2c2, 0xb60b1d1230b20e05},
                    {0x90bd77f3483bb9b9, 0xb1c6f22b5e6f48c3}, {0xb4ecd5f01a4aa828, 0x1e38aeb6360b1af4},
                    {0xe2280b6c20dd5232, 0x25c6da63c38de1b1}, {0x8d590723948a535f, 0x579c487e5a38ad0f},
                    {0xb0af48ec79ace837, 0x2d835a9df0c6d852}, {0xdcdb1b2798182244, 0xf8e431456cf88e66},
                    {0x8a08f0f8bf0f156b, 0x1b8e9ecb641b5900}, {0xac8b2d36eed2dac5, 0xe272467e3d222f40},
                    {0xd7adf884aa879177, 0x5b0ed81dcc6abb10}, {0x86ccbb52ea94baea, 0x98e947129fc2b4ea},
                    {0xa87fea27a539e9a5, 0x3f2398d747b36225}, {0xd29fe4b18e88640e, 0x8eec7f0d19a03aae},
                    {0x83a3eeeef9153e89, 0x1953cf68300424ad}, {0xa48ceaaab75a8e2b, 0x5fa8c3423c052dd8},
                    {0xcdb02555653131b6, 0x3792f412cb06794e}, {0x808e17555f3ebf11, 0xe2bbd88bbee40bd1},
                    {0xa0b19d2ab70e6ed6, 0x5b6aceaeae9d0ec5}, {0xc8de047564d20a8b, 0xf245825a5a445276},
                    {0xfb158592be068d2e, 0xeed6e2f0f0d56713}, {0x9ced737bb6c4183d, 0x55464dd69685606c},
                    {0xc428d05aa4751e4c, 0xaa97e14c3c26b887}, {0xf53304714d9265df, 0xd53dd99f4b3066a9},
                    {0x993fe2c6d07b7fab, 0xe546a8038efe402a}, {0xbf8fdb78849a5f96, 0xde98520472bdd034},
                    {0xef73d256a5c0f77c, 0x963e66858f6d4441}, {0x95a8637627989aad, 0xdde7001379a44aa9},
                    {0xbb127c53b17ec159, 0x5560c018580d5d53}, {0xe9d71b689dde71af, 0xaab8f01e6e10b4a7},
                    {0x9226712162ab070d, 0xcab3961304ca70e9}, {0xb6b00d69bb55c8d1, 0x3d607b97c5fd0d23},
                    {0xe45c10c42a2b3b05, 0x8cb89a7db77c506b}, {0x8eb98a7a9a5b04e3, 0x77f3608e92adb243},
                    {0xb267ed1940f1c61c, 0x55f038b237591ed4}, {0xdf01e85f912e37a3, 0x6b6c46dec52f6689},
                    {0x8b61313bbabce2c6, 0x2323ac4b3b3da016}, {0xae397d8aa96c1b77, 0xabec975e0a0d081b},
                    {0xd9c7dced53c72255, 0x96e7bd358c904a22}, {0x881cea14545c7575, 0x7e50d64177da2e55},
                    {0xaa242499697392d2, 0xdde50bd1d5d0b9ea}, {0xd4ad2dbfc3d07787, 0x955e4ec64b44e865},
                    {0x84ec3c97da624ab4, 0xbd5af13bef0b113f}, {0xa6274bbdd0fadd61, 0xecb1ad8aeacdd58f},
                    {0xcfb11ead453994ba, 0x67de18eda5814af3}, {0x81ceb32c4b43fcf4, 0x80eacf948770ced8},
                    {0xa2425ff75e14fc31, 0xa1258379a94d028e}, {0xcad2f7f5359a3b3e, 0x096ee45813a04331},
                    {0xfd87b5f28300ca0d, 0x8bca9d6e188853fd}, {0x9e74d1b791e07e48, 0x775ea264cf55347e},
                    {0xc612062576589dda, 0x95364afe032a819e}, {0xf79687aed3eec551, 0x3a83ddbd83f52205},
                    {0x9abe14cd44753b52, 0xc4926a9672793543}, {0xc16d9a0095928a27, 0x75b7053c0f178294},
                    {0xf1c90080baf72cb1, 0x5324c68b12dd6339}, {0x971da05074da7bee, 0xd3f6fc16ebca5e04},
                    {0xbce5086492111aea, 0x88f4bb1ca6bcf585}, {0xec1e4a7db69561a5, 0x2b31e9e3d06c32e6},
                    {0x9392ee8e921d5d07, 0x3aff322e62439fd0}, {0xb877aa3236a4b449, 0x09befeb9fad487c3},
                    {0xe69594bec44de15b, 0x4c2ebe687989a9b4}, {0x901d7cf73ab0acd9, 0x0f9d37014bf60a11},
                    {0xb424dc35095cd80f, 0x538484c19ef38c95}, {0xe12e13424bb40e13, 0x2865a5f206b06fba},
                    {0x8cbccc096f5088cb, 0xf93f87b7442e45d4}, {0xafebff0bcb24aafe, 0xf78f69a51539d749},
                    {0xdbe6fecebdedd5be, 0xb573440e5a884d1c}, {0x89705f4136b4a597, 0x31680a88f8953031},
                    {0xabcc77118461cefc, 0xfdc20d2b36ba7c3e}, {0xd6bf94d5e57a42bc, 0x3d32907604691b4d},
                    {0x8637bd05af6c69b5, 0xa63f9a49c2c1b110}, {0xa7c5ac471b478423, 0x0fcf80dc33721d54},
                    {0xd1b71758e219652b, 0xd3c36113404ea4a9}, {0x83126e978d4fdf3b, 0x645a1cac083126ea},
                    {0xa3d70a3d70a3d70a, 0x3d70a3d70a3d70a4}, {0xcccccccccccccccc, 0xcccccccccccccccd},
                    {0x8000000000000000, 0x0000000000000000}, {0xa000000000000000, 0x0000000000000000},
                    {0xc800000000000000, 0x0000000000000000}, {0xfa00000000000000, 0x0000000000000000},
                    {0x9c40000000000000, 0x0000000000000000}, {0xc350000000000000, 0x0000000000000000},
                    {0xf424000000000000, 0x0000000000000000}, {0x9896800000000000, 0x0000000000000000},
                    {0xbebc200000000000, 0x0000000000000000}, {0xee6b280000000000, 0x0000000000000000},
                    {0x9502f90000000000, 0x0000000000000000}, {0xba43b74000000000, 0x0000000000000000},
                    {0xe8d4a51000000000, 0x0000000000000000}, {0x9184e72a00000000, 0x0000000000000000},
                    {0xb5e620f480000000, 0x0000000000000000}, {0xe35fa931a0000000, 0x0000000000000000},
                    {0x8e1bc9bf04000000, 0x0000000000000000}, {0xb1a2bc2ec5000000, 0x0000000000000000},
                    {0xde0b6b3a76400000, 0x0000000000000000}, {0x8ac7230489e80000, 0x0000000000000000},
                    {0xad78ebc5ac620000, 0x0000000000000000}, {0xd8d726b7177a8000, 0x0000000000000000},
                    {0x878678326eac9000, 0x0000000000000000}, {0xa968163f0a57b400, 0x0000000000000000},
                    {0xd3c21bcecceda100, 0x0000000000000000}, {0x84595161401484a0, 0x0000000000000000},
                    {0xa56fa5b99019a5c8, 0x0000000000000000}, {0xcecb8f27f4200f3a, 0x0000000000000000},
                    {0x813f3978f8940984, 0x4000000000000000}, {0xa18f07d736b90be5, 0x5000000000000000},
                    {0xc9f2c9cd04674ede, 0xa400000000000000}, {0xfc6f7c4045812296, 0x4d00000000000000},
                    {0x9dc5ada82b70b59d, 0xf020000000000000}, {0xc5371912364ce305, 0x6c28000000000000},
                    {0xf684df56c3e01bc6, 0xc732000000000000}, {0x9a130b963a6c115c, 0x3c7f400000000000},
                    {0xc097ce7bc90715b3, 0x4b9f100000000000}, {0xf0bdc21abb48db20, 0x1e86d40000000000},
                    {0x96769950b50d88f4, 0x1314448000000000}, {0xbc143fa4e250eb31, 0x17d955a000000000},
                    {0xeb194f8e1ae525fd, 0x5dcfab0800000000}, {0x92efd1b8d0cf37be, 0x5aa1cae500000000},
                    {0xb7abc627050305ad, 0xf14a3d9e40000000}, {0xe596b7b0c643c719, 0x6d9ccd05d0000000},
                    {0x8f7e32ce7bea5c6f, 0xe4820023a2000000}, {0xb35dbf821ae4f38b, 0xdda2802c8a800000},
                    {0xe0352f62a19e306e, 0xd50b2037ad200000}, {0x8c213d9da502de45, 0x4526f422cc340000},
                    {0xaf298d050e4395d6, 0x9670b12b7f410000}, {0xdaf3f04651d47b4c, 0x3c0cdd765f114000},
                    {0x88d8762bf324cd0f, 0xa5880a69fb6ac800}, {0xab0e93b6efee0053, 0x8eea0d047a457a00},
                    {0xd5d238a4abe98068, 0x72a4904598d6d880}, {0x85a36366eb71f041, 0x47a6da2b7f864750},
                    {0xa70c3c40a64e6c51, 0x999090b65f67d924}, {0xd0cf4b50cfe20765, 0xfff4b4e3f741cf6d},
                    {0x82818f1281ed449f, 0xbff8f10e7a8921a5}, {0xa321f2d7226895c7, 0xaff72d52192b6a0e},
                    {0xcbea6f8ceb02bb39, 0x9bf4f8a69f764491}, {0xfee50b7025c36a08, 0x02f236d04753d5b5},
                    {0x9f4f2726179a2245, 0x01d762422c946591}, {0xc722f0ef9d80aad6, 0x424d3ad2b7b97ef6},
                    {0xf8ebad2b84e0d58b, 0xd2e0898765a7deb3}, {0x9b934c3b330c8577, 0x63cc55f49f88eb30},
                    {0xc2781f49ffcfa6d5, 0x3cbf6b71c76b25fc}, {0xf316271c7fc3908a, 0x8bef464e3945ef7b},
                    {0x97edd871cfda3a56, 0x97758bf0e3cbb5ad}, {0xbde94e8e43d0c8ec, 0x3d52eeed1cbea318},
                    {0xed63a231d4c4fb27, 0x4ca7aaa863ee4bde}, {0x945e455f24fb1cf8, 0x8fe8caa93e74ef6b},
                    {0xb975d6b6ee39e436, 0xb3e2fd538e122b45}, {0xe7d34c64a9c85d44, 0x60dbbca87196b617},
                    {0x90e40fbeea1d3a4a, 0xbc8955e946fe31ce}, {0xb51d13aea4a488dd, 0x6babab6398bdbe42},
                    {0xe264589a4dcdab14, 0xc696963c7eed2dd2}, {0x8d7eb76070a08aec, 0xfc1e1de5cf543ca3},
                    {0xb0de65388cc8ada8, 0x3b25a55f43294bcc}, {0xdd15fe86affad912, 0x49ef0eb713f39ebf},
                    {0x8a2dbf142dfcc7ab, 0x6e3569326c784338}, {0xacb92ed9397bf996, 0x49c2c37f07965405},
                    {0xd7e77a8f87daf7fb, 0xdc33745ec97be907}, {0x86f0ac99b4e8dafd, 0x69a028bb3ded71a4},
                    {0xa8acd7c0222311bc, 0xc40832ea0d68ce0d}, {0xd2d80db02aabd62b, 0xf50a3fa490c30191},
                    {0x83c7088e1aab65db, 0x792667c6da79e0fb}, {0xa4b8cab1a1563f52, 0x577001b891185939},
                    {0xcde6fd5e09abcf26, 0xed4c0226b55e6f87}, {0x80b05e5ac60b6178, 0x544f8158315b05b5},
                    {0xa0dc75f1778e39d6, 0x696361ae3db1c722}, {0xc913936dd571c84c, 0x03bc3a19cd1e38ea},
                    {0xfb5878494ace3a5f, 0x04ab48a04065c724}, {0x9d174b2dcec0e47b, 0x62eb0d64283f9c77},
                    {0xc45d1df942711d9a, 0x3ba5d0bd324f8395}, {0xf5746577930d6500, 0xca8f44ec7ee3647a},
                    {0x9968bf6abbe85f20, 0x7e998b13cf4e1ecc}, {0xbfc2ef456ae276e8, 0x9e3fedd8c321a67f},
                    {0xefb3ab16c59b14a2, 0xc5cfe94ef3ea101f}, {0x95d04aee3b80ece5, 0xbba1f1d158724a13},
                    {0xbb445da9ca61281f, 0x2a8a6e45ae8edc98}, {0xea1575143cf97226, 0xf52d09d71a3293be},
                    {0x924d692ca61be758, 0x593c2626705f9c57}, {0xb6e0c377cfa2e12e, 0x6f8b2fb00c77836d},
                    {0xe498f455c38b997a, 0x0b6dfb9c0f956448}, {0x8edf98b59a373fec, 0x4724bd4189bd5ead},
                    {0xb2977ee300c50fe7, 0x58edec91ec2cb658}, {0xdf3d5e9bc0f653e1, 0x2f2967b66737e3ee},
                    {0x8b865b215899f46c, 0xbd79e0d20082ee75}, {0xae67f1e9aec07187, 0xecd8590680a3aa12},
                    {0xda01ee641a708de9, 0xe80e6f4820cc9496}, {0x884134fe908658b2, 0x3109058d147fdcde},
                    {0xaa51823e34a7eede, 0xbd4b46f0599fd416}, {0xd4e5e2cdc1d1ea96, 0x6c9e18ac7007c91b},
                    {0x850fadc09923329e, 0x03e2cf6bc604ddb1}, {0xa6539930bf6bff45, 0x84db8346b786151d},
                    {0xcfe87f7cef46ff16, 0xe612641865679a64}, {0x81f14fae158c5f6e, 0x4fcb7e8f3f60c07f},
                    {0xa26da3999aef7749, 0xe3be5e330f38f09e}, {0xcb090c8001ab551c, 0x5cadf5bfd3072cc6},
                    {0xfdcb4fa002162a63, 0x73d9732fc7c8f7f7}, {0x9e9f11c4014dda7e, 0x2867e7fddcdd9afb},
                    {0xc646d63501a1511d, 0xb281e1fd541501b9}, {0xf7d88bc24209a565, 0x1f225a7ca91a4227},
                    {0x9ae757596946075f, 0x3375788de9b06959}, {0xc1a12d2fc3978937, 0x0052d6b1641c83af},
                    {0xf209787bb47d6b84, 0xc0678c5dbd23a49b}, {0x9745eb4d50ce6332, 0xf840b7ba963646e1},
                    {0xbd176620a501fbff, 0xb650e5a93bc3d899}, {0xec5d3fa8ce427aff, 0xa3e51f138ab4cebf},
                    {0x93ba47c980e98cdf, 0xc66f336c36b10138}, {0xb8a8d9bbe123f017, 0xb80b0047445d4185},
                    {0xe6d3102ad96cec1d, 0xa60dc059157491e6}, {0x9043ea1ac7e41392, 0x87c89837ad68db30},
                    {0xb454e4a179dd1877, 0x29babe4598c311fc}, {0xe16a1dc9d8545e94, 0xf4296dd6fef3d67b},
                    {0x8ce2529e2734bb1d, 0x1899e4a65f58660d}, {0xb01ae745b101e9e4, 0x5ec05dcff72e7f90},
                    {0xdc21a1171d42645d, 0x76707543f4fa1f74}, {0x899504ae72497eba, 0x6a06494a791c53a9},
                    {0xabfa45da0edbde69, 0x0487db9d17636893}, {0xd6f8d7509292d603, 0x45a9d2845d3c42b7},
                    {0x865b86925b9bc5c2, 0x0b8a2392ba45a9b3}, {0xa7f26836f282b732, 0x8e6cac7768d7141f},
                    {0xd1ef0244af2364ff, 0x3207d795430cd927}, {0x8335616aed761f1f, 0x7f44e6bd49e807b9},
                    {0xa402b9c5a8d3a6e7, 0x5f16206c9c6209a7}, {0xcd036837130890a1, 0x36dba887c37a8c10},
                    {0x802221226be55a64, 0xc2494954da2c978a}, {0xa02aa96b06deb0fd, 0xf2db9baa10b7bd6d},
                    {0xc83553c5c8965d3d, 0x6f92829494e5acc8}, {0xfa42a8b73abbf48c, 0xcb772339ba1f17fa},
                    {0x9c69a97284b578d7, 0xff2a760414536efc}, {0xc38413cf25e2d70d, 0xfef5138519684abb},
                    {0xf46518c2ef5b8cd1, 0x7eb258665fc25d6a}, {0x98bf2f79d5993802, 0xef2f773ffbd97a62},
                    {0xbeeefb584aff8603, 0xaafb550ffacfd8fb}, {0xeeaaba2e5dbf6784, 0x95ba2a53f983cf39},
                    {0x952ab45cfa97a0b2, 0xdd945a747bf26184}, {0xba756174393d88df, 0x94f971119aeef9e5},
                    {0xe912b9d1478ceb17, 0x7a37cd5601aab85e}, {0x91abb422ccb812ee, 0xac62e055c10ab33b},
                    {0xb616a12b7fe617aa, 0x577b986b314d600a}, {0xe39c49765fdf9d94, 0xed5a7e85fda0b80c},
                    {0x8e41ade9fbebc27d, 0x14588f13be847308}, {0xb1d219647ae6b31c, 0x596eb2d8ae258fc9},
                    {0xde469fbd99a05fe3, 0x6fca5f8ed9aef3bc}, {0x8aec23d680043bee, 0x25de7bb9480d5855},
                    {0xada72ccc20054ae9, 0xaf561aa79a10ae6b}, {0xd910f7ff28069da4, 0x1b2ba1518094da05},
                    {0x87aa9aff79042286, 0x90fb44d2f05d0843}, {0xa99541bf57452b28, 0x353a1607ac744a54},
                    {0xd3fa922f2d1675f2, 0x42889b8997915ce9}, {0x847c9b5d7c2e09b7, 0x69956135febada12},
                    {0xa59bc234db398c25, 0x43fab9837e699096}, {0xcf02b2c21207ef2e, 0x94f967e45e03f4bc},
                    {0x8161afb94b44f57d, 0x1d1be0eebac278f6}, {0xa1ba1ba79e1632dc, 0x6462d92a69731733},
                    {0xca28a291859bbf93, 0x7d7b8f7503cfdcff}, {0xfcb2cb35e702af78, 0x5cda735244c3d43f},
                    {0x9defbf01b061adab, 0x3a0888136afa64a8}, {0xc56baec21c7a1916, 0x088aaa1845b8fdd1},
                    {0xf6c69a72a3989f5b, 0x8aad549e57273d46}, {0x9a3c2087a63f6399, 0x36ac54e2f678864c},
                    {0xc0cb28a98fcf3c7f, 0x84576a1bb416a7de}, {0xf0fdf2d3f3c30b9f, 0x656d44a2a11c51d6},
                    {0x969eb7c47859e743, 0x9f644ae5a4b1b326}, {0xbc4665b596706114, 0x873d5d9f0dde1fef},
                    {0xeb57ff22fc0c7959, 0xa90cb506d155a7eb}, {0x9316ff75dd87cbd8, 0x09a7f12442d588f3},
                    {0xb7dcbf5354e9bece, 0x0c11ed6d538aeb30}, {0xe5d3ef282a242e81, 0x8f1668c8a86da5fb},
                    {0x8fa475791a569d10, 0xf96e017d694487bd}, {0xb38d92d760ec4455, 0x37c981dcc395a9ad},
                    {0xe070f78d3927556a, 0x85bbe253f47b1418}, {0x8c469ab843b89562, 0x93956d7478ccec8f},
                    {0xaf58416654a6babb, 0x387ac8d1970027b3}, {0xdb2e51bfe9d0696a, 0x06997b05fcc0319f},
                    {0x88fcf317f22241e2, 0x441fece3bdf81f04}, {0xab3c2fddeeaad25a, 0xd527e81cad7626c4},
                    {0xd60b3bd56a5586f1, 0x8a71e223d8d3b075}, {0x85c7056562757456, 0xf6872d5667844e4a},
                    {0xa738c6bebb12d16c, 0xb428f8ac016561dc}, {0xd106f86e69d785c7, 0xe13336d701beba53},
                    {0x82a45b450226b39c, 0xecc0024661173474}, {0xa34d721642b06084, 0x27f002d7f95d0191},
                    {0xcc20ce9bd35c78a5, 0x31ec038df7b441f5}, {0xff290242c83396ce, 0x7e67047175a15272},
                    {0x9f79a169bd203e41, 0x0f0062c6e984d387}, {0xc75809c42c684dd1, 0x52c07b78a3e60869},
                    {0xf92e0c3537826145, 0xa7709a56ccdf8a83}, {0x9bbcc7a142b17ccb, 0x88a66076400bb692},
                    {0xc2abf989935ddbfe, 0x6acff893d00ea436}, {0xf356f7ebf83552fe, 0x0583f6b8c4124d44},
                    {0x98165af37b2153de, 0xc3727a337a8b704b}, {0xbe1bf1b059e9a8d6, 0x744f18c0592e4c5d},
                    {0xeda2ee1c7064130c, 0x1162def06f79df74}, {0x9485d4d1c63e8be7, 0x8addcb5645ac2ba9},
                    {0xb9a74a0637ce2ee1, 0x6d953e2bd7173693}, {0xe8111c87c5c1ba99, 0xc8fa8db6ccdd0438},
                    {0x910ab1d4db9914a0, 0x1d9c9892400a22a3}, {0xb54d5e4a127f59c8, 0x2503beb6d00cab4c},
                    {0xe2a0b5dc971f303a, 0x2e44ae64840fd61e}, {0x8da471a9de737e24, 0x5ceaecfed289e5d3},
                    {0xb10d8e1456105dad, 0x7425a83e872c5f48}, {0xdd50f1996b947518, 0xd12f124e28f7771a},
                    {0x8a5296ffe33cc92f, 0x82bd6b70d99aaa70}, {0xace73cbfdc0bfb7b, 0x636cc64d1001550c},
                    {0xd8210befd30efa5a, 0x3c47f7e05401aa4f}, {0x8714a775e3e95c78, 0x65acfaec34810a72},
                    {0xa8d9d1535ce3b396, 0x7f1839a741a14d0e}, {0xd31045a8341ca07c, 0x1ede48111209a051},
                    {0x83ea2b892091e44d, 0x934aed0aab460433}, {0xa4e4b66b68b65d60, 0xf81da84d56178540},
                    {0xce1de40642e3f4b9, 0x36251260ab9d668f}, {0x80d2ae83e9ce78f3, 0xc1d72b7c6b42601a},
                    {0xa1075a24e4421730, 0xb24cf65b8612f820}, {0xc94930ae1d529cfc, 0xdee033f26797b628},
                    {0xfb9b7cd9a4a7443c, 0x169840ef017da3b2}, {0x9d412e0806e88aa5, 0x8e1f289560ee864f},
                    {0xc491798a08a2ad4e, 0xf1a6f2bab92a27e3}, {0xf5b5d7ec8acb58a2, 0xae10af696774b1dc},
                    {0x9991a6f3d6bf1765, 0xacca6da1e0a8ef2a}, {0xbff610b0cc6edd3f, 0x17fd090a58d32af4},
                    {0xeff394dcff8a948e, 0xddfc4b4cef07f5b1}, {0x95f83d0a1fb69cd9, 0x4abdaf101564f98f},
                    {0xbb764c4ca7a4440f, 0x9d6d1ad41abe37f2}, {0xea53df5fd18d5513, 0x84c86189216dc5ee},
                    {0x92746b9be2f8552c, 0x32fd3cf5b4e49bb5}, {0xb7118682dbb66a77, 0x3fbc8c33221dc2a2},
                    {0xe4d5e82392a40515, 0x0fabaf3feaa5334b}, {0x8f05b1163ba6832d, 0x29cb4d87f2a7400f},
                    {0xb2c71d5bca9023f8, 0x743e20e9ef511013}, {0xdf78e4b2bd342cf6, 0x914da9246b255417},
                    {0x8bab8eefb6409c1a, 0x1ad089b6c2f7548f}, {0xae9672aba3d0c320, 0xa184ac2473b529b2},
                    {0xda3c0f568cc4f3e8, 0xc9e5d72d90a2741f}, {0x8865899617fb1871, 0x7e2fa67c7a658893},
                    {0xaa7eebfb9df9de8d, 0xddbb901b98feeab8}, {0xd51ea6fa85785631, 0x552a74227f3ea566},
                    {0x8533285c936b35de, 0xd53a88958f872760}, {0xa67ff273b8460356, 0x8a892abaf368f138},
                    {0xd01fef10a657842c, 0x2d2b7569b0432d86}, {0x8213f56a67f6b29b, 0x9c3b29620e29fc74},
                    {0xa298f2c501f45f42, 0x8349f3ba91b47b90}, {0xcb3f2f7642717713, 0x241c70a936219a74},
                    {0xfe0efb53d30dd4d7, 0xed238cd383aa0111}, {0x9ec95d1463e8a506, 0xf4363804324a40ab},
                    {0xc67bb4597ce2ce48, 0xb143c6053edcd0d6}, {0xf81aa16fdc1b81da, 0xdd94b7868e94050b},
                    {0x9b10a4e5e9913128, 0xca7cf2b4191c8327}, {0xc1d4ce1f63f57d72, 0xfd1c2f611f63a3f1},
                    {0xf24a01a73cf2dccf, 0xbc633b39673c8ced}, {0x976e41088617ca01, 0xd5be0503e085d814},
                    {0xbd49d14aa79dbc82, 0x4b2d8644d8a74e19}, {0xec9c459d51852ba2, 0xddf8e7d60ed1219f},
                    {0x93e1ab8252f33b45, 0xcabb90e5c942b504}, {0xb8da1662e7b00a17, 0x3d6a751f3b936244},
                    {0xe7109bfba19c0c9d, 0x0cc512670a783ad5}, {0x906a617d450187e2, 0x27fb2b80668b24c6},
                    {0xb484f9dc9641e9da, 0xb1f9f660802dedf7}, {0xe1a63853bbd26451, 0x5e7873f8a0396974},
                    {0x8d07e33455637eb2, 0xdb0b487b6423e1e9}, {0xb049dc016abc5e5f, 0x91ce1a9a3d2cda63},
                    {0xdc5c5301c56b75f7, 0x7641a140cc7810fc}, {0x89b9b3e11b6329ba, 0xa9e904c87fcb0a9e},
                    {0xac2820d9623bf429, 0x546345fa9fbdcd45}, {0xd732290fbacaf133, 0xa97c177947ad4096},
                    {0x867f59a9d4bed6c0, 0x49ed8eabcccc485e}, {0xa81f301449ee8c70, 0x5c68f256bfff5a75},
                    {0xd226fc195c6a2f8c, 0x73832eec6fff3112}, {0x83585d8fd9c25db7, 0xc831fd53c5ff7eac},
                    {0xa42e74f3d032f525, 0xba3e7ca8b77f5e56}, {0xcd3a1230c43fb26f, 0x28ce1bd2e55f35ec},
                    {0x80444b5e7aa7cf85, 0x7980d163cf5b81b4}, {0xa0555e361951c366, 0xd7e105bcc3326220},
                    {0xc86ab5c39fa63440, 0x8dd9472bf3fefaa8}, {0xfa856334878fc150, 0xb14f98f6f0feb952},
                    {0x9c935e00d4b9d8d2, 0x6ed1bf9a569f33d4}, {0xc3b8358109e84f07, 0x0a862f80ec4700c9},
                    {0xf4a642e14c6262c8, 0xcd27bb612758c0fb}, {0x98e7e9cccfbd7dbd, 0x8038d51cb897789d},
                    {0xbf21e44003acdd2c, 0xe0470a63e6bd56c4}, {0xeeea5d5004981478, 0x1858ccfce06cac75},
                    {0x95527a5202df0ccb, 0x0f37801e0c43ebc9}, {0xbaa718e68396cffd, 0xd30560258f54e6bb},
                    {0xe950df20247c83fd, 0x47c6b82ef32a206a}, {0x91d28b7416cdd27e, 0x4cdc331d57fa5442},
                    {0xb6472e511c81471d, 0xe0133fe4adf8e953}, {0xe3d8f9e563a198e5, 0x58180fddd97723a7},
                    {0x8e679c2f5e44ff8f, 0x570f09eaa7ea7649}, {0xb201833b35d63f73, 0x2cd2cc6551e513db},
                    {0xde81e40a034bcf4f, 0xf8077f7ea65e58d2}, {0x8b112e86420f6191, 0xfb04afaf27faf783},
                    {0xadd57a27d29339f6, 0x79c5db9af1f9b564}, {0xd94ad8b1c7380874, 0x18375281ae7822bd},
                    {0x87cec76f1c830548, 0x8f2293910d0b15b6}, {0xa9c2794ae3a3c69a, 0xb2eb3875504ddb23},
                    {0xd433179d9c8cb841, 0x5fa60692a46151ec}, {0x849feec281d7f328, 0xdbc7c41ba6bcd334},
                    {0xa5c7ea73224deff3, 0x12b9b522906c0801}, {0xcf39e50feae16bef, 0xd768226b34870a01},
                    {0x81842f29f2cce375, 0xe6a1158300d46641}, {0xa1e53af46f801c53, 0x60495ae3c1097fd1},
                    {0xca5e89b18b602368, 0x385bb19cb14bdfc5}, {0xfcf62c1dee382c42, 0x46729e03dd9ed7b6},
                    {0x9e19db92b4e31ba9, 0x6c07a2c26a8346d2}, {0xc5a05277621be293, 0xc7098b7305241886},
                    {0xf70867153aa2db38, 0xb8cbee4fc66d1ea8}};
            };
#if !JKJ_HAS_INLINE_VARIABLE
            template <class Dummy>
            constexpr typename cache_holder<ieee754_binary64, Dummy>::cache_entry_type
                cache_holder<ieee754_binary64, Dummy>::cache[];
#endif

            // Compressed cache for double
            template <class Dummy = void>
            struct compressed_cache_detail {
                static constexpr int compression_ratio = 27;
                static constexpr std::size_t compressed_table_size =
                    (cache_holder<ieee754_binary64>::max_k - cache_holder<ieee754_binary64>::min_k +
                     compression_ratio) /
                    compression_ratio;

                struct cache_holder_t {
                    wuint::uint128 table[compressed_table_size];
                };
                struct pow5_holder_t {
                    std::uint64_t table[compression_ratio];
                };

#if JKJ_HAS_CONSTEXPR17
                static constexpr cache_holder_t cache = [] {
                    cache_holder_t res{};
                    for (std::size_t i = 0; i < compressed_table_size; ++i) {
                        res.table[i] = cache_holder<ieee754_binary64>::cache[i * compression_ratio];
                    }
                    return res;
                }();
                static constexpr pow5_holder_t pow5 = [] {
                    pow5_holder_t res{};
                    std::uint64_t p = 1;
                    for (std::size_t i = 0; i < compression_ratio; ++i) {
                        res.table[i] = p;
                        p *= 5;
                    }
                    return res;
                }();
#else
                template <std::size_t... indices>
                static constexpr cache_holder_t make_cache_table(index_sequence<indices...>) {
                    return {cache_holder<ieee754_binary64>::cache[indices * compression_ratio]...};
                }
                static constexpr cache_holder_t cache =
                    make_cache_table(make_index_sequence<compressed_table_size>{});

                template <std::size_t... indices>
                static constexpr pow5_holder_t make_pow5_table(index_sequence<indices...>) {
                    return {compute_power<indices>(std::uint64_t(5))...};
                }
                static constexpr pow5_holder_t pow5 =
                    make_pow5_table(make_index_sequence<compression_ratio>{});
#endif
            };
#if !JKJ_HAS_INLINE_VARIABLE
            template <class Dummy>
            constexpr typename compressed_cache_detail<Dummy>::cache_holder_t
                compressed_cache_detail<Dummy>::cache;
            template <class Dummy>
            constexpr typename compressed_cache_detail<Dummy>::pow5_holder_t
                compressed_cache_detail<Dummy>::pow5;
#endif
        }


        ////////////////////////////////////////////////////////////////////////////////////////
        // Policies.
        ////////////////////////////////////////////////////////////////////////////////////////

        namespace detail {
            // Forward declare the implementation class.
            template <class Float, class FloatTraits = default_float_traits<Float>>
            struct impl;

            namespace policy_impl {
                // Sign policies.
                namespace sign {
                    struct base {};

                    struct ignore : base {
                        using sign_policy = ignore;
                        static constexpr bool return_has_sign = false;

                        template <class SignedSignificandBits, class UnsignedDecimalFp>
                        static constexpr UnsignedDecimalFp handle_sign(SignedSignificandBits,
                                                                       UnsignedDecimalFp r) noexcept {
                            return r;
                        }
                    };

                    struct return_sign : base {
                        using sign_policy = return_sign;
                        static constexpr bool return_has_sign = true;

                        template <class SignedSignificandBits, class UnsignedDecimalFp>
                        static constexpr unsigned_decimal_fp_to_signed_t<UnsignedDecimalFp>
                        handle_sign(SignedSignificandBits s, UnsignedDecimalFp r) noexcept {
                            return add_sign_to_unsigned_decimal_fp(s.is_negative(), r);
                        }
                    };
                }

                // Trailing zero policies.
                namespace trailing_zero {
                    struct base {};

                    struct ignore : base {
                        using trailing_zero_policy = ignore;
                        static constexpr bool report_trailing_zeros = false;

                        template <class Impl, class ReturnType>
                        static constexpr ReturnType
                        on_trailing_zeros(typename Impl::carrier_uint significand,
                                          int exponent) noexcept {
                            return {significand, exponent};
                        }

                        template <class Impl, class ReturnType>
                        static constexpr ReturnType
                        no_trailing_zeros(typename Impl::carrier_uint significand,
                                          int exponent) noexcept {
                            return {significand, exponent};
                        }
                    };

                    struct remove : base {
                        using trailing_zero_policy = remove;
                        static constexpr bool report_trailing_zeros = false;

                        template <class Impl, class ReturnType>
                        JKJ_FORCEINLINE static constexpr ReturnType
                        on_trailing_zeros(typename Impl::carrier_uint significand,
                                          int exponent) noexcept {
                            return {significand, exponent + Impl::remove_trailing_zeros(significand)};
                        }

                        template <class Impl, class ReturnType>
                        static constexpr ReturnType
                        no_trailing_zeros(typename Impl::carrier_uint significand,
                                          int exponent) noexcept {
                            return {significand, exponent};
                        }
                    };

                    struct report : base {
                        using trailing_zero_policy = report;
                        static constexpr bool report_trailing_zeros = true;

                        template <class Impl, class ReturnType>
                        static constexpr ReturnType
                        on_trailing_zeros(typename Impl::carrier_uint significand,
                                          int exponent) noexcept {
                            return {significand, exponent, true};
                        }

                        template <class Impl, class ReturnType>
                        static constexpr ReturnType
                        no_trailing_zeros(typename Impl::carrier_uint significand,
                                          int exponent) noexcept {
                            return {significand, exponent, false};
                        }
                    };
                }

                // Decimal-to-binary rounding mode policies.
                namespace decimal_to_binary_rounding {
                    struct base {};

                    enum class tag_t { to_nearest, left_closed_directed, right_closed_directed };
                    namespace interval_type {
                        struct symmetric_boundary {
                            static constexpr bool is_symmetric = true;
                            bool is_closed;
                            constexpr bool include_left_endpoint() const noexcept { return is_closed; }
                            constexpr bool include_right_endpoint() const noexcept { return is_closed; }
                        };
                        struct asymmetric_boundary {
                            static constexpr bool is_symmetric = false;
                            bool is_left_closed;
                            constexpr bool include_left_endpoint() const noexcept {
                                return is_left_closed;
                            }
                            constexpr bool include_right_endpoint() const noexcept {
                                return !is_left_closed;
                            }
                        };
                        struct closed {
                            static constexpr bool is_symmetric = true;
                            static constexpr bool include_left_endpoint() noexcept { return true; }
                            static constexpr bool include_right_endpoint() noexcept { return true; }
                        };
                        struct open {
                            static constexpr bool is_symmetric = true;
                            static constexpr bool include_left_endpoint() noexcept { return false; }
                            static constexpr bool include_right_endpoint() noexcept { return false; }
                        };
                        struct left_closed_right_open {
                            static constexpr bool is_symmetric = false;
                            static constexpr bool include_left_endpoint() noexcept { return true; }
                            static constexpr bool include_right_endpoint() noexcept { return false; }
                        };
                        struct right_closed_left_open {
                            static constexpr bool is_symmetric = false;
                            static constexpr bool include_left_endpoint() noexcept { return false; }
                            static constexpr bool include_right_endpoint() noexcept { return true; }
                        };
                    }

                    struct nearest_to_even : base {
                        using decimal_to_binary_rounding_policy = nearest_to_even;
                        static constexpr auto tag = tag_t::to_nearest;
                        using normal_interval_type = interval_type::symmetric_boundary;
                        using shorter_interval_type = interval_type::closed;

                        template <class SignedSignificandBits, class Func, class... Args>
                        JKJ_FORCEINLINE
                            JKJ_SAFEBUFFERS static constexpr decltype(Func{}(declval<nearest_to_even>(),
                                                                             Args{}...))
                            delegate(SignedSignificandBits, Func f, Args... args) noexcept {
                            return f(nearest_to_even{}, args...);
                        }

                        template <class SignedSignificandBits, class Func, class... Args>
                        JKJ_FORCEINLINE JKJ_SAFEBUFFERS static constexpr decltype(Func{}(Args{}...,
                                                                                         false))
                        invoke_normal_interval_case(SignedSignificandBits s, Func f,
                                                    Args... args) noexcept {
                            return f(args..., s.has_even_significand_bits());
                        }
                        template <class SignedSignificandBits, class Func, class... Args>
                        JKJ_FORCEINLINE JKJ_SAFEBUFFERS static constexpr decltype(Func{}(Args{}...))
                        invoke_shorter_interval_case(SignedSignificandBits, Func f,
                                                     Args... args) noexcept {
                            return f(args...);
                        }
                    };
                    struct nearest_to_odd : base {
                        using decimal_to_binary_rounding_policy = nearest_to_odd;
                        static constexpr auto tag = tag_t::to_nearest;
                        using normal_interval_type = interval_type::symmetric_boundary;
                        using shorter_interval_type = interval_type::open;

                        template <class SignedSignificandBits, class Func, class... Args>
                        JKJ_FORCEINLINE
                            JKJ_SAFEBUFFERS static constexpr decltype(Func{}(declval<nearest_to_odd>(),
                                                                             Args{}...))
                            delegate(SignedSignificandBits, Func f, Args... args) noexcept {
                            return f(nearest_to_odd{}, args...);
                        }

                        template <class SignedSignificandBits, class Func, class... Args>
                        JKJ_FORCEINLINE JKJ_SAFEBUFFERS static constexpr decltype(Func{}(Args{}...,
                                                                                         false))
                        invoke_normal_interval_case(SignedSignificandBits s, Func f,
                                                    Args... args) noexcept {
                            return f(args..., !s.has_even_significand_bits());
                        }
                        template <class SignedSignificandBits, class Func, class... Args>
                        JKJ_FORCEINLINE JKJ_SAFEBUFFERS static constexpr decltype(Func{}(Args{}...))
                        invoke_shorter_interval_case(SignedSignificandBits, Func f,
                                                     Args... args) noexcept {
                            return f(args...);
                        }
                    };
                    struct nearest_toward_plus_infinity : base {
                        using decimal_to_binary_rounding_policy = nearest_toward_plus_infinity;
                        static constexpr auto tag = tag_t::to_nearest;
                        using normal_interval_type = interval_type::asymmetric_boundary;
                        using shorter_interval_type = interval_type::asymmetric_boundary;

                        template <class SignedSignificandBits, class Func, class... Args>
                        JKJ_FORCEINLINE JKJ_SAFEBUFFERS static constexpr decltype(Func{}(
                            declval<nearest_toward_plus_infinity>(), Args{}...))
                        delegate(SignedSignificandBits, Func f, Args... args) noexcept {
                            return f(nearest_toward_plus_infinity{}, args...);
                        }

                        template <class SignedSignificandBits, class Func, class... Args>
                        JKJ_FORCEINLINE JKJ_SAFEBUFFERS static constexpr decltype(Func{}(Args{}...,
                                                                                         false))
                        invoke_normal_interval_case(SignedSignificandBits s, Func f,
                                                    Args... args) noexcept {
                            return f(args..., !s.is_negative());
                        }
                        template <class SignedSignificandBits, class Func, class... Args>
                        JKJ_FORCEINLINE JKJ_SAFEBUFFERS static constexpr decltype(Func{}(Args{}...,
                                                                                         false))
                        invoke_shorter_interval_case(SignedSignificandBits s, Func f,
                                                     Args... args) noexcept {
                            return f(args..., !s.is_negative());
                        }
                    };
                    struct nearest_toward_minus_infinity : base {
                        using decimal_to_binary_rounding_policy = nearest_toward_minus_infinity;
                        static constexpr auto tag = tag_t::to_nearest;
                        using normal_interval_type = interval_type::asymmetric_boundary;
                        using shorter_interval_type = interval_type::asymmetric_boundary;

                        template <class SignedSignificandBits, class Func, class... Args>
                        JKJ_FORCEINLINE JKJ_SAFEBUFFERS static constexpr decltype(Func{}(
                            declval<nearest_toward_minus_infinity>(), Args{}...))
                        delegate(SignedSignificandBits, Func f, Args... args) noexcept {
                            return f(nearest_toward_minus_infinity{}, args...);
                        }

                        template <class SignedSignificandBits, class Func, class... Args>
                        JKJ_FORCEINLINE JKJ_SAFEBUFFERS static constexpr decltype(Func{}(Args{}...,
                                                                                         false))
                        invoke_normal_interval_case(SignedSignificandBits s, Func f,
                                                    Args... args) noexcept {
                            return f(args..., s.is_negative());
                        }
                        template <class SignedSignificandBits, class Func, class... Args>
                        JKJ_FORCEINLINE JKJ_SAFEBUFFERS static constexpr decltype(Func{}(Args{}...,
                                                                                         false))
                        invoke_shorter_interval_case(SignedSignificandBits s, Func f,
                                                     Args... args) noexcept {
                            return f(args..., s.is_negative());
                        }
                    };
                    struct nearest_toward_zero : base {
                        using decimal_to_binary_rounding_policy = nearest_toward_zero;
                        static constexpr auto tag = tag_t::to_nearest;
                        using normal_interval_type = interval_type::right_closed_left_open;
                        using shorter_interval_type = interval_type::right_closed_left_open;

                        template <class SignedSignificandBits, class Func, class... Args>
                        JKJ_FORCEINLINE JKJ_SAFEBUFFERS static constexpr decltype(Func{}(
                            declval<nearest_toward_zero>(), Args{}...))
                        delegate(SignedSignificandBits, Func f, Args... args) noexcept {
                            return f(nearest_toward_zero{}, args...);
                        }

                        template <class SignedSignificandBits, class Func, class... Args>
                        JKJ_FORCEINLINE JKJ_SAFEBUFFERS static constexpr decltype(Func{}(Args{}...))
                        invoke_normal_interval_case(SignedSignificandBits, Func f,
                                                    Args... args) noexcept {
                            return f(args...);
                        }
                        template <class SignedSignificandBits, class Func, class... Args>
                        JKJ_FORCEINLINE JKJ_SAFEBUFFERS static constexpr decltype(Func{}(Args{}...))
                        invoke_shorter_interval_case(SignedSignificandBits, Func f,
                                                     Args... args) noexcept {
                            return f(args...);
                        }
                    };
                    struct nearest_away_from_zero : base {
                        using decimal_to_binary_rounding_policy = nearest_away_from_zero;
                        static constexpr auto tag = tag_t::to_nearest;
                        using normal_interval_type = interval_type::left_closed_right_open;
                        using shorter_interval_type = interval_type::left_closed_right_open;

                        template <class SignedSignificandBits, class Func, class... Args>
                        JKJ_FORCEINLINE JKJ_SAFEBUFFERS static constexpr decltype(Func{}(
                            declval<nearest_away_from_zero>(), Args{}...))
                        delegate(SignedSignificandBits, Func f, Args... args) noexcept {
                            return f(nearest_away_from_zero{}, args...);
                        }

                        template <class SignedSignificandBits, class Func, class... Args>
                        JKJ_FORCEINLINE JKJ_SAFEBUFFERS static constexpr decltype(Func{}(Args{}...))
                        invoke_normal_interval_case(SignedSignificandBits, Func f,
                                                    Args... args) noexcept {
                            return f(args...);
                        }
                        template <class SignedSignificandBits, class Func, class... Args>
                        JKJ_FORCEINLINE JKJ_SAFEBUFFERS static constexpr decltype(Func{}(Args{}...))
                        invoke_shorter_interval_case(SignedSignificandBits, Func f,
                                                     Args... args) noexcept {
                            return f(args...);
                        }
                    };

                    namespace detail {
                        struct nearest_always_closed {
                            static constexpr auto tag = tag_t::to_nearest;
                            using normal_interval_type = interval_type::closed;
                            using shorter_interval_type = interval_type::closed;

                            template <class SignedSignificandBits, class Func, class... Args>
                            JKJ_FORCEINLINE JKJ_SAFEBUFFERS static constexpr decltype(Func{}(Args{}...))
                            invoke_normal_interval_case(SignedSignificandBits, Func f,
                                                        Args... args) noexcept {
                                return f(args...);
                            }
                            template <class SignedSignificandBits, class Func, class... Args>
                            JKJ_FORCEINLINE JKJ_SAFEBUFFERS static constexpr decltype(Func{}(Args{}...))
                            invoke_shorter_interval_case(SignedSignificandBits, Func f,
                                                         Args... args) noexcept {
                                return f(args...);
                            }
                        };
                        struct nearest_always_open {
                            static constexpr auto tag = tag_t::to_nearest;
                            using normal_interval_type = interval_type::open;
                            using shorter_interval_type = interval_type::open;

                            template <class SignedSignificandBits, class Func, class... Args>
                            JKJ_FORCEINLINE JKJ_SAFEBUFFERS static constexpr decltype(Func{}(Args{}...))
                            invoke_normal_interval_case(SignedSignificandBits, Func f,
                                                        Args... args) noexcept {
                                return f(args...);
                            }
                            template <class SignedSignificandBits, class Func, class... Args>
                            JKJ_FORCEINLINE JKJ_SAFEBUFFERS static constexpr decltype(Func{}(Args{}...))
                            invoke_shorter_interval_case(SignedSignificandBits, Func f,
                                                         Args... args) noexcept {
                                return f(args...);
                            }
                        };
                    }

                    struct nearest_to_even_static_boundary : base {
                        using decimal_to_binary_rounding_policy = nearest_to_even_static_boundary;
                        template <class SignedSignificandBits, class Func, class... Args>
                        JKJ_FORCEINLINE JKJ_SAFEBUFFERS static constexpr decltype(Func{}(
                            detail::nearest_always_closed{}, Args{}...))
                        delegate(SignedSignificandBits s, Func f, Args... args) noexcept {
                            return s.has_even_significand_bits()
                                       ? f(detail::nearest_always_closed{}, args...)
                                       : f(detail::nearest_always_open{}, args...);
                        }
                    };
                    struct nearest_to_odd_static_boundary : base {
                        using decimal_to_binary_rounding_policy = nearest_to_odd_static_boundary;
                        template <class SignedSignificandBits, class Func, class... Args>
                        JKJ_FORCEINLINE JKJ_SAFEBUFFERS static constexpr decltype(Func{}(
                            detail::nearest_always_closed{}, Args{}...))
                        delegate(SignedSignificandBits s, Func f, Args... args) noexcept {
                            return s.has_even_significand_bits()
                                       ? f(detail::nearest_always_open{}, args...)
                                       : f(detail::nearest_always_closed{}, args...);
                        }
                    };
                    struct nearest_toward_plus_infinity_static_boundary : base {
                        using decimal_to_binary_rounding_policy =
                            nearest_toward_plus_infinity_static_boundary;
                        template <class SignedSignificandBits, class Func, class... Args>
                        JKJ_FORCEINLINE
                            JKJ_SAFEBUFFERS static constexpr decltype(Func{}(nearest_toward_zero{},
                                                                             Args{}...))
                            delegate(SignedSignificandBits s, Func f, Args... args) noexcept {
                            return s.is_negative() ? f(nearest_toward_zero{}, args...)
                                                   : f(nearest_away_from_zero{}, args...);
                        }
                    };
                    struct nearest_toward_minus_infinity_static_boundary : base {
                        using decimal_to_binary_rounding_policy =
                            nearest_toward_minus_infinity_static_boundary;
                        template <class SignedSignificandBits, class Func, class... Args>
                        JKJ_FORCEINLINE
                            JKJ_SAFEBUFFERS static constexpr decltype(Func{}(nearest_toward_zero{},
                                                                             Args{}...))
                            delegate(SignedSignificandBits s, Func f, Args... args) noexcept {
                            return s.is_negative() ? f(nearest_away_from_zero{}, args...)
                                                   : f(nearest_toward_zero{}, args...);
                        }
                    };

                    namespace detail {
                        struct left_closed_directed {
                            static constexpr auto tag = tag_t::left_closed_directed;
                        };
                        struct right_closed_directed {
                            static constexpr auto tag = tag_t::right_closed_directed;
                        };
                    }

                    struct toward_plus_infinity : base {
                        using decimal_to_binary_rounding_policy = toward_plus_infinity;
                        template <class SignedSignificandBits, class Func, class... Args>
                        JKJ_FORCEINLINE JKJ_SAFEBUFFERS static constexpr decltype(Func{}(
                            detail::left_closed_directed{}, Args{}...))
                        delegate(SignedSignificandBits s, Func f, Args... args) noexcept {
                            return s.is_negative() ? f(detail::left_closed_directed{}, args...)
                                                   : f(detail::right_closed_directed{}, args...);
                        }
                    };
                    struct toward_minus_infinity : base {
                        using decimal_to_binary_rounding_policy = toward_minus_infinity;
                        template <class SignedSignificandBits, class Func, class... Args>
                        JKJ_FORCEINLINE JKJ_SAFEBUFFERS static constexpr decltype(Func{}(
                            detail::left_closed_directed{}, Args{}...))
                        delegate(SignedSignificandBits s, Func f, Args... args) noexcept {
                            return s.is_negative() ? f(detail::right_closed_directed{}, args...)
                                                   : f(detail::left_closed_directed{}, args...);
                        }
                    };
                    struct toward_zero : base {
                        using decimal_to_binary_rounding_policy = toward_zero;
                        template <class SignedSignificandBits, class Func, class... Args>
                        JKJ_FORCEINLINE JKJ_SAFEBUFFERS static constexpr decltype(Func{}(
                            detail::left_closed_directed{}, Args{}...))
                        delegate(SignedSignificandBits, Func f, Args... args) noexcept {
                            return f(detail::left_closed_directed{}, args...);
                        }
                    };
                    struct away_from_zero : base {
                        using decimal_to_binary_rounding_policy = away_from_zero;
                        template <class SignedSignificandBits, class Func, class... Args>
                        JKJ_FORCEINLINE JKJ_SAFEBUFFERS static constexpr decltype(Func{}(
                            detail::right_closed_directed{}, Args{}...))
                        delegate(SignedSignificandBits, Func f, Args... args) noexcept {
                            return f(detail::right_closed_directed{}, args...);
                        }
                    };
                }

                // Binary-to-decimal rounding policies.
                // (Always assumes nearest rounding modes.)
                namespace binary_to_decimal_rounding {
                    struct base {};

                    enum class tag_t { do_not_care, to_even, to_odd, away_from_zero, toward_zero };

                    struct do_not_care : base {
                        using binary_to_decimal_rounding_policy = do_not_care;
                        static constexpr auto tag = tag_t::do_not_care;

                        template <class CarrierUInt>
                        static constexpr bool prefer_round_down(CarrierUInt) noexcept {
                            return false;
                        }
                    };

                    struct to_even : base {
                        using binary_to_decimal_rounding_policy = to_even;
                        static constexpr auto tag = tag_t::to_even;

                        template <class CarrierUInt>
                        static constexpr bool prefer_round_down(CarrierUInt significand) noexcept {
                            return significand % 2 != 0;
                        }
                    };

                    struct to_odd : base {
                        using binary_to_decimal_rounding_policy = to_odd;
                        static constexpr auto tag = tag_t::to_odd;

                        template <class CarrierUInt>
                        static constexpr bool prefer_round_down(CarrierUInt significand) noexcept {
                            return significand % 2 == 0;
                        }
                    };

                    struct away_from_zero : base {
                        using binary_to_decimal_rounding_policy = away_from_zero;
                        static constexpr auto tag = tag_t::away_from_zero;

                        template <class CarrierUInt>
                        static constexpr bool prefer_round_down(CarrierUInt) noexcept {
                            return false;
                        }
                    };

                    struct toward_zero : base {
                        using binary_to_decimal_rounding_policy = toward_zero;
                        static constexpr auto tag = tag_t::toward_zero;

                        template <class CarrierUInt>
                        static constexpr bool prefer_round_down(CarrierUInt) noexcept {
                            return true;
                        }
                    };
                }

                // Cache policies.
                namespace cache {
                    struct base {};

                    struct full : base {
                        using cache_policy = full;
                        template <class FloatFormat>
                        static constexpr typename cache_holder<FloatFormat>::cache_entry_type
                        get_cache(int k) noexcept {
#if JKJ_HAS_CONSTEXPR14
                            assert(k >= cache_holder<FloatFormat>::min_k &&
                                   k <= cache_holder<FloatFormat>::max_k);
#endif
                            return cache_holder<FloatFormat>::cache[std::size_t(
                                k - cache_holder<FloatFormat>::min_k)];
                        }
                    };

                    struct compact : base {
                        using cache_policy = compact;

                        template <class FloatFormat, class Dummy = void>
                        struct get_cache_impl {
                            static constexpr typename cache_holder<FloatFormat>::cache_entry_type
                            get_cache(int k) noexcept {
                                return full::get_cache<FloatFormat>(k);
                            }
                        };

                        template <class Dummy>
                        struct get_cache_impl<ieee754_binary64, Dummy> {
                            static JKJ_CONSTEXPR20 cache_holder<ieee754_binary64>::cache_entry_type
                            get_cache(int k) noexcept {
                                // Compute the base index.
                                auto const cache_index =
                                    int(std::uint32_t(k - cache_holder<ieee754_binary64>::min_k) /
                                        compressed_cache_detail<>::compression_ratio);
                                auto const kb =
                                    cache_index * compressed_cache_detail<>::compression_ratio +
                                    cache_holder<ieee754_binary64>::min_k;
                                auto const offset = k - kb;

                                // Get the base cache.
                                auto const base_cache =
                                    compressed_cache_detail<>::cache.table[cache_index];

                                if (offset == 0) {
                                    return base_cache;
                                }
                                else {
                                    // Compute the required amount of bit-shift.
                                    auto const alpha = log::floor_log2_pow10(kb + offset) -
                                                       log::floor_log2_pow10(kb) - offset;
                                    assert(alpha > 0 && alpha < 64);

                                    // Try to recover the real cache.
                                    auto const pow5 = compressed_cache_detail<>::pow5.table[offset];
                                    auto recovered_cache = wuint::umul128(base_cache.high(), pow5);
                                    auto const middle_low = wuint::umul128(base_cache.low(), pow5);

                                    recovered_cache += middle_low.high();

                                    auto const high_to_middle = recovered_cache.high() << (64 - alpha);
                                    auto const middle_to_low = recovered_cache.low() << (64 - alpha);

                                    recovered_cache = wuint::uint128{
                                        (recovered_cache.low() >> alpha) | high_to_middle,
                                        ((middle_low.low() >> alpha) | middle_to_low)};

                                    assert(recovered_cache.low() + 1 != 0);
                                    recovered_cache = {recovered_cache.high(),
                                                       recovered_cache.low() + 1};

                                    return recovered_cache;
                                }
                            }
                        };

                        template <class FloatFormat>
                        static JKJ_CONSTEXPR20 typename cache_holder<FloatFormat>::cache_entry_type
                        get_cache(int k) noexcept {
                            assert(k >= cache_holder<FloatFormat>::min_k &&
                                   k <= cache_holder<FloatFormat>::max_k);

                            return get_cache_impl<FloatFormat>::get_cache(k);
                        }
                    };
                }
            }
        }

        namespace policy {
            namespace sign {
                JKJ_INLINE_VARIABLE auto ignore = detail::policy_impl::sign::ignore{};
                JKJ_INLINE_VARIABLE auto return_sign = detail::policy_impl::sign::return_sign{};
            }

            namespace trailing_zero {
                JKJ_INLINE_VARIABLE auto ignore = detail::policy_impl::trailing_zero::ignore{};
                JKJ_INLINE_VARIABLE auto remove = detail::policy_impl::trailing_zero::remove{};
                JKJ_INLINE_VARIABLE auto report = detail::policy_impl::trailing_zero::report{};
            }

            namespace decimal_to_binary_rounding {
                JKJ_INLINE_VARIABLE auto nearest_to_even =
                    detail::policy_impl::decimal_to_binary_rounding::nearest_to_even{};
                JKJ_INLINE_VARIABLE auto nearest_to_odd =
                    detail::policy_impl::decimal_to_binary_rounding::nearest_to_odd{};
                JKJ_INLINE_VARIABLE auto nearest_toward_plus_infinity =
                    detail::policy_impl::decimal_to_binary_rounding::nearest_toward_plus_infinity{};
                JKJ_INLINE_VARIABLE auto nearest_toward_minus_infinity =
                    detail::policy_impl::decimal_to_binary_rounding::nearest_toward_minus_infinity{};
                JKJ_INLINE_VARIABLE auto nearest_toward_zero =
                    detail::policy_impl::decimal_to_binary_rounding::nearest_toward_zero{};
                JKJ_INLINE_VARIABLE auto nearest_away_from_zero =
                    detail::policy_impl::decimal_to_binary_rounding::nearest_away_from_zero{};

                JKJ_INLINE_VARIABLE auto nearest_to_even_static_boundary =
                    detail::policy_impl::decimal_to_binary_rounding::nearest_to_even_static_boundary{};
                JKJ_INLINE_VARIABLE auto nearest_to_odd_static_boundary =
                    detail::policy_impl::decimal_to_binary_rounding::nearest_to_odd_static_boundary{};
                JKJ_INLINE_VARIABLE auto nearest_toward_plus_infinity_static_boundary =
                    detail::policy_impl::decimal_to_binary_rounding::
                        nearest_toward_plus_infinity_static_boundary{};
                JKJ_INLINE_VARIABLE auto nearest_toward_minus_infinity_static_boundary =
                    detail::policy_impl::decimal_to_binary_rounding::
                        nearest_toward_minus_infinity_static_boundary{};

                JKJ_INLINE_VARIABLE auto toward_plus_infinity =
                    detail::policy_impl::decimal_to_binary_rounding::toward_plus_infinity{};
                JKJ_INLINE_VARIABLE auto toward_minus_infinity =
                    detail::policy_impl::decimal_to_binary_rounding::toward_minus_infinity{};
                JKJ_INLINE_VARIABLE auto toward_zero =
                    detail::policy_impl::decimal_to_binary_rounding::toward_zero{};
                JKJ_INLINE_VARIABLE auto away_from_zero =
                    detail::policy_impl::decimal_to_binary_rounding::away_from_zero{};
            }

            namespace binary_to_decimal_rounding {
                JKJ_INLINE_VARIABLE auto do_not_care =
                    detail::policy_impl::binary_to_decimal_rounding::do_not_care{};
                JKJ_INLINE_VARIABLE auto to_even =
                    detail::policy_impl::binary_to_decimal_rounding::to_even{};
                JKJ_INLINE_VARIABLE auto to_odd =
                    detail::policy_impl::binary_to_decimal_rounding::to_odd{};
                JKJ_INLINE_VARIABLE auto away_from_zero =
                    detail::policy_impl::binary_to_decimal_rounding::away_from_zero{};
                JKJ_INLINE_VARIABLE auto toward_zero =
                    detail::policy_impl::binary_to_decimal_rounding::toward_zero{};
            }

            namespace cache {
                JKJ_INLINE_VARIABLE auto full = detail::policy_impl::cache::full{};
                JKJ_INLINE_VARIABLE auto compact = detail::policy_impl::cache::compact{};
            }
        }

        namespace detail {
            ////////////////////////////////////////////////////////////////////////////////////////
            // The main algorithm.
            ////////////////////////////////////////////////////////////////////////////////////////

            template <class Float, class FloatTraits>
            struct impl : private FloatTraits, private FloatTraits::format {
                using format = typename FloatTraits::format;
                using carrier_uint = typename FloatTraits::carrier_uint;

                using FloatTraits::carrier_bits;
                using format::significand_bits;
                using format::min_exponent;
                using format::max_exponent;
                using format::exponent_bias;
                using format::decimal_digits;

                static constexpr int kappa = std::is_same<format, ieee754_binary32>::value ? 1 : 2;
                static_assert(kappa >= 1, "");
                static_assert(carrier_bits >= significand_bits + 2 + log::floor_log2_pow10(kappa + 1),
                              "");

                static constexpr int min(int x, int y) noexcept { return x < y ? x : y; }
                static constexpr int max(int x, int y) noexcept { return x > y ? x : y; }

                static constexpr int min_k = min(
                    -log::floor_log10_pow2_minus_log10_4_over_3(int(max_exponent - significand_bits)),
                    -log::floor_log10_pow2(int(max_exponent - significand_bits)) + kappa);
                static_assert(min_k >= cache_holder<format>::min_k, "");

                // We do invoke shorter_interval_case for exponent == min_exponent case,
                // so we should not add 1 here.
                static constexpr int max_k =
                    max(-log::floor_log10_pow2_minus_log10_4_over_3(
                            int(min_exponent - significand_bits /*+ 1*/)),
                        -log::floor_log10_pow2(int(min_exponent - significand_bits)) + kappa);
                static_assert(max_k <= cache_holder<format>::max_k, "");

                using cache_entry_type = typename cache_holder<format>::cache_entry_type;
                static constexpr auto cache_bits = cache_holder<format>::cache_bits;

                static constexpr int case_shorter_interval_left_endpoint_lower_threshold = 2;
                static constexpr int case_shorter_interval_left_endpoint_upper_threshold =
                    2 +
                    log::floor_log2(
                        compute_power<
                            count_factors<5>((carrier_uint(1) << (significand_bits + 2)) - 1) + 1>(10) /
                        3);

                static constexpr int case_shorter_interval_right_endpoint_lower_threshold = 0;
                static constexpr int case_shorter_interval_right_endpoint_upper_threshold =
                    2 +
                    log::floor_log2(
                        compute_power<
                            count_factors<5>((carrier_uint(1) << (significand_bits + 1)) + 1) + 1>(10) /
                        3);

                static constexpr int shorter_interval_tie_lower_threshold =
                    -log::floor_log5_pow2_minus_log5_3(significand_bits + 4) - 2 - significand_bits;
                static constexpr int shorter_interval_tie_upper_threshold =
                    -log::floor_log5_pow2(significand_bits + 2) - 2 - significand_bits;

                struct compute_mul_result {
                    carrier_uint integer_part;
                    bool is_integer;
                };
                struct compute_mul_parity_result {
                    bool parity;
                    bool is_integer;
                };
                template <class FloatFormat, class Dummy = void>
                struct compute_mul_impl;

                //// The main algorithm assumes the input is a normal/subnormal finite number

                template <class ReturnType, class IntervalType, class TrailingZeroPolicy,
                          class BinaryToDecimalRoundingPolicy, class CachePolicy,
                          class... AdditionalArgs>
                JKJ_FORCEINLINE JKJ_SAFEBUFFERS static JKJ_CONSTEXPR20 ReturnType
                compute_nearest_normal(carrier_uint const two_fc, int const binary_exponent,
                                       AdditionalArgs... additional_args) noexcept {
                    //////////////////////////////////////////////////////////////////////
                    // Step 1: Schubfach multiplier calculation
                    //////////////////////////////////////////////////////////////////////

                    IntervalType interval_type{additional_args...};

                    // Compute k and beta.
                    int const minus_k = log::floor_log10_pow2(binary_exponent) - kappa;
                    auto const cache = CachePolicy::template get_cache<format>(-minus_k);
                    int const beta = binary_exponent + log::floor_log2_pow10(-minus_k);

                    // Compute zi and deltai.
                    // 10^kappa <= deltai < 10^(kappa + 1)
                    auto const deltai = compute_mul_impl<format>::compute_delta(cache, beta);
                    // For the case of binary32, the result of integer check is not correct for
                    // 29711844 * 2^-82
                    // = 6.1442653300000000008655037797566933477355632930994033813476... * 10^-18
                    // and 29711844 * 2^-81
                    // = 1.2288530660000000001731007559513386695471126586198806762695... * 10^-17,
                    // and they are the unique counterexamples. However, since 29711844 is even,
                    // this does not cause any problem for the endpoints calculations; it can only
                    // cause a problem when we need to perform integer check for the center.
                    // Fortunately, with these inputs, that branch is never executed, so we are
                    // fine.
                    auto const z_result =
                        compute_mul_impl<format>::compute_mul((two_fc | 1) << beta, cache);


                    //////////////////////////////////////////////////////////////////////
                    // Step 2: Try larger divisor; remove trailing zeros if necessary
                    //////////////////////////////////////////////////////////////////////

                    constexpr auto big_divisor = compute_power<kappa + 1>(std::uint32_t(10));
                    constexpr auto small_divisor = compute_power<kappa>(std::uint32_t(10));

                    // Using an upper bound on zi, we might be able to optimize the division
                    // better than the compiler; we are computing zi / big_divisor here.
                    carrier_uint decimal_significand =
                        div::divide_by_pow10<kappa + 1, carrier_uint,
                                             (carrier_uint(1) << (significand_bits + 1)) * big_divisor -
                                                 1>(z_result.integer_part);
                    auto r = std::uint32_t(z_result.integer_part - big_divisor * decimal_significand);

                    do {
                        if (r < deltai) {
                            // Exclude the right endpoint if necessary.
                            if (r == 0 &&
                                (z_result.is_integer & !interval_type.include_right_endpoint())) {
                                JKJ_IF_CONSTEXPR(
                                    BinaryToDecimalRoundingPolicy::tag ==
                                    policy_impl::binary_to_decimal_rounding::tag_t::do_not_care) {
                                    decimal_significand *= 10;
                                    --decimal_significand;
                                    return TrailingZeroPolicy::template no_trailing_zeros<impl,
                                                                                          ReturnType>(
                                        decimal_significand, minus_k + kappa);
                                }
                                else {
                                    --decimal_significand;
                                    r = big_divisor;
                                    break;
                                }
                            }
                        }
                        else if (r > deltai) {
                            break;
                        }
                        else {
                            // r == deltai; compare fractional parts.
                            auto const x_result =
                                compute_mul_impl<format>::compute_mul_parity(two_fc - 1, cache, beta);

                            if (!(x_result.parity |
                                  (x_result.is_integer & interval_type.include_left_endpoint()))) {
                                break;
                            }
                        }

                        // We may need to remove trailing zeros.
                        return TrailingZeroPolicy::template on_trailing_zeros<impl, ReturnType>(
                            decimal_significand, minus_k + kappa + 1);
                    } while (false);


                    //////////////////////////////////////////////////////////////////////
                    // Step 3: Find the significand with the smaller divisor
                    //////////////////////////////////////////////////////////////////////

                    decimal_significand *= 10;

                    JKJ_IF_CONSTEXPR(BinaryToDecimalRoundingPolicy::tag ==
                                     policy_impl::binary_to_decimal_rounding::tag_t::do_not_care) {
                        // Normally, we want to compute
                        // significand += r / small_divisor
                        // and return, but we need to take care of the case that the resulting
                        // value is exactly the right endpoint, while that is not included in the
                        // interval.
                        if (!interval_type.include_right_endpoint()) {
                            // Is r divisible by 10^kappa?
                            if (z_result.is_integer &&
                                div::check_divisibility_and_divide_by_pow10<kappa>(r)) {
                                // This should be in the interval.
                                decimal_significand += r - 1;
                            }
                            else {
                                decimal_significand += r;
                            }
                        }
                        else {
                            decimal_significand += div::small_division_by_pow10<kappa>(r);
                        }
                    }
                    else {
                        auto dist = r - (deltai / 2) + (small_divisor / 2);
                        bool const approx_y_parity = ((dist ^ (small_divisor / 2)) & 1) != 0;

                        // Is dist divisible by 10^kappa?
                        bool const divisible_by_small_divisor =
                            div::check_divisibility_and_divide_by_pow10<kappa>(dist);

                        // Add dist / 10^kappa to the significand.
                        decimal_significand += dist;

                        if (divisible_by_small_divisor) {
                            // Check z^(f) >= epsilon^(f).
                            // We have either yi == zi - epsiloni or yi == (zi - epsiloni) - 1,
                            // where yi == zi - epsiloni if and only if z^(f) >= epsilon^(f).
                            // Since there are only 2 possibilities, we only need to care about the
                            // parity. Also, zi and r should have the same parity since the divisor
                            // is an even number.
                            auto const y_result =
                                compute_mul_impl<format>::compute_mul_parity(two_fc, cache, beta);
                            if (y_result.parity != approx_y_parity) {
                                --decimal_significand;
                            }
                            else {
                                // If z^(f) >= epsilon^(f), we might have a tie
                                // when z^(f) == epsilon^(f), or equivalently, when y is an integer.
                                // For tie-to-up case, we can just choose the upper one.
                                if (BinaryToDecimalRoundingPolicy::prefer_round_down(
                                        decimal_significand) &
                                    y_result.is_integer) {
                                    --decimal_significand;
                                }
                            }
                        }
                    }
                    return TrailingZeroPolicy::template no_trailing_zeros<impl, ReturnType>(
                        decimal_significand, minus_k + kappa);
                }

                template <class ReturnType, class IntervalType, class TrailingZeroPolicy,
                          class BinaryToDecimalRoundingPolicy, class CachePolicy,
                          class... AdditionalArgs>
                JKJ_SAFEBUFFERS static JKJ_CONSTEXPR20 ReturnType compute_nearest_shorter(
                    int const binary_exponent, AdditionalArgs... additional_args) noexcept {
                    IntervalType interval_type{additional_args...};

                    // Compute k and beta.
                    int const minus_k = log::floor_log10_pow2_minus_log10_4_over_3(binary_exponent);
                    int const beta = binary_exponent + log::floor_log2_pow10(-minus_k);

                    // Compute xi and zi.
                    auto const cache = CachePolicy::template get_cache<format>(-minus_k);

                    auto xi = compute_mul_impl<format>::compute_left_endpoint_for_shorter_interval_case(
                        cache, beta);
                    auto zi =
                        compute_mul_impl<format>::compute_right_endpoint_for_shorter_interval_case(
                            cache, beta);

                    // If we don't accept the right endpoint and
                    // if the right endpoint is an integer, decrease it.
                    if (!interval_type.include_right_endpoint() &&
                        is_right_endpoint_integer_shorter_interval(binary_exponent)) {
                        --zi;
                    }
                    // If we don't accept the left endpoint or
                    // if the left endpoint is not an integer, increase it.
                    if (!interval_type.include_left_endpoint() ||
                        !is_left_endpoint_integer_shorter_interval(binary_exponent)) {
                        ++xi;
                    }

                    // Try bigger divisor.
                    carrier_uint decimal_significand = zi / 10;

                    // If succeed, remove trailing zeros if necessary and return.
                    if (decimal_significand * 10 >= xi) {
                        return TrailingZeroPolicy::template on_trailing_zeros<impl, ReturnType>(
                            decimal_significand, minus_k + 1);
                    }

                    // Otherwise, compute the round-up of y.
                    decimal_significand =
                        compute_mul_impl<format>::compute_round_up_for_shorter_interval_case(cache,
                                                                                             beta);

                    // When tie occurs, choose one of them according to the rule.
                    if (BinaryToDecimalRoundingPolicy::prefer_round_down(decimal_significand) &&
                        binary_exponent >= shorter_interval_tie_lower_threshold &&
                        binary_exponent <= shorter_interval_tie_upper_threshold) {
                        --decimal_significand;
                    }
                    else if (decimal_significand < xi) {
                        ++decimal_significand;
                    }
                    return TrailingZeroPolicy::template no_trailing_zeros<impl, ReturnType>(
                        decimal_significand, minus_k);
                }

                template <class ReturnType, class TrailingZeroPolicy, class CachePolicy>
                JKJ_FORCEINLINE JKJ_SAFEBUFFERS static JKJ_CONSTEXPR20 ReturnType
                compute_left_closed_directed(carrier_uint const two_fc, int binary_exponent) noexcept {
                    //////////////////////////////////////////////////////////////////////
                    // Step 1: Schubfach multiplier calculation
                    //////////////////////////////////////////////////////////////////////

                    // Compute k and beta.
                    int const minus_k = log::floor_log10_pow2(binary_exponent) - kappa;
                    auto const cache = CachePolicy::template get_cache<format>(-minus_k);
                    int const beta = binary_exponent + log::floor_log2_pow10(-minus_k);

                    // Compute xi and deltai.
                    // 10^kappa <= deltai < 10^(kappa + 1)
                    auto const deltai = compute_mul_impl<format>::compute_delta(cache, beta);
                    auto x_result = compute_mul_impl<format>::compute_mul(two_fc << beta, cache);

                    // Deal with the unique exceptional cases
                    // 29711844 * 2^-82
                    // = 6.1442653300000000008655037797566933477355632930994033813476... * 10^-18
                    // and 29711844 * 2^-81
                    // = 1.2288530660000000001731007559513386695471126586198806762695... * 10^-17
                    // for binary32.
                    JKJ_IF_CONSTEXPR(std::is_same<format, ieee754_binary32>::value) {
                        if (binary_exponent <= -80) {
                            x_result.is_integer = false;
                        }
                    }

                    if (!x_result.is_integer) {
                        ++x_result.integer_part;
                    }

                    //////////////////////////////////////////////////////////////////////
                    // Step 2: Try larger divisor; remove trailing zeros if necessary
                    //////////////////////////////////////////////////////////////////////

                    constexpr auto big_divisor = compute_power<kappa + 1>(std::uint32_t(10));

                    // Using an upper bound on xi, we might be able to optimize the division
                    // better than the compiler; we are computing xi / big_divisor here.
                    carrier_uint decimal_significand =
                        div::divide_by_pow10<kappa + 1, carrier_uint,
                                             (carrier_uint(1) << (significand_bits + 1)) * big_divisor -
                                                 1>(x_result.integer_part);
                    auto r = std::uint32_t(x_result.integer_part - big_divisor * decimal_significand);

                    if (r != 0) {
                        ++decimal_significand;
                        r = big_divisor - r;
                    }

                    do {
                        if (r > deltai) {
                            break;
                        }
                        else if (r == deltai) {
                            // Compare the fractional parts.
                            // This branch is never taken for the exceptional cases
                            // 2f_c = 29711482, e = -81
                            // (6.1442649164096937243516663440523473127541365101933479309082... *
                            // 10^-18) and 2f_c = 29711482, e = -80
                            // (1.2288529832819387448703332688104694625508273020386695861816... *
                            // 10^-17).
                            auto const z_result =
                                compute_mul_impl<format>::compute_mul_parity(two_fc + 2, cache, beta);
                            if (z_result.parity || z_result.is_integer) {
                                break;
                            }
                        }

                        // The ceiling is inside, so we are done.
                        return TrailingZeroPolicy::template on_trailing_zeros<impl, ReturnType>(
                            decimal_significand, minus_k + kappa + 1);
                    } while (false);


                    //////////////////////////////////////////////////////////////////////
                    // Step 3: Find the significand with the smaller divisor
                    //////////////////////////////////////////////////////////////////////

                    decimal_significand *= 10;
                    decimal_significand -= div::small_division_by_pow10<kappa>(r);
                    return TrailingZeroPolicy::template no_trailing_zeros<impl, ReturnType>(
                        decimal_significand, minus_k + kappa);
                }

                template <class ReturnType, class TrailingZeroPolicy, class CachePolicy>
                JKJ_FORCEINLINE JKJ_SAFEBUFFERS static JKJ_CONSTEXPR20 ReturnType
                compute_right_closed_directed(carrier_uint const two_fc, int const binary_exponent,
                                              bool shorter_interval) noexcept {
                    //////////////////////////////////////////////////////////////////////
                    // Step 1: Schubfach multiplier calculation
                    //////////////////////////////////////////////////////////////////////

                    // Compute k and beta.
                    int const minus_k =
                        log::floor_log10_pow2(binary_exponent - (shorter_interval ? 1 : 0)) - kappa;
                    auto const cache = CachePolicy::template get_cache<format>(-minus_k);
                    int const beta = binary_exponent + log::floor_log2_pow10(-minus_k);

                    // Compute zi and deltai.
                    // 10^kappa <= deltai < 10^(kappa + 1)
                    auto const deltai = shorter_interval
                                            ? compute_mul_impl<format>::compute_delta(cache, beta - 1)
                                            : compute_mul_impl<format>::compute_delta(cache, beta);
                    carrier_uint const zi =
                        compute_mul_impl<format>::compute_mul(two_fc << beta, cache).integer_part;


                    //////////////////////////////////////////////////////////////////////
                    // Step 2: Try larger divisor; remove trailing zeros if necessary
                    //////////////////////////////////////////////////////////////////////

                    constexpr auto big_divisor = compute_power<kappa + 1>(std::uint32_t(10));

                    // Using an upper bound on zi, we might be able to optimize the division better
                    // than the compiler; we are computing zi / big_divisor here.
                    carrier_uint decimal_significand =
                        div::divide_by_pow10<kappa + 1, carrier_uint,
                                             (carrier_uint(1) << (significand_bits + 1)) * big_divisor -
                                                 1>(zi);
                    auto const r = std::uint32_t(zi - big_divisor * decimal_significand);

                    do {
                        if (r > deltai) {
                            break;
                        }
                        else if (r == deltai) {
                            // Compare the fractional parts.
                            if (!compute_mul_impl<format>::compute_mul_parity(
                                     two_fc - (shorter_interval ? 1 : 2), cache, beta)
                                     .parity) {
                                break;
                            }
                        }

                        // The floor is inside, so we are done.
                        return TrailingZeroPolicy::template on_trailing_zeros<impl, ReturnType>(
                            decimal_significand, minus_k + kappa + 1);
                    } while (false);


                    //////////////////////////////////////////////////////////////////////
                    // Step 3: Find the significand with the small divisor
                    //////////////////////////////////////////////////////////////////////

                    decimal_significand *= 10;
                    decimal_significand += div::small_division_by_pow10<kappa>(r);
                    return TrailingZeroPolicy::template no_trailing_zeros<impl, ReturnType>(
                        decimal_significand, minus_k + kappa);
                }

                // Remove trailing zeros from n and return the number of zeros removed.
                JKJ_FORCEINLINE static JKJ_CONSTEXPR20 int
                remove_trailing_zeros(carrier_uint& n) noexcept {
                    assert(n != 0);

                    JKJ_IF_CONSTEXPR(std::is_same<format, ieee754_binary32>::value) {
                        constexpr auto mod_inv_5 = std::uint32_t(0xcccccccd);
                        constexpr auto mod_inv_25 = mod_inv_5 * mod_inv_5;

                        int s = 0;
                        while (true) {
                            auto q = bits::rotr(n * mod_inv_25, 2);
                            if (q <= std::numeric_limits<std::uint32_t>::max() / 100) {
                                n = q;
                                s += 2;
                            }
                            else {
                                break;
                            }
                        }
                        auto q = bits::rotr(n * mod_inv_5, 1);
                        if (q <= std::numeric_limits<std::uint32_t>::max() / 10) {
                            n = q;
                            s |= 1;
                        }

                        return s;
                    }
                    else {
#if JKJ_HAS_IF_CONSTEXPR
                        static_assert(std::is_same<format, ieee754_binary64>::value, "");
#endif

                        // Divide by 10^8 and reduce to 32-bits if divisible.
                        // Since ret_value.significand <= (2^53 * 1000 - 1) / 1000 < 10^16,
                        // n is at most of 16 digits.

                        // This magic number is ceil(2^90 / 10^8).
                        constexpr auto magic_number = std::uint64_t(12379400392853802749ull);
                        auto nm = wuint::umul128(n, magic_number);

                        // Is n is divisible by 10^8?
                        if ((nm.high() & ((std::uint64_t(1) << (90 - 64)) - 1)) == 0 &&
                            nm.low() < magic_number) {
                            // If yes, work with the quotient.
                            auto n32 = std::uint32_t(nm.high() >> (90 - 64));

                            constexpr auto mod_inv_5 = std::uint32_t(0xcccccccd);
                            constexpr auto mod_inv_25 = mod_inv_5 * mod_inv_5;

                            int s = 8;
                            while (true) {
                                auto q = bits::rotr(n32 * mod_inv_25, 2);
                                if (q <= std::numeric_limits<std::uint32_t>::max() / 100) {
                                    n32 = q;
                                    s += 2;
                                }
                                else {
                                    break;
                                }
                            }
                            auto q = bits::rotr(n32 * mod_inv_5, 1);
                            if (q <= std::numeric_limits<std::uint32_t>::max() / 10) {
                                n32 = q;
                                s |= 1;
                            }

                            n = n32;
                            return s;
                        }

                        // If n is not divisible by 10^8, work with n itself.
                        constexpr auto mod_inv_5 = std::uint64_t(0xcccccccccccccccd);
                        constexpr auto mod_inv_25 = mod_inv_5 * mod_inv_5;

                        int s = 0;
                        while (true) {
                            auto q = bits::rotr(n * mod_inv_25, 2);
                            if (q <= std::numeric_limits<std::uint64_t>::max() / 100) {
                                n = q;
                                s += 2;
                            }
                            else {
                                break;
                            }
                        }
                        auto q = bits::rotr(n * mod_inv_5, 1);
                        if (q <= std::numeric_limits<std::uint64_t>::max() / 10) {
                            n = q;
                            s |= 1;
                        }

                        return s;
                    }
                }

                template <class Dummy>
                struct compute_mul_impl<ieee754_binary32, Dummy> {
                    static JKJ_CONSTEXPR20 compute_mul_result
                    compute_mul(carrier_uint u, cache_entry_type const& cache) noexcept {
                        auto r = wuint::umul96_upper64(u, cache);
                        return {carrier_uint(r >> 32), carrier_uint(r) == 0};
                    }

                    static constexpr std::uint32_t compute_delta(cache_entry_type const& cache,
                                                                 int beta) noexcept {
                        return std::uint32_t(cache >> (cache_bits - 1 - beta));
                    }

                    static JKJ_CONSTEXPR20 compute_mul_parity_result compute_mul_parity(
                        carrier_uint two_f, cache_entry_type const& cache, int beta) noexcept {
                        assert(beta >= 1);
                        assert(beta < 64);

                        auto r = wuint::umul96_lower64(two_f, cache);
                        return {((r >> (64 - beta)) & 1) != 0, std::uint32_t(r >> (32 - beta)) == 0};
                    }

                    static constexpr carrier_uint
                    compute_left_endpoint_for_shorter_interval_case(cache_entry_type const& cache,
                                                                    int beta) noexcept {
                        return carrier_uint((cache - (cache >> (significand_bits + 2))) >>
                                            (cache_bits - significand_bits - 1 - beta));
                    }

                    static constexpr carrier_uint
                    compute_right_endpoint_for_shorter_interval_case(cache_entry_type const& cache,
                                                                     int beta) noexcept {
                        return carrier_uint((cache + (cache >> (significand_bits + 1))) >>
                                            (cache_bits - significand_bits - 1 - beta));
                    }

                    static constexpr carrier_uint
                    compute_round_up_for_shorter_interval_case(cache_entry_type const& cache,
                                                               int beta) noexcept {
                        return (carrier_uint(cache >> (cache_bits - significand_bits - 2 - beta)) + 1) /
                               2;
                    }
                };

                template <class Dummy>
                struct compute_mul_impl<ieee754_binary64, Dummy> {
                    static JKJ_CONSTEXPR20 compute_mul_result
                    compute_mul(carrier_uint u, cache_entry_type const& cache) noexcept {
                        auto r = wuint::umul192_upper128(u, cache);
                        return {r.high(), r.low() == 0};
                    }

                    static constexpr std::uint32_t compute_delta(cache_entry_type const& cache,
                                                                 int beta) noexcept {
                        return std::uint32_t(cache.high() >> (carrier_bits - 1 - beta));
                    }

                    static JKJ_CONSTEXPR20 compute_mul_parity_result compute_mul_parity(
                        carrier_uint two_f, cache_entry_type const& cache, int beta) noexcept {
                        assert(beta >= 1);
                        assert(beta < 64);

                        auto r = wuint::umul192_lower128(two_f, cache);
                        return {((r.high() >> (64 - beta)) & 1) != 0,
                                ((r.high() << beta) | (r.low() >> (64 - beta))) == 0};
                    }

                    static constexpr carrier_uint
                    compute_left_endpoint_for_shorter_interval_case(cache_entry_type const& cache,
                                                                    int beta) noexcept {
                        return (cache.high() - (cache.high() >> (significand_bits + 2))) >>
                               (carrier_bits - significand_bits - 1 - beta);
                    }

                    static constexpr carrier_uint
                    compute_right_endpoint_for_shorter_interval_case(cache_entry_type const& cache,
                                                                     int beta) noexcept {
                        return (cache.high() + (cache.high() >> (significand_bits + 1))) >>
                               (carrier_bits - significand_bits - 1 - beta);
                    }

                    static constexpr carrier_uint
                    compute_round_up_for_shorter_interval_case(cache_entry_type const& cache,
                                                               int beta) noexcept {
                        return ((cache.high() >> (carrier_bits - significand_bits - 2 - beta)) + 1) / 2;
                    }
                };

                static constexpr bool
                is_right_endpoint_integer_shorter_interval(int exponent) noexcept {
                    return exponent >= case_shorter_interval_right_endpoint_lower_threshold &&
                           exponent <= case_shorter_interval_right_endpoint_upper_threshold;
                }

                static constexpr bool is_left_endpoint_integer_shorter_interval(int exponent) noexcept {
                    return exponent >= case_shorter_interval_left_endpoint_lower_threshold &&
                           exponent <= case_shorter_interval_left_endpoint_upper_threshold;
                }
            };


            ////////////////////////////////////////////////////////////////////////////////////////
            // Policy holder.
            ////////////////////////////////////////////////////////////////////////////////////////

            namespace policy_impl {
                // The library will specify a list of accepted kinds of policies and their defaults,
                // and the user will pass a list of policies. The aim of helper classes/functions
                // here is to do the following:
                //   1. Check if the policy parameters given by the user are all valid; that means,
                //      each of them should be of the kinds specified by the library.
                //      If that's not the case, then the compilation fails.
                //   2. Check if multiple policy parameters for the same kind is specified by the
                //   user.
                //      If that's the case, then the compilation fails.
                //   3. Build a class deriving from all policies the user have given, and also from
                //      the default policies if the user did not specify one for some kinds.
                // A policy belongs to a certain kind if it is deriving from a base class.

                // For a given kind, find a policy belonging to that kind.
                // Check if there are more than one such policies.
                enum class policy_found_info { not_found, unique, repeated };
                template <class Policy, policy_found_info info>
                struct found_policy_pair {
                    using policy = Policy;
                    static constexpr auto found_info = info;
                };

                template <class Base, class DefaultPolicy>
                struct base_default_pair {
                    using base = Base;

                    template <class FoundPolicyInfo, class... Policies>
                    struct get_found_policy_pair_impl;

                    template <class FoundPolicyInfo>
                    struct get_found_policy_pair_impl<FoundPolicyInfo> {
                        using type = FoundPolicyInfo;
                    };

                    template <class FoundPolicyInfo, class FirstPolicy, class... RemainingPolicies>
                    struct get_found_policy_pair_impl<FoundPolicyInfo, FirstPolicy,
                                                      RemainingPolicies...> {
                        using type = typename std::conditional<
                            std::is_base_of<Base, FirstPolicy>::value,
                            typename std::conditional<
                                FoundPolicyInfo::found_info == policy_found_info::not_found,
                                typename get_found_policy_pair_impl<
                                    found_policy_pair<FirstPolicy, policy_found_info::unique>,
                                    RemainingPolicies...>::type,
                                typename get_found_policy_pair_impl<
                                    found_policy_pair<FirstPolicy, policy_found_info::repeated>,
                                    RemainingPolicies...>::type>::type,
                            typename get_found_policy_pair_impl<FoundPolicyInfo,
                                                                RemainingPolicies...>::type>::type;
                    };

                    template <class... Policies>
                    using get_found_policy_pair = typename get_found_policy_pair_impl<
                        found_policy_pair<DefaultPolicy, policy_found_info::not_found>,
                        Policies...>::type;
                };
                template <class... BaseDefaultPairs>
                struct base_default_pair_list {};

                // Check if a given policy belongs to one of the kinds specified by the library.
                template <class Policy>
                constexpr bool check_policy_validity(Policy, base_default_pair_list<>) {
                    return false;
                }
                template <class Policy, class FirstBaseDefaultPair, class... RemainingBaseDefaultPairs>
                constexpr bool check_policy_validity(
                    Policy,
                    base_default_pair_list<FirstBaseDefaultPair, RemainingBaseDefaultPairs...>) {
                    return std::is_base_of<typename FirstBaseDefaultPair::base, Policy>::value ||
                           check_policy_validity(
                               Policy{}, base_default_pair_list<RemainingBaseDefaultPairs...>{});
                }

                template <class BaseDefaultPairList>
                constexpr bool check_policy_list_validity(BaseDefaultPairList) {
                    return true;
                }

                template <class BaseDefaultPairList, class FirstPolicy, class... RemainingPolicies>
                constexpr bool check_policy_list_validity(BaseDefaultPairList, FirstPolicy,
                                                          RemainingPolicies... remaining_policies) {
                    return check_policy_validity(FirstPolicy{}, BaseDefaultPairList{}) &&
                           check_policy_list_validity(BaseDefaultPairList{}, remaining_policies...);
                }

                // Build policy_holder.
                template <bool repeated_, class... FoundPolicyPairs>
                struct found_policy_pair_list {
                    static constexpr bool repeated = repeated_;
                };

                template <class... Policies>
                struct policy_holder : Policies... {};

                template <class BaseDefaultPairList, class FoundPolicyPairList, class... Policies>
                struct make_policy_holder_impl;

                template <bool repeated, class... FoundPolicyPairs, class... Policies>
                struct make_policy_holder_impl<base_default_pair_list<>,
                                               found_policy_pair_list<repeated, FoundPolicyPairs...>,
                                               Policies...> {
                    using type = found_policy_pair_list<repeated, FoundPolicyPairs...>;
                };

                template <class FirstBaseDefaultPair, class... RemainingBaseDefaultPairs, bool repeated,
                          class... FoundPolicyPairs, class... Policies>
                struct make_policy_holder_impl<
                    base_default_pair_list<FirstBaseDefaultPair, RemainingBaseDefaultPairs...>,
                    found_policy_pair_list<repeated, FoundPolicyPairs...>, Policies...> {
                    using new_found_policy_pair =
                        typename FirstBaseDefaultPair::template get_found_policy_pair<Policies...>;

                    using type = typename make_policy_holder_impl<
                        base_default_pair_list<RemainingBaseDefaultPairs...>,
                        found_policy_pair_list<(repeated || new_found_policy_pair::found_info ==
                                                                policy_found_info::repeated),
                                               new_found_policy_pair, FoundPolicyPairs...>,
                        Policies...>::type;
                };

                template <class BaseDefaultPairList, class... Policies>
                using policy_pair_list =
                    typename make_policy_holder_impl<BaseDefaultPairList, found_policy_pair_list<false>,
                                                     Policies...>::type;

                template <class FoundPolicyPairList, class... RawPolicies>
                struct convert_to_policy_holder_impl;

                template <bool repeated, class... RawPolicies>
                struct convert_to_policy_holder_impl<found_policy_pair_list<repeated>, RawPolicies...> {
                    using type = policy_holder<RawPolicies...>;
                };

                template <bool repeated, class FirstFoundPolicyPair, class... RemainingFoundPolicyPairs,
                          class... RawPolicies>
                struct convert_to_policy_holder_impl<
                    found_policy_pair_list<repeated, FirstFoundPolicyPair,
                                           RemainingFoundPolicyPairs...>,
                    RawPolicies...> {
                    using type = typename convert_to_policy_holder_impl<
                        found_policy_pair_list<repeated, RemainingFoundPolicyPairs...>,
                        typename FirstFoundPolicyPair::policy, RawPolicies...>::type;
                };

                template <class FoundPolicyPairList>
                using convert_to_policy_holder =
                    typename convert_to_policy_holder_impl<FoundPolicyPairList>::type;

                template <class BaseDefaultPairList, class... Policies>
                constexpr convert_to_policy_holder<policy_pair_list<BaseDefaultPairList, Policies...>>
                make_policy_holder(BaseDefaultPairList, Policies... policies) {
                    static_assert(check_policy_list_validity(BaseDefaultPairList{}, Policies{}...),
                                  "jkj::dragonbox: an invalid policy is specified");

                    static_assert(!policy_pair_list<BaseDefaultPairList, Policies...>::repeated,
                                  "jkj::dragonbox: each policy should be specified at most once");

                    return {};
                }
            }

            template <class... Policies>
            using to_decimal_policy_holder = decltype(policy_impl::make_policy_holder(
                policy_impl::base_default_pair_list<
                    policy_impl::base_default_pair<policy_impl::sign::base,
                                                   policy_impl::sign::return_sign>,
                    policy_impl::base_default_pair<policy_impl::trailing_zero::base,
                                                   policy_impl::trailing_zero::remove>,
                    policy_impl::base_default_pair<
                        policy_impl::decimal_to_binary_rounding::base,
                        policy_impl::decimal_to_binary_rounding::nearest_to_even>,
                    policy_impl::base_default_pair<policy_impl::binary_to_decimal_rounding::base,
                                                   policy_impl::binary_to_decimal_rounding::to_even>,
                    policy_impl::base_default_pair<policy_impl::cache::base,
                                                   policy_impl::cache::full>>{},
                Policies{}...));

            template <class FloatTraits, class... Policies>
            using to_decimal_return_type =
                decimal_fp<typename FloatTraits::carrier_uint,
                           to_decimal_policy_holder<Policies...>::return_has_sign,
                           to_decimal_policy_holder<Policies...>::report_trailing_zeros>;

            template <class Float, class FloatTraits, class PolicyHolder, class IntervalTypeProvider>
            struct invoke_shorter_dispatcher {
                using unsigned_return_type = decimal_fp<typename FloatTraits::carrier_uint, false,
                                                        PolicyHolder::report_trailing_zeros>;

                template <class... Args>
                JKJ_FORCEINLINE JKJ_SAFEBUFFERS JKJ_CONSTEXPR20 unsigned_return_type
                operator()(Args... args) noexcept {
                    return impl<Float, FloatTraits>::template compute_nearest_shorter<
                        unsigned_return_type, typename IntervalTypeProvider::shorter_interval_type,
                        typename PolicyHolder::trailing_zero_policy,
                        typename PolicyHolder::binary_to_decimal_rounding_policy,
                        typename PolicyHolder::cache_policy>(args...);
                }
            };

            template <class Float, class FloatTraits, class PolicyHolder, class IntervalTypeProvider>
            struct invoke_normal_dispatcher {
                using unsigned_return_type = decimal_fp<typename FloatTraits::carrier_uint, false,
                                                        PolicyHolder::report_trailing_zeros>;

                template <class... Args>
                JKJ_FORCEINLINE JKJ_SAFEBUFFERS JKJ_CONSTEXPR20 unsigned_return_type
                operator()(Args... args) noexcept {
                    return impl<Float, FloatTraits>::template compute_nearest_normal<
                        unsigned_return_type, typename IntervalTypeProvider::normal_interval_type,
                        typename PolicyHolder::trailing_zero_policy,
                        typename PolicyHolder::binary_to_decimal_rounding_policy,
                        typename PolicyHolder::cache_policy>(args...);
                }
            };

            template <class Float, class FloatTraits, class PolicyHolder, class IntervalTypeProvider>
            JKJ_SAFEBUFFERS JKJ_CONSTEXPR20
                decimal_fp<typename FloatTraits::carrier_uint, PolicyHolder::return_has_sign,
                           PolicyHolder::report_trailing_zeros>
                to_decimal_impl(signed_significand_bits<Float, FloatTraits> signed_significand_bits,
                                unsigned int exponent_bits) noexcept {
                using namespace policy_impl;
                using unsigned_return_type = decimal_fp<typename FloatTraits::carrier_uint, false,
                                                        PolicyHolder::report_trailing_zeros>;
                using format = typename FloatTraits::format;
                constexpr auto tag = IntervalTypeProvider::tag;

                auto two_fc = signed_significand_bits.remove_sign_bit_and_shift();
                auto exponent = int(exponent_bits);

                JKJ_IF_CONSTEXPR(tag == decimal_to_binary_rounding::tag_t::to_nearest) {
                    // Is the input a normal number?
                    if (exponent != 0) {
                        exponent += format::exponent_bias - format::significand_bits;

                        // Shorter interval case; proceed like Schubfach.
                        // One might think this condition is wrong, since when exponent_bits ==
                        // 1 and two_fc == 0, the interval is actually regular. However, it
                        // turns out that this seemingly wrong condition is actually fine,
                        // because the end result is anyway the same.
                        //
                        // [binary32]
                        // (fc-1/2) * 2^e = 1.175'494'28... * 10^-38
                        // (fc-1/4) * 2^e = 1.175'494'31... * 10^-38
                        //    fc    * 2^e = 1.175'494'35... * 10^-38
                        // (fc+1/2) * 2^e = 1.175'494'42... * 10^-38
                        //
                        // Hence, shorter_interval_case will return 1.175'494'4 * 10^-38.
                        // 1.175'494'3 * 10^-38 is also a correct shortest representation that
                        // will be rejected if we assume shorter interval, but 1.175'494'4 *
                        // 10^-38 is closer to the true value so it doesn't matter.
                        //
                        // [binary64]
                        // (fc-1/2) * 2^e = 2.225'073'858'507'201'13... * 10^-308
                        // (fc-1/4) * 2^e = 2.225'073'858'507'201'25... * 10^-308
                        //    fc    * 2^e = 2.225'073'858'507'201'38... * 10^-308
                        // (fc+1/2) * 2^e = 2.225'073'858'507'201'63... * 10^-308
                        //
                        // Hence, shorter_interval_case will return 2.225'073'858'507'201'4 *
                        // 10^-308. This is indeed of the shortest length, and it is the unique
                        // one closest to the true value among valid representations of the same
                        // length.
                        static_assert(std::is_same<format, ieee754_binary32>::value ||
                                          std::is_same<format, ieee754_binary64>::value,
                                      "");

                        if (two_fc == 0) {
                            return PolicyHolder::handle_sign(
                                signed_significand_bits,
                                IntervalTypeProvider::invoke_shorter_interval_case(
                                    signed_significand_bits,
                                    invoke_shorter_dispatcher<Float, FloatTraits, PolicyHolder,
                                                              IntervalTypeProvider>{},
                                    exponent));
                        }

                        two_fc |= (decltype(two_fc)(1) << (format::significand_bits + 1));
                    }
                    // Is the input a subnormal number?
                    else {
                        exponent = format::min_exponent - format::significand_bits;
                    }

                    return PolicyHolder::handle_sign(
                        signed_significand_bits,
                        IntervalTypeProvider::invoke_normal_interval_case(
                            signed_significand_bits,
                            invoke_normal_dispatcher<Float, FloatTraits, PolicyHolder,
                                                     IntervalTypeProvider>{},
                            two_fc, exponent));
                }
                else JKJ_IF_CONSTEXPR(tag == decimal_to_binary_rounding::tag_t::left_closed_directed) {
                    // Is the input a normal number?
                    if (exponent != 0) {
                        exponent += format::exponent_bias - format::significand_bits;
                        two_fc |= (decltype(two_fc)(1) << (format::significand_bits + 1));
                    }
                    // Is the input a subnormal number?
                    else {
                        exponent = format::min_exponent - format::significand_bits;
                    }

                    return PolicyHolder::handle_sign(
                        signed_significand_bits,
                        detail::impl<Float, FloatTraits>::template compute_left_closed_directed<
                            unsigned_return_type, typename PolicyHolder::trailing_zero_policy,
                            typename PolicyHolder::cache_policy>(two_fc, exponent));
                }
                else {
#if JKJ_HAS_IF_CONSTEXPR
                    static_assert(tag == decimal_to_binary_rounding::tag_t::right_closed_directed, "");
#endif

                    bool shorter_interval = false;

                    // Is the input a normal number?
                    if (exponent != 0) {
                        if (two_fc == 0 && exponent != 1) {
                            shorter_interval = true;
                        }
                        exponent += format::exponent_bias - format::significand_bits;
                        two_fc |= (decltype(two_fc)(1) << (format::significand_bits + 1));
                    }
                    // Is the input a subnormal number?
                    else {
                        exponent = format::min_exponent - format::significand_bits;
                    }

                    return PolicyHolder::handle_sign(
                        signed_significand_bits,
                        detail::impl<Float, FloatTraits>::template compute_right_closed_directed<
                            unsigned_return_type, typename PolicyHolder::trailing_zero_policy,
                            typename PolicyHolder::cache_policy>(two_fc, exponent, shorter_interval));
                }
            }

            template <class Float, class FloatTraits, class PolicyHolder>
            struct to_decimal_dispatcher {
                using return_type =
                    decimal_fp<typename FloatTraits::carrier_uint, PolicyHolder::return_has_sign,
                               PolicyHolder::report_trailing_zeros>;

                template <class IntervalTypeProvider, class... Args>
                JKJ_FORCEINLINE JKJ_SAFEBUFFERS JKJ_CONSTEXPR20 return_type
                operator()(IntervalTypeProvider, Args... args) noexcept {
                    return to_decimal_impl<Float, FloatTraits, PolicyHolder, IntervalTypeProvider>(
                        args...);
                }
            };
        }


        ////////////////////////////////////////////////////////////////////////////////////////
        // The interface function.
        ////////////////////////////////////////////////////////////////////////////////////////

        template <class Float, class FloatTraits = default_float_traits<Float>, class... Policies>
        JKJ_FORCEINLINE
            JKJ_SAFEBUFFERS JKJ_CONSTEXPR20 detail::to_decimal_return_type<FloatTraits, Policies...>
            to_decimal(signed_significand_bits<Float, FloatTraits> signed_significand_bits,
                       unsigned int exponent_bits, Policies...) noexcept {
            // Build policy holder type.
            using namespace detail::policy_impl;
            using policy_holder = detail::to_decimal_policy_holder<Policies...>;

            return policy_holder::delegate(
                signed_significand_bits,
                detail::to_decimal_dispatcher<Float, FloatTraits, policy_holder>{},
                signed_significand_bits, exponent_bits);
        }

        template <class Float, class FloatTraits = default_float_traits<Float>, class... Policies>
        JKJ_FORCEINLINE
            JKJ_SAFEBUFFERS JKJ_CONSTEXPR20 detail::to_decimal_return_type<FloatTraits, Policies...>
            to_decimal(Float x, Policies... policies) noexcept {
            auto const br = float_bits<Float, FloatTraits>(x);
            auto const exponent_bits = br.extract_exponent_bits();
            auto const s = br.remove_exponent_bits(exponent_bits);
            assert(br.is_finite());

            return to_decimal<Float, FloatTraits>(s, exponent_bits, policies...);
        }
    }
}

#undef JKJ_HAS_BUILTIN
#undef JKJ_FORCEINLINE
#undef JKJ_SAFEBUFFERS
#undef JKJ_CONSTEXPR20
#undef JKJ_CAN_BRANCH_ON_CONSTEVAL
#undef JKJ_IF_NOT_CONSTEVAL
#undef JKJ_IF_CONSTEVAL
#undef JKJ_HAS_BIT_CAST
#undef JKJ_IF_CONSTEXPR
#undef JKJ_HAS_IF_CONSTEXPR
#undef JKJ_INLINE_VARIABLE
#undef JKJ_HAS_INLINE_VARIABLE
#undef JKJ_HAS_CONSTEXPR17
#undef JKJ_CONSTEXPR14
#undef JKJ_HAS_CONSTEXPR14

#endif
