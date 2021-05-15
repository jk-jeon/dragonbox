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

#ifndef JKJ_DRAGONBOX_TO_CHARS
#define JKJ_DRAGONBOX_TO_CHARS

#include "dragonbox/dragonbox.h"

namespace jkj::dragonbox {
	namespace to_chars_detail {
		char* to_chars(unsigned_fp_t<float> v, char* buffer);
		char* to_chars(unsigned_fp_t<double> v, char* buffer);
	}

	// Returns the next-to-end position
	template <class Float, class... Policies>
	char* to_chars_n(Float x, char* buffer, Policies... policies)
	{
		using namespace jkj::dragonbox::detail::policy_impl;
		using policy_holder = decltype(make_policy_holder(
			base_default_pair_list<
				base_default_pair<trailing_zero::base, trailing_zero::remove>,
				base_default_pair<rounding_mode::base, rounding_mode::nearest_to_even>,
				base_default_pair<correct_rounding::base, correct_rounding::to_even>,
				base_default_pair<cache::base, cache::normal>
			>{}, policies...));

		static_assert(!policy_holder::report_trailing_zeros,
			"jkj::dragonbox::policy::trailing_zeros::report is not valid for to_chars & to_chars_n");

		using ieee754_format_info = ieee754_format_info<ieee754_traits<Float>::format>;

		auto br = ieee754_bits(x);
		if (br.is_finite()) {
			if (br.is_negative()) {
				*buffer = '-';
				++buffer;
			}
			if (br.is_nonzero()) {
				return to_chars_detail::to_chars(to_decimal(x,
					policy::sign::ignore,
					typename policy_holder::trailing_zero_policy{},
					typename policy_holder::rounding_mode_policy{},
					typename policy_holder::correct_rounding_policy{},
					typename policy_holder::cache_policy{}),
					buffer);
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
	template <class Float, class... Policies>
	char* to_chars(Float x, char* buffer, Policies... policies)
	{
		auto ptr = to_chars_n(x, buffer, policies...);
		*ptr = '\0';
		return ptr;
	}

	// Maximum required buffer size
	template <ieee754_format format>
	inline constexpr std::size_t max_output_string_length =
		format == ieee754_format::binary32 ?
		// sign(1) + significand(9) + decimal_point(1) + exp_marker(1) + exp_sign(1) + exp(2)
		(1 + 9 + 1 + 1 + 1 + 2) :
		// format == ieee754_format::binary64
		// sign(1) + significand(17) + decimal_point(1) + exp_marker(1) + exp_sign(1) + exp(3)
		(1 + 17 + 1 + 1 + 1 + 3);
}

#endif

