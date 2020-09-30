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

#include "dragonbox/dragonbox.h"

#include <iostream>

template <class Float>
static bool verify_fast_multiplication_xz()
{
	using impl = jkj::dragonbox::detail::impl<Float>;
	using carrier_uint = typename impl::carrier_uint;

	constexpr auto fl = (carrier_uint(1) << (impl::significand_bits + 2)) - 1;
	constexpr auto fr = (carrier_uint(1) << (impl::significand_bits + 2)) + 2;

	using jkj::dragonbox::detail::log::floor_log10_pow2_minus_log10_4_over_3;
	using jkj::dragonbox::detail::log::floor_log2_pow10;

	bool success = true;

	for (int e = impl::min_exponent + 1; e <= impl::max_exponent; ++e) {
		int const exponent = e - impl::significand_bits;

		// Compute k and beta
		int const minus_k = floor_log10_pow2_minus_log10_4_over_3(exponent);
		int const beta_minus_1 = exponent + floor_log2_pow10(-minus_k);
		int const beta_minus_2 = beta_minus_1 - 1;

		// Load cache
		auto const cache = jkj::dragonbox::policy::cache::normal.get_cache<impl::format>(-minus_k);

		// Compute the endpoints using the fast method
		auto x_fast = impl::compute_left_endpoint_for_shorter_interval_case(cache, beta_minus_1);
		auto z_fast = impl::compute_right_endpoint_for_shorter_interval_case(cache, beta_minus_1);

		// Compute the endpoints using the exact method
		auto x_exact = beta_minus_2 >= 0 ?
			impl::compute_mul(fl << beta_minus_2, cache) :
			impl::compute_mul(fl, cache) >> -beta_minus_2;
		auto z_exact = beta_minus_2 >= 0 ?
			impl::compute_mul(fr << beta_minus_2, cache) :
			impl::compute_mul(fr, cache) >> -beta_minus_2;

		if (x_fast != x_exact) {
			std::cout << "(e = " << e << ") left endpoint is not correct; computed = "
				<< x_fast << "; true_value = " << x_exact << "\n";
			success = false;
		}
		if (z_fast != z_exact) {
			std::cout << "(e = " << e << ") right endpoint is not correct; computed = "
				<< z_fast << "; true_value = " << z_exact << "\n";
			success = false;
		}
	}

	if (success) {
		std::cout << "All cases are verified.\n";
	}
	else {
		std::cout << "Error detected.\n";
	}

	return success;
}

template <class Float>
static bool verify_fast_multiplication_yru()
{
	using impl = jkj::dragonbox::detail::impl<Float>;
	bool success = true;

	for (int k = impl::min_k; k < 0; ++k) {
		auto const cache = jkj::dragonbox::policy::cache::normal.get_cache<impl::format>(k);

		// Since p + beta <= q, suffices to check that the lower half of the cache is not 0
		auto const lower_half = [cache] {
			if constexpr (impl::format == jkj::dragonbox::ieee754_format::binary32)
			{
				return std::uint32_t(cache);
			}
			else
			{
				return cache.low();
			}
		}();

		if (lower_half == 0) {
			std::cout << "(k = " << k << ") computation might be incorrect\n";
			success = false;
		}
	}

	if (success) {
		std::cout << "All cases are verified.\n";
	}
	else {
		std::cout << "Error detected.\n";
	}

	return success;
}

int main()
{
	bool success = true;

	std::cout << "[Verifying fast computation of xi and zi for the closer boundary case (binary32)...]\n";
	success &= verify_fast_multiplication_xz<float>();
	std::cout << "Done.\n\n\n";

	std::cout << "[Verifying fast computation of yru for the closer boundary case (binary32)...]\n";
	success &= verify_fast_multiplication_yru<float>();
	std::cout << "Done.\n\n\n";

	std::cout << "[Verifying fast computation of xi and zi for the closer boundary case (binary64)...]\n";
	success &= verify_fast_multiplication_xz<double>();
	std::cout << "Done.\n\n\n";

	std::cout << "[Verifying fast computation of yru for the closer boundary case (binary64)...]\n";
	success &= verify_fast_multiplication_yru<double>();
	std::cout << "Done.\n\n\n";

	if (!success) {
		return -1;
	}
}
