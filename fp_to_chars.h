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

#ifndef JKJ_DRAGONBOX_FP_TO_CHARS
#define JKJ_DRAGONBOX_FP_TO_CHARS

#include "dragonbox.h"

namespace jkj {
	namespace fp_to_chars_detail {
		char* to_chars(unsigned_fp_t<float> v, char* buffer);
		char* to_chars(unsigned_fp_t<double> v, char* buffer);
	}

	// Returns the next-to-end position
	template <bool allow_trailing_zeros = false, class Float,
		class RoundingMode = dragonbox_rounding_modes::nearest_to_even,
		class CorrectRoundingSearch = dragonbox_correct_rounding::tie_to_even
	>
	char* fp_to_chars_n(Float x, char* buffer,
		RoundingMode&& rounding_mode = {},
		CorrectRoundingSearch&& crs = {})
	{
		using ieee754_format_info = jkj::ieee754_format_info<jkj::ieee754_traits<Float>::format>;

		auto br = ieee754_bits(x);
		if (br.is_finite()) {
			if (br.is_negative()) {
				*buffer = '-';
				++buffer;
			}
			if (br.is_nonzero()) {
				return fp_to_chars_detail::to_chars(
					dragonbox<false, allow_trailing_zeros>(x,
					std::forward<RoundingMode>(rounding_mode),
					std::forward<CorrectRoundingSearch>(crs)), buffer);
			}
			else {
				std::memcpy(buffer, "0E0", 3);
				return buffer + 3;
			}
		}
		else {
			if ((br.u << (ieee754_format_info::exponent_bits + 1)) != 0)
			{
				std::memcpy(buffer, "NaN", 3);
				return buffer + 3;
			}
			else {
				if (br.is_negative()) {
					*buffer = '-';
					++buffer;
				}
				std::memcpy(buffer, "Infinity", 8);
				return buffer + 8;
			}
		}
	}

	// Null-terminate and bypass the return value of fp_to_chars_n
	template <bool allow_trailing_zeros = false, class Float,
		class RoundingMode = dragonbox_rounding_modes::nearest_to_even,
		class CorrectRoundingSearch = dragonbox_correct_rounding::tie_to_even
	>
	char* fp_to_chars(Float x, char* buffer,
		RoundingMode&& rounding_mode = {},
		CorrectRoundingSearch&& crs = {})
	{
		auto ptr = fp_to_chars_n<allow_trailing_zeros>(x, buffer,
			std::forward<RoundingMode>(rounding_mode),
			std::forward<CorrectRoundingSearch>(crs));
		*ptr = '\0';
		return ptr;
	}
}

#endif