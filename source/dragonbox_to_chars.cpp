// Copyright 2020 Junekey Jeon
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

		JKJ_FORCEINLINE static constexpr std::uint32_t decimal_length(std::uint32_t const v) {
			assert(v < 1000000000);
			if (v >= 100000000) { return 9; }
			if (v >= 10000000) { return 8; }
			if (v >= 1000000) { return 7; }
			if (v >= 100000) { return 6; }
			if (v >= 10000) { return 5; }
			if (v >= 1000) { return 4; }
			if (v >= 100) { return 3; }
			if (v >= 10) { return 2; }
			return 1;
		}

		template <class Float, class FloatTraits>
		char* to_chars(typename FloatTraits::carrier_uint significand, int exponent, char* buffer)
		{
			std::uint32_t s32;
			int significand_length, remaining_significand_length;
			if constexpr (std::is_same_v<typename FloatTraits::format, ieee754_binary64>)
			{
				if ((significand >> 32) != 0) {
					// Since significand is at most 10^17, the quotient is at most 10^9, so
					// it fits inside 32-bit integer
					s32 = std::uint32_t(significand / 1'0000'0000);
					auto r = std::uint32_t(significand) - s32 * 1'0000'0000;

					remaining_significand_length = int(decimal_length(s32));
					significand_length = remaining_significand_length + 8;

					// Print 8 digits
					for (int i = 0; i < 2; ++i) {
#ifdef __clang__ // https://bugs.llvm.org/show_bug.cgi?id=38217
						auto c = r - 1'0000 * (r / 1'0000);
#else
						auto c = r % 1'0000;
#endif
						r /= 1'0000;
						std::memcpy(buffer + significand_length - 4 * i - 1,
							&radix_100_table[(c % 100) * 2], 2);
						std::memcpy(buffer + significand_length - 4 * i - 3,
							&radix_100_table[(c / 100) * 2], 2);
					}
				}
				else {
					s32 = std::uint32_t(significand);
					if (s32 >= 10'0000'0000) {
						significand_length = 10;
					}
					else {
						significand_length = int(decimal_length(s32));
					}
					remaining_significand_length = significand_length;
				}
			}
			else
			{
				s32 = std::uint32_t(significand);
				significand_length = int(decimal_length(s32));
				remaining_significand_length = significand_length;
			}

			while (remaining_significand_length > 4) {
#ifdef __clang__ // https://bugs.llvm.org/show_bug.cgi?id=38217
				auto c = s32 - 1'0000 * (significand / 1'0000);
#else
				auto c = s32 % 1'0000;
#endif
				s32 /= 1'0000;
				std::memcpy(buffer + remaining_significand_length - 1,
					&radix_100_table[(c % 100) * 2], 2);
				std::memcpy(buffer + remaining_significand_length - 3,
					&radix_100_table[(c / 100) * 2], 2);
				remaining_significand_length -= 4;
			}
			if (remaining_significand_length > 2) {
				auto c = s32 % 100;
				s32 /= 100;
				std::memcpy(buffer + remaining_significand_length - 1,
					&radix_100_table[c * 2], 2);
				remaining_significand_length -= 2;
			}
			if (remaining_significand_length > 1) {
				assert(remaining_significand_length == 2);
				buffer[0] = char('0' + (s32 / 10));
				buffer[1] = '.';
				buffer[2] = char('0' + (s32 % 10));
				buffer += (significand_length + 1);
			}
			else {
				buffer[0] = char('0' + s32);
				if (significand_length > 1) {
					buffer[1] = '.';
					buffer += (significand_length + 1);
				}
				else {
					buffer += 1;
				}
			}

			// Print exponent and return
			auto exp = exponent + significand_length - 1;
			if (exp < 0) {
				std::memcpy(buffer, "E-", 2);
				buffer += 2;
				exp = -exp;
			}
			else {
				*buffer = 'E';
				++buffer;
			}

			if constexpr (std::is_same_v<typename FloatTraits::format, ieee754_binary64>)
			{
				if (exp >= 100) {
					std::memcpy(buffer, &radix_100_table[(exp / 10) * 2], 2);
					buffer[2] = (char)('0' + (exp % 10));
					buffer += 3;
				}
				else if (exp >= 10) {
					std::memcpy(buffer, &radix_100_table[exp * 2], 2);
					buffer += 2;
				}
				else {
					*buffer = (char)('0' + exp);
					buffer += 1;
				}
			}
			else
			{
				if (exp >= 10) {
					std::memcpy(buffer, &radix_100_table[exp * 2], 2);
					buffer += 2;
				}
				else {
					*buffer = (char)('0' + exp);
					buffer += 1;
				}
			}

			return buffer;
		}

		template char* to_chars<float, default_float_traits<float>>(std::uint32_t, int, char*);
		template char* to_chars<double, default_float_traits<double>>(std::uint64_t, int, char*);
	}
}
