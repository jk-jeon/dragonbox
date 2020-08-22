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

#include "../fp_to_chars.h"

template <class Float>
jkj::signed_fp_t<Float> decompose_float(Float x) {
	using common_info = jkj::dragonbox_detail::common_info<Float>;
	jkj::signed_fp_t<Float> ret_value;

	std::memcpy(&ret_value.significand, &x, sizeof(Float));

	ret_value.is_negative = (ret_value.significand & common_info::sign_bit_mask) != 0;
	ret_value.exponent = ((ret_value.significand << 1) >> (common_info::precision + 1));
	ret_value.significand <<= (common_info::extended_precision -
		common_info::precision - 1);

	// Deal with normal/subnormal dichotomy
	ret_value.significand |= (ret_value.exponent == 0 ? 0 : common_info::sign_bit_mask);

	ret_value.exponent += (common_info::exponent_bias - common_info::extended_precision + 1);

	// x should be a finite number
	assert(ret_value.exponent != 1 - common_info::exponent_bias);

	return ret_value;
}

#include <iostream>
#include <iomanip>
#include <string>

template <class Float>
void live_test()
{
	char buffer[41];

	while (true) {
		Float x;
		std::string x_str;
		while (true) {
			std::getline(std::cin, x_str);
			try {
				if constexpr (sizeof(Float) == 4) {
					x = std::stof(x_str);
				}
				else {
					x = std::stod(x_str);
				}
			}
			catch (...) {
				std::cout << "Not a valid input; input again.\n";
				continue;
			}
			break;
		}

		auto xx = decompose_float(x);
		std::cout << "              sign: " << (xx.is_negative ? "-" : "+") << std::endl;
		std::cout << "          exponent: " << xx.exponent << std::endl;
		std::cout << "       significand: " << "0x" << std::hex << std::setfill('0');
		if constexpr (sizeof(Float) == 4) {
			std::cout << std::setw(8);
		}
		else {
			std::cout << std::setw(16);
		}
		std::cout << xx.significand << std::dec << std::endl;

		jkj::fp_to_chars(x, buffer);
		std::cout << " Dragonbox output: " << buffer << std::endl;
	}
}

void live_test_float() {
	std::cout << "[Start live test for float's]\n";
	live_test<float>();
}
void live_test_double() {
	std::cout << "[Start live test for double's]\n";
	live_test<double>();
}