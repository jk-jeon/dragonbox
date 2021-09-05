// Copyright 2020-2021 Junekey Jeon
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
        // clang-format off
		// Hey clang-format, please don't ruin thes nice alignments!
		static constexpr char radix_100_table[] = {
			'0', '0', '0', '1', '0', '2', '0', '3', '0', '4',
			'0', '5', '0', '6', '0', '7', '0', '8', '0', '9',
			'1', '0', '1', '1', '1', '2', '1', '3', '1', '4',
			'1', '5', '1', '6', '1', '7', '1', '8', '1', '9',
			'2', '0', '2', '1', '2', '2', '2', '3', '2', '4',
			'2', '5', '2', '6', '2', '7', '2', '8', '2', '9',
			'3', '0', '3', '1', '3', '2', '3', '3', '3', '4',
			'3', '5', '3', '6', '3', '7', '3', '8', '3', '9',
			'4', '0', '4', '1', '4', '2', '4', '3', '4', '4',
			'4', '5', '4', '6', '4', '7', '4', '8', '4', '9',
			'5', '0', '5', '1', '5', '2', '5', '3', '5', '4',
			'5', '5', '5', '6', '5', '7', '5', '8', '5', '9',
			'6', '0', '6', '1', '6', '2', '6', '3', '6', '4',
			'6', '5', '6', '6', '6', '7', '6', '8', '6', '9',
			'7', '0', '7', '1', '7', '2', '7', '3', '7', '4',
			'7', '5', '7', '6', '7', '7', '7', '8', '7', '9',
			'8', '0', '8', '1', '8', '2', '8', '3', '8', '4',
			'8', '5', '8', '6', '8', '7', '8', '8', '8', '9',
			'9', '0', '9', '1', '9', '2', '9', '3', '9', '4',
			'9', '5', '9', '6', '9', '7', '9', '8', '9', '9'
		};

		static constexpr std::int8_t trailing_zero_count_table[] = {
			2, 0, 0, 0, 0, 0, 0, 0, 0, 0,
			1, 0, 0, 0, 0, 0, 0, 0, 0, 0,
			1, 0, 0, 0, 0, 0, 0, 0, 0, 0,
			1, 0, 0, 0, 0, 0, 0, 0, 0, 0,
			1, 0, 0, 0, 0, 0, 0, 0, 0, 0,
			1, 0, 0, 0, 0, 0, 0, 0, 0, 0,
			1, 0, 0, 0, 0, 0, 0, 0, 0, 0,
			1, 0, 0, 0, 0, 0, 0, 0, 0, 0,
			1, 0, 0, 0, 0, 0, 0, 0, 0, 0,
			1, 0, 0, 0, 0, 0, 0, 0, 0, 0
		};
        // clang-format on

        JKJ_FORCEINLINE static constexpr std::uint32_t
        decimal_length_minus_1(std::uint32_t const v) {
            assert(v < 1000000000);
            if (v >= 100000000) {
                return 8;
            }
            if (v >= 10000000) {
                return 7;
            }
            if (v >= 1000000) {
                return 6;
            }
            if (v >= 100000) {
                return 5;
            }
            if (v >= 10000) {
                return 4;
            }
            if (v >= 1000) {
                return 3;
            }
            if (v >= 100) {
                return 2;
            }
            if (v >= 10) {
                return 1;
            }
            return 0;
        }

        // Granlund-Montgomery style fast division
        struct quotient_remainder_pair {
            std::uint32_t quotient;
            std::uint32_t remainder;
        };
        template <std::uint32_t divisor, unsigned int max_precision,
                  unsigned int additional_precision>
        static constexpr quotient_remainder_pair fast_div(std::uint32_t n) noexcept {
            static_assert(max_precision > 0 && max_precision <= 32);
            assert(n < std::uint32_t(1u << max_precision));

            constexpr auto left_end = std::uint32_t(
                ((1u << (max_precision + additional_precision)) + divisor - 1) / divisor);
            constexpr auto right_end = std::uint32_t(
                ((1u << additional_precision) * ((1 << max_precision) + 1)) / divisor);

            // Ensures sufficient precision.
            static_assert(left_end <= right_end);
            // Ensures no overflow.
            static_assert(left_end <= std::uint32_t(1u << (32 - max_precision)));

            auto quotient = (n * left_end) >> (max_precision + additional_precision);
            auto remainder = n - divisor * quotient;
            return {quotient, remainder};
        }

        // Assumes no trailing zero.
        template <>
        char* to_chars<float, default_float_traits<float>>(std::uint32_t s32, int exponent,
                                                           char* buffer) noexcept {
            int remaining_digits_minus_1 = int(decimal_length_minus_1(s32));
            exponent += remaining_digits_minus_1;
            int exponent_position = remaining_digits_minus_1 + 2;

            while (remaining_digits_minus_1 >= 4) {
#ifdef __clang__ // https://bugs.llvm.org/show_bug.cgi?id=38217
                auto c = s32 - 1'0000 * (s32 / 1'0000);
#else
                auto c = s32 % 1'0000;
#endif
                s32 /= 1'0000;

                // c1 = c / 100; c2 = c % 100;
                auto [c1, c2] = fast_div<100, 14, 5>(c);

                std::memcpy(buffer + remaining_digits_minus_1, &radix_100_table[c2 * 2], 2);
                std::memcpy(buffer + remaining_digits_minus_1 - 2, &radix_100_table[c1 * 2], 2);
                remaining_digits_minus_1 -= 4;
            }
            if (remaining_digits_minus_1 >= 2) {
                // c1 = s32 / 100; c2 = s32 % 100;
                auto [c1, c2] = fast_div<100, 14, 5>(s32);
                s32 = c1;

                std::memcpy(buffer + remaining_digits_minus_1, &radix_100_table[c2 * 2], 2);
                remaining_digits_minus_1 -= 2;
            }
            if (remaining_digits_minus_1 > 0) {
                assert(remaining_digits_minus_1 == 1);
                // d1 = s32 / 10; d2 = s32 % 10;
                auto [d1, d2] = fast_div<10, 7, 3>(s32);

                buffer[0] = char('0' + d1);
                buffer[1] = '.';
                buffer[2] = char('0' + d2);
                buffer += exponent_position;
            }
            else {
                buffer[0] = char('0' + s32);

                // If the significand is of 1 digit, do not print decimal dot.
                if (exponent_position != 2) {
                    buffer[1] = '.';
                    buffer += exponent_position;
                }
                else {
                    buffer += 1;
                }
            }

            // Print exponent and return
            if (exponent < 0) {
                std::memcpy(buffer, "E-", 2);
                buffer += 2;
                exponent = -exponent;
            }
            else {
                *buffer = 'E';
                buffer += 1;
            }

            if (exponent >= 10) {
                std::memcpy(buffer, &radix_100_table[exponent * 2], 2);
                buffer += 2;
            }
            else {
                *buffer = (char)('0' + exponent);
                buffer += 1;
            }

            return buffer;
        }

        // May have trailing zeros.
        template <>
        char* to_chars<double, default_float_traits<double>>(std::uint64_t const significand,
                                                             int exponent, char* buffer) noexcept {
            std::uint32_t s32;
            int remaining_digits_minus_1;
            int exponent_position;
            bool may_have_more_trailing_zeros = false;

            if ((significand >> 32) != 0) {
                // Since significand is at most 10^17, the quotient is at most 10^9, so
                // it fits inside 32-bit integer
                s32 = std::uint32_t(significand / 1'0000'0000);
                auto r = std::uint32_t(significand) - s32 * 1'0000'0000;

                remaining_digits_minus_1 = int(decimal_length_minus_1(s32)) + 8;
                exponent += remaining_digits_minus_1;
                exponent_position = remaining_digits_minus_1 + 2;

                if (r != 0) {
                    // Print 8 digits
#ifdef __clang__ // https://bugs.llvm.org/show_bug.cgi?id=38217
                    auto c = r - 1'0000 * (r / 1'0000);
#else
                    auto c = r % 1'0000;
#endif
                    r /= 1'0000;

                    // c1 = r / 100; c2 = r % 100;
                    auto [c1, c2] = fast_div<100, 14, 5>(r);
                    // c3 = c / 100; c4 = c % 100;
                    auto [c3, c4] = fast_div<100, 14, 5>(c);

                    auto tz = trailing_zero_count_table[c4];
                    if (tz == 0) {
                        goto print_c4_label;
                    }
                    else if (tz == 1) {
                        std::memcpy(buffer + remaining_digits_minus_1, &radix_100_table[c4 * 2], 1);
                        exponent_position -= 1;
                        goto print_c3_label;
                    }

                    tz = trailing_zero_count_table[c3];
                    if (tz == 0) {
                        exponent_position -= 2;
                        goto print_c3_label;
                    }
                    else if (tz == 1) {
                        std::memcpy(buffer + remaining_digits_minus_1 - 2, &radix_100_table[c3 * 2],
                                    1);
                        exponent_position -= 3;
                        goto print_c2_label;
                    }

                    tz = trailing_zero_count_table[c2];
                    if (tz == 0) {
                        exponent_position -= 4;
                        goto print_c2_label;
                    }
                    else if (tz == 1) {
                        std::memcpy(buffer + remaining_digits_minus_1 - 4, &radix_100_table[c2 * 2],
                                    1);
                        exponent_position -= 5;
                        goto print_c1_label;
                    }

                    tz = trailing_zero_count_table[c1];
                    if (tz == 0) {
                        exponent_position -= 6;
                        goto print_c1_label;
                    }
                    // We assumed r != 0, so c1 cannot be zero in this case.
                    assert(tz == 1);
                    std::memcpy(buffer + remaining_digits_minus_1 - 6, &radix_100_table[c1 * 2], 1);
                    exponent_position -= 7;
                    goto after_print_label;

                print_c4_label:
                    std::memcpy(buffer + remaining_digits_minus_1, &radix_100_table[c4 * 2], 2);

                print_c3_label:
                    std::memcpy(buffer + remaining_digits_minus_1 - 2, &radix_100_table[c3 * 2], 2);

                print_c2_label:
                    std::memcpy(buffer + remaining_digits_minus_1 - 4, &radix_100_table[c2 * 2], 2);

                print_c1_label:
                    std::memcpy(buffer + remaining_digits_minus_1 - 6, &radix_100_table[c1 * 2], 2);

                after_print_label:;
                }      // r != 0
                else { // r == 0
                    exponent_position -= 8;
                    may_have_more_trailing_zeros = true;
                }
                remaining_digits_minus_1 -= 8;
            }
            else {
                s32 = std::uint32_t(significand);
                if (s32 >= 10'0000'0000) {
                    remaining_digits_minus_1 = 9;
                }
                else {
                    remaining_digits_minus_1 = int(decimal_length_minus_1(s32));
                }
                exponent += remaining_digits_minus_1;
                exponent_position = remaining_digits_minus_1 + 2;
                may_have_more_trailing_zeros = true;
            }

            while (remaining_digits_minus_1 >= 4) {
#ifdef __clang__ // https://bugs.llvm.org/show_bug.cgi?id=38217
                auto c = s32 - 1'0000 * (s32 / 1'0000);
#else
                auto c = s32 % 1'0000;
#endif
                s32 /= 1'0000;

                // c1 = c / 100; c2 = c % 100;
                auto [c1, c2] = fast_div<100, 14, 5>(c);

                if (may_have_more_trailing_zeros) {
                    auto tz = trailing_zero_count_table[c2];
                    if (tz == 0) {
                        may_have_more_trailing_zeros = false;
                        goto inside_loop_print_c2_label;
                    }
                    else if (tz == 1) {
                        may_have_more_trailing_zeros = false;
                        exponent_position -= 1;
                        std::memcpy(buffer + remaining_digits_minus_1, &radix_100_table[c2 * 2], 1);
                        goto inside_loop_print_c1_label;
                    }

                    tz = trailing_zero_count_table[c1];
                    if (tz == 0) {
                        may_have_more_trailing_zeros = false;
                        exponent_position -= 2;
                        goto inside_loop_print_c1_label;
                    }
                    else if (tz == 1) {
                        may_have_more_trailing_zeros = false;
                        exponent_position -= 3;
                        std::memcpy(buffer + remaining_digits_minus_1 - 2, &radix_100_table[c1 * 2],
                                    1);
                        goto inside_loop_after_print_label;
                    }
                    exponent_position -= 4;
                    goto inside_loop_after_print_label;
                }

            inside_loop_print_c2_label:
                std::memcpy(buffer + remaining_digits_minus_1, &radix_100_table[c2 * 2], 2);

            inside_loop_print_c1_label:
                std::memcpy(buffer + remaining_digits_minus_1 - 2, &radix_100_table[c1 * 2], 2);

            inside_loop_after_print_label:
                remaining_digits_minus_1 -= 4;
            }
            if (remaining_digits_minus_1 >= 2) {
                // c1 = s32 / 100; c2 = s32 % 100;
                auto [c1, c2] = fast_div<100, 14, 5>(s32);
                s32 = c1;

                if (may_have_more_trailing_zeros) {
                    auto tz = trailing_zero_count_table[c2];
                    exponent_position -= tz;
                    if (tz == 0) {
                        std::memcpy(buffer + remaining_digits_minus_1, &radix_100_table[c2 * 2], 2);
                        may_have_more_trailing_zeros = false;
                    }
                    else if (tz == 1) {
                        std::memcpy(buffer + remaining_digits_minus_1, &radix_100_table[c2 * 2], 1);
                        may_have_more_trailing_zeros = false;
                    }
                }
                else {
                    std::memcpy(buffer + remaining_digits_minus_1, &radix_100_table[c2 * 2], 2);
                }

                remaining_digits_minus_1 -= 2;
            }
            if (remaining_digits_minus_1 > 0) {
                assert(remaining_digits_minus_1 == 1);
                // d1 = s32 / 10; d2 = s32 % 10;
                auto [d1, d2] = fast_div<10, 7, 3>(s32);

                buffer[0] = char('0' + d1);
                if (may_have_more_trailing_zeros && d2 == 0) {
                    buffer += 1;
                }
                else {
                    buffer[1] = '.';
                    buffer[2] = char('0' + d2);
                    buffer += exponent_position;
                }
            }
            else {
                buffer[0] = char('0' + s32);

                if (may_have_more_trailing_zeros) {
                    buffer += 1;
                }
                else {
                    buffer[1] = '.';
                    buffer += exponent_position;
                }
            }

            // Print exponent and return
            if (exponent < 0) {
                std::memcpy(buffer, "E-", 2);
                buffer += 2;
                exponent = -exponent;
            }
            else {
                *buffer = 'E';
                buffer += 1;
            }

            if (exponent >= 100) {
                // d1 = exponent / 10; d2 = exponent % 10;
                auto [d1, d2] = fast_div<10, 10, 3>(std::uint32_t(exponent));
                std::memcpy(buffer, &radix_100_table[d1 * 2], 2);
                buffer[2] = (char)('0' + d2);
                buffer += 3;
            }
            else if (exponent >= 10) {
                std::memcpy(buffer, &radix_100_table[exponent * 2], 2);
                buffer += 2;
            }
            else {
                *buffer = (char)('0' + exponent);
                buffer += 1;
            }

            return buffer;
        }
    }
}
