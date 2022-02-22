// Copyright 2020-2022 Junekey Jeon
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


#include "dragonbox/dragonbox_to_chars.h"

#if defined(__GNUC__) || defined(__clang__)
    #define JKJ_FORCEINLINE inline __attribute__((always_inline))
#elif defined(_MSC_VER)
    #define JKJ_FORCEINLINE __forceinline
#else
    #define JKJ_FORCEINLINE inline
#endif

namespace jkj::dragonbox {
    namespace to_chars_detail {
        // These "//"'s are to prevent clang-format to ruin this nice alignment.
        // Thanks to reddit user u/mcmcc:
        // https://www.reddit.com/r/cpp/comments/so3wx9/dragonbox_110_is_released_a_fast_floattostring/hw8z26r/?context=3
        static constexpr char radix_100_table[] = {
            '0', '0', '0', '1', '0', '2', '0', '3', '0', '4', //
            '0', '5', '0', '6', '0', '7', '0', '8', '0', '9', //
            '1', '0', '1', '1', '1', '2', '1', '3', '1', '4', //
            '1', '5', '1', '6', '1', '7', '1', '8', '1', '9', //
            '2', '0', '2', '1', '2', '2', '2', '3', '2', '4', //
            '2', '5', '2', '6', '2', '7', '2', '8', '2', '9', //
            '3', '0', '3', '1', '3', '2', '3', '3', '3', '4', //
            '3', '5', '3', '6', '3', '7', '3', '8', '3', '9', //
            '4', '0', '4', '1', '4', '2', '4', '3', '4', '4', //
            '4', '5', '4', '6', '4', '7', '4', '8', '4', '9', //
            '5', '0', '5', '1', '5', '2', '5', '3', '5', '4', //
            '5', '5', '5', '6', '5', '7', '5', '8', '5', '9', //
            '6', '0', '6', '1', '6', '2', '6', '3', '6', '4', //
            '6', '5', '6', '6', '6', '7', '6', '8', '6', '9', //
            '7', '0', '7', '1', '7', '2', '7', '3', '7', '4', //
            '7', '5', '7', '6', '7', '7', '7', '8', '7', '9', //
            '8', '0', '8', '1', '8', '2', '8', '3', '8', '4', //
            '8', '5', '8', '6', '8', '7', '8', '8', '8', '9', //
            '9', '0', '9', '1', '9', '2', '9', '3', '9', '4', //
            '9', '5', '9', '6', '9', '7', '9', '8', '9', '9'  //
        };

        // These digit generation routines are inspired by James Anhalt's itoa algorithm:
        // https://github.com/jeaiii/itoa
        // The main idea is for given n, find y such that floor(10^k * y / 2^32) = n holds,
        // where k is an appropriate integer depending on the length of n.
        // For example, if n = 1234567, we set k = 6. In this case, we have
        // floor(y / 2^32) = 1,
        // floor(10^2 * ((10^0 * y) mod 2^32) / 2^32) = 23,
        // floor(10^2 * ((10^2 * y) mod 2^32) / 2^32) = 45, and
        // floor(10^2 * ((10^4 * y) mod 2^32) / 2^32) = 67.
        // See https://jk-jeon.github.io/posts/2022/02/jeaiii-algorithm/ for more explanation.

        JKJ_FORCEINLINE static void print_9_digits(std::uint32_t s32, int& exponent,
                                                   char*& buffer) noexcept {
            if (s32 < 100) {
                if (s32 < 10) {
                    // 1 digit.
                    buffer[0] = char('0' + s32);
                    buffer += 1;
                }
                else {
                    // 2 digits.
                    buffer[0] = radix_100_table[int(s32) * 2];
                    buffer[1] = '.';
                    buffer[2] = radix_100_table[int(s32) * 2 + 1];
                    buffer += 3;
                    exponent += 1;
                }
            }
            else {
                if (s32 < 100'0000) {
                    if (s32 < 1'0000) {
                        // 3 or 4 digits.
                        // 42949673 = ceil(2^32 / 100)
                        auto prod = s32 * std::uint64_t(42949673);
                        auto two_digits = int(prod >> 32);

                        // 3 digits.
                        if (two_digits < 10) {
                            buffer[0] = char(two_digits + '0');
                            buffer[1] = '.';
                            exponent += 2;
                            prod = std::uint32_t(prod) * std::uint64_t(100);
                            std::memcpy(buffer + 2, radix_100_table + int(prod >> 32) * 2, 2);
                            buffer += 4;
                        }
                        // 4 digits.
                        else {
                            buffer[0] = radix_100_table[two_digits * 2];
                            buffer[1] = '.';
                            buffer[2] = radix_100_table[two_digits * 2 + 1];
                            exponent += 3;
                            prod = std::uint32_t(prod) * std::uint64_t(100);
                            std::memcpy(buffer + 3, radix_100_table + int(prod >> 32) * 2, 2);
                            buffer += 5;
                        }
                    }
                    else {
                        // 5 or 6 digits.
                        // 429497 = ceil(2^32 / 1'0000)
                        auto prod = s32 * std::uint64_t(429497);
                        auto two_digits = int(prod >> 32);

                        // 5 digits.
                        if (two_digits < 10) {
                            buffer[0] = char(two_digits + '0');
                            buffer[1] = '.';
                            exponent += 4;
                            prod = std::uint32_t(prod) * std::uint64_t(100);
                            std::memcpy(buffer + 2, radix_100_table + int(prod >> 32) * 2, 2);
                            prod = std::uint32_t(prod) * std::uint64_t(100);
                            std::memcpy(buffer + 4, radix_100_table + int(prod >> 32) * 2, 2);
                            buffer += 6;
                        }
                        // 6 digits.
                        else {
                            buffer[0] = radix_100_table[two_digits * 2];
                            buffer[1] = '.';
                            buffer[2] = radix_100_table[two_digits * 2 + 1];
                            exponent += 5;
                            prod = std::uint32_t(prod) * std::uint64_t(100);
                            std::memcpy(buffer + 3, radix_100_table + int(prod >> 32) * 2, 2);
                            prod = std::uint32_t(prod) * std::uint64_t(100);
                            std::memcpy(buffer + 5, radix_100_table + int(prod >> 32) * 2, 2);
                            buffer += 7;
                        }
                    }
                }
                else {
                    if (s32 < 1'0000'0000) {
                        // 7 or 8 digits.
                        // 281474978 = ceil(2^48 / 100'0000) + 1
                        auto prod = s32 * std::uint64_t(281474978);
                        prod >>= 16;
                        auto two_digits = int(prod >> 32);

                        // 7 digits.
                        if (two_digits < 10) {
                            buffer[0] = char(two_digits + '0');
                            buffer[1] = '.';
                            exponent += 6;
                            prod = std::uint32_t(prod) * std::uint64_t(100);
                            std::memcpy(buffer + 2, radix_100_table + int(prod >> 32) * 2, 2);
                            prod = std::uint32_t(prod) * std::uint64_t(100);
                            std::memcpy(buffer + 4, radix_100_table + int(prod >> 32) * 2, 2);
                            prod = std::uint32_t(prod) * std::uint64_t(100);
                            std::memcpy(buffer + 6, radix_100_table + int(prod >> 32) * 2, 2);
                            buffer += 8;
                        }
                        // 8 digits.
                        else {
                            buffer[0] = radix_100_table[two_digits * 2];
                            buffer[1] = '.';
                            buffer[2] = radix_100_table[two_digits * 2 + 1];
                            exponent += 7;
                            prod = std::uint32_t(prod) * std::uint64_t(100);
                            std::memcpy(buffer + 3, radix_100_table + int(prod >> 32) * 2, 2);
                            prod = std::uint32_t(prod) * std::uint64_t(100);
                            std::memcpy(buffer + 5, radix_100_table + int(prod >> 32) * 2, 2);
                            prod = std::uint32_t(prod) * std::uint64_t(100);
                            std::memcpy(buffer + 7, radix_100_table + int(prod >> 32) * 2, 2);
                            buffer += 9;
                        }
                    }
                    else {
                        // 9 digits.
                        // 1441151882 = ceil(2^57 / 1'0000'0000) + 1
                        auto prod = s32 * std::uint64_t(1441151882);
                        prod >>= 25;
                        buffer[0] = char(int(prod >> 32) + '0');
                        buffer[1] = '.';
                        exponent += 8;

                        prod = std::uint32_t(prod) * std::uint64_t(100);
                        std::memcpy(buffer + 2, radix_100_table + int(prod >> 32) * 2, 2);
                        prod = std::uint32_t(prod) * std::uint64_t(100);
                        std::memcpy(buffer + 4, radix_100_table + int(prod >> 32) * 2, 2);
                        prod = std::uint32_t(prod) * std::uint64_t(100);
                        std::memcpy(buffer + 6, radix_100_table + int(prod >> 32) * 2, 2);
                        prod = std::uint32_t(prod) * std::uint64_t(100);
                        std::memcpy(buffer + 8, radix_100_table + int(prod >> 32) * 2, 2);
                        buffer += 10;
                    }
                }
            }
        }

        template <>
        char* to_chars<float, default_float_traits<float>>(std::uint32_t s32, int exponent,
                                                           char* buffer) noexcept {
            // Print significand.
            print_9_digits(s32, exponent, buffer);

            // Print exponent and return
            if (exponent < 0) {
                std::memcpy(buffer, "E-", 2);
                buffer += 2;
                exponent = -exponent;
            }
            else {
                buffer[0] = 'E';
                buffer += 1;
            }

            if (exponent >= 10) {
                std::memcpy(buffer, &radix_100_table[exponent * 2], 2);
                buffer += 2;
            }
            else {
                buffer[0] = (char)('0' + exponent);
                buffer += 1;
            }

            return buffer;
        }

        template <>
        char* to_chars<double, default_float_traits<double>>(std::uint64_t const significand,
                                                             int exponent, char* buffer) noexcept {
            std::uint32_t first_block, second_block;
            bool have_second_block;

            if (significand < 10'0000'0000) {
                first_block = std::uint32_t(significand);
                have_second_block = false;
            }
            else {
                first_block = std::uint32_t(significand / 1'0000'0000);
                second_block = std::uint32_t(significand) - first_block * 1'0000'0000;
                have_second_block = true;
            }

            // Print the first block of significand.
            print_9_digits(first_block, exponent, buffer);

            // Print second block if necessary.
            if (have_second_block) {
                // 281474978 = ceil(2^48 / 100'0000) + 1
                auto prod = second_block * std::uint64_t(281474978);
                prod >>= 16;
                prod += 1;
                exponent += 8;

                std::memcpy(buffer + 0, radix_100_table + int(prod >> 32) * 2, 2);
                prod = std::uint32_t(prod) * std::uint64_t(100);
                std::memcpy(buffer + 2, radix_100_table + int(prod >> 32) * 2, 2);
                prod = std::uint32_t(prod) * std::uint64_t(100);
                std::memcpy(buffer + 4, radix_100_table + int(prod >> 32) * 2, 2);
                prod = std::uint32_t(prod) * std::uint64_t(100);
                std::memcpy(buffer + 6, radix_100_table + int(prod >> 32) * 2, 2);
                buffer += 8;
            }

            // Print exponent and return
            if (exponent < 0) {
                std::memcpy(buffer, "E-", 2);
                buffer += 2;
                exponent = -exponent;
            }
            else {
                buffer[0] = 'E';
                buffer += 1;
            }

            if (exponent >= 100) {
                // d1 = exponent / 10; d2 = exponent % 10;
                // 6554 = ceil(2^16 / 10)
                auto prod = std::uint32_t(exponent) * std::uint32_t(6554);
                auto d1 = prod >> 16;
                prod = std::uint16_t(prod) * std::uint32_t(5); // * 10
                auto d2 = prod >> 15;                          // >> 16
                std::memcpy(buffer, &radix_100_table[d1 * 2], 2);
                buffer[2] = char('0' + d2);
                buffer += 3;
            }
            else if (exponent >= 10) {
                std::memcpy(buffer, &radix_100_table[exponent * 2], 2);
                buffer += 2;
            }
            else {
                buffer[0] = char('0' + exponent);
                buffer += 1;
            }

            return buffer;
        }
    }
}
