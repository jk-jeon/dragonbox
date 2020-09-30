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

#include "bigint.h"
#include <algorithm>
#include <vector>

namespace {
	// Min-Max Euclid algorithm
	// Precondition: a, b, N are positive integers
	template <std::size_t array_size>
	struct minmax_euclid_return {
		jkj::dragonbox::detail::bigint_impl<array_size> min;
		jkj::dragonbox::detail::bigint_impl<array_size> max;
		std::uint64_t argmin;
		std::uint64_t argmax;
	};
		
	template <std::size_t array_size>
	minmax_euclid_return<array_size> minmax_euclid(
		jkj::dragonbox::detail::bigint_impl<array_size> const& a,
		jkj::dragonbox::detail::bigint_impl<array_size> const& b,
		std::uint64_t N)
	{
		using bigint_t = jkj::dragonbox::detail::bigint_impl<array_size>;

		minmax_euclid_return<array_size> ret;
		ret.max = b;

		bigint_t ai = a;
		bigint_t bi = b;
		std::uint64_t si = 1;
		std::uint64_t ui = 0;

		while (true) {
			// Update ui and bi
			auto new_b = bi;
			auto qi = new_b.long_division(ai);
			if (new_b == 0) {
				assert(qi > 0);
				--qi;
				new_b = ai;
			}
			auto new_u = qi * si + ui;

			if (new_u > N) {
				// Find 0 < k < qi such that ui + k*si <= N < ui + (k+1)*si
				auto k = (N - ui) / si;

				// si <= N < new_u
				ret.min = ai;
				ret.argmin = si;
				ret.max -= bi;
				ret.max += k * ai;
				ret.argmax = ui + k * si;

				break;
			}
			assert(new_u.leading_one_pos.element_pos == 0);

			// Update si and ai
			auto new_a = ai;
			auto pi = new_a.long_division(new_b);
			if (new_a == 0) {
				assert(pi > 0);
				--pi;
				new_a = new_b;
			}
			auto new_s = pi * new_u + si;

			if (new_s > N) {
				// Find 0 < k < pi such that si + k*u(i+1) <= N < si + (k+1)*u(i+1)
				auto k = (N - si) / new_u.elements[0];

				// new_u <= N < new_s
				ret.min = ai;
				ret.min -= k * new_b;
				ret.argmin = si + k * new_u.elements[0];
				ret.max -= new_b;
				ret.argmax = new_u.elements[0];

				break;
			}
			assert(new_s.leading_one_pos.element_pos == 0);

			if (new_b == bi && new_a == ai) {
				// Reached to the gcd
				assert(ui == new_u.elements[0]);
				assert(si == new_s.elements[0]);

				ret.max -= new_b;
				ret.argmax = new_u.elements[0];

				auto sum_idx = new_s + new_u;
				if (sum_idx > N) {
					ret.min = new_a;
					ret.argmin = new_s.elements[0];
				}
				else {
					assert(sum_idx.leading_one_pos.element_pos == 0);
					ret.min = 0;
					ret.argmin = sum_idx.elements[0];
				}

				break;
			}

			bi = new_b;
			ui = new_u.elements[0];
			ai = new_a;
			si = new_s.elements[0];
		}

		return ret;
	}

	template <class Float, class F1, class F2>
	std::size_t check_negative_k(F1&& on_each, F2&& on_max)
	{
		using namespace jkj::dragonbox::detail;

		// Compute required bits to hold big integers
		// 1. Should be able to hold 2^(E - p - 1 + k)
		//    One can show E - p - 1 + k < (kappa - k + 1)log2(10) + k - 1
		//    and the RHS is a decreasing function of k;
		//    we need floor((kappa - k + 1)log2(10)) + k + 1 bits
		// 2. Should be able to hold 5^-k * (2^(p + 2) - 1)
		//    Enough to have ceil((-k)log2(5)) + p + 3 bits,
		//    which is a decreasing function of k, or equivalently,
		//    floor((-k)log2(10)) + k + p + 4 bits
		constexpr auto max_bits = std::size_t(std::max(
			log::floor_log2_pow10(impl<Float>::kappa - impl<Float>::min_k + 1) + impl<Float>::min_k + 1,
			log::floor_log2_pow10(-impl<Float>::min_k) + impl<Float>::min_k + impl<Float>::significand_bits + 4));

		using bigint_type = bigint<max_bits>;

		constexpr auto range = (std::uint64_t(1) << (impl<Float>::significand_bits + 2)) - 1;
		constexpr int min_exponent = log::floor_log2_pow10(impl<Float>::kappa + 1)
			+ impl<Float>::significand_bits + 1;

		std::size_t max_required_bits = 0;

		bigint_type power_of_5 = 1;
		int prev_k = 0;
		for (int e = min_exponent; e <= impl<Float>::max_exponent; ++e) {
			int k = impl<Float>::kappa - log::floor_log10_pow2(e - impl<Float>::significand_bits);
			assert(k < 0);

			if (k != prev_k) {
				assert(k == prev_k - 1);
				power_of_5.multiply_5();
				prev_k = k;
			}

			assert(k + e - impl<Float>::significand_bits - 1 >= 0);
			auto mod_min = minmax_euclid(
				bigint_type::power_of_2(k + e - impl<Float>::significand_bits - 1),
				power_of_5, range).max;

			auto dividend = power_of_5 * range;
			auto divisor = power_of_5 - mod_min;
			auto quotient = dividend.long_division(divisor);

			auto log2_res_p1 = quotient.leading_one_pos.element_pos *
				quotient.element_number_of_bits + quotient.leading_one_pos.bit_pos;

			auto required_bits = log2_res_p1 + e - impl<Float>::significand_bits +
				log::floor_log2_pow10(k);

			on_each(e - impl<Float>::significand_bits, required_bits);

			if (required_bits > max_required_bits) {
				max_required_bits = required_bits;
			}
		}

		on_max(max_required_bits);
		return max_required_bits;
	}

	template <class Float, class F1, class F2>
	std::size_t check_positive_k(F1&& on_each, F2&& on_max)
	{
		using namespace jkj::dragonbox::detail;

		// Compute required bits to hold big integers
		// 1. Should be able to hold 2^(-k - E + p + 1)
		//    One can show -k - E + p + 1 <= (k - kappa)log2(10) - k + 1
		//    and the RHS is an increasing function of k;
		//    we need floor((k - kappa)log2(10)) - k + 1 bits
		// 2. Should be able to hold 5^k * (2^(p + 2) - 1)
		//    Enough to have ceil(k log2(5)) + p + 3 bits,
		//    which is an increasing function of k, or equivalently,
		//    floor(k log2(10)) - k + p + 4 bits
		constexpr auto max_bits = std::size_t(std::max(
			log::floor_log2_pow10(impl<Float>::max_k - impl<Float>::kappa) - impl<Float>::max_k + 1,
			log::floor_log2_pow10(impl<Float>::max_k) - impl<Float>::max_k + impl<Float>::significand_bits + 4));

		using bigint_type = bigint<max_bits>;

		constexpr auto range = (std::uint64_t(1) << (impl<Float>::significand_bits + 2)) - 1;
		constexpr int max_exponent = log::floor_log2_pow10(impl<Float>::kappa + 1)
			+ impl<Float>::significand_bits;

		std::size_t max_required_bits = 0;

		bigint_type power_of_5 = 1;
		int prev_k = 0;
		for (int e = max_exponent; e >= impl<Float>::min_exponent; --e) {
			int k = impl<Float>::kappa - log::floor_log10_pow2(e - impl<Float>::significand_bits);
			assert(k >= 0);

			if (k != prev_k) {
				assert(k == prev_k + 1);
				power_of_5.multiply_5();
				prev_k = k;
			}

			std::size_t required_bits = 0;
			if (-k - e + impl<Float>::significand_bits + 1 >= 0) {
				auto mod_max = minmax_euclid(
					power_of_5,
					bigint_type::power_of_2(-k - e + impl<Float>::significand_bits + 1), range).min;

				auto dividend = mod_max;
				auto divisor = range;
				auto quotient = dividend.long_division(divisor);

				auto log2_res_p1 = quotient.leading_one_pos.element_pos *
					quotient.element_number_of_bits + quotient.leading_one_pos.bit_pos;

				required_bits = log::floor_log2_pow10(k) - k + 2 - log2_res_p1;
			}
			else {
				required_bits = log::floor_log2_pow10(k) - k + 1;
			}

			on_each(e - impl<Float>::significand_bits, required_bits);

			if (required_bits > max_required_bits) {
				max_required_bits = required_bits;
			}
		}

		on_max(max_required_bits);
		return max_required_bits;
	}

	template <class Float, class F1, class F2>
	std::size_t check_negative_k_nearest_shorter_interval(F1&& on_each, F2&& on_max)
	{
		using namespace jkj::dragonbox::detail;

		// Compute required bits to hold big integers
		// 1. Should be able to hold 2^(E - p - 2 + k) * (2^(p + 2) + 2)
		//    Hence, (E + k + 1) bits should be sufficient
		//    One can show E + k + 1 < (-k)log2(10) + k + log2(10/3) + p + 3
		//    and the RHS is a decreasing function of k, and is strictly less than
		//    floor((-k)log2(10)) + k + p + 5
		// 2. Should be able to hold 5^-k * (2^(p + 2) + 2)
		//    Enough to have ceil((-k)log2(5)) + p + 3 bits,
		//    which is a decreasing function of k, or equivalently,
		//    floor((-k)log2(10)) + k + p + 4 bits
		constexpr auto max_bits = std::size_t(
			log::floor_log2_pow10(-impl<Float>::min_k) + impl<Float>::min_k + impl<Float>::significand_bits + 5);

		using bigint_type = bigint<max_bits>;

		constexpr auto four_fl = (std::uint64_t(1) << (impl<Float>::significand_bits + 2)) - 1;
		constexpr auto four_fr = (std::uint64_t(1) << (impl<Float>::significand_bits + 2)) + 2;
		constexpr int min_exponent = impl<Float>::significand_bits + 4;

		std::size_t max_required_bits = 0;

		bigint_type power_of_5 = 1;
		int prev_k = 0;
		for (int e = min_exponent; e <= impl<Float>::max_exponent; ++e) {
			int k = -log::floor_log10_pow2_minus_log10_4_over_3(e - impl<Float>::significand_bits);
			assert(k < 0);

			if (k != prev_k) {
				assert(k == prev_k - 1);
				power_of_5.multiply_5();
				prev_k = k;
			}

			assert(k + e - impl<Float>::significand_bits - 2 >= 0);

			// Check the left endpoint
			auto remainder = bigint_type::power_of_2(k + e - impl<Float>::significand_bits - 2) * four_fl;
			remainder.long_division(power_of_5);
			auto dividend = power_of_5 * four_fl;
			auto divisor = power_of_5 - remainder;
			auto quotient = dividend.long_division(divisor);

			auto log2_res_p1 = quotient.leading_one_pos.element_pos *
				quotient.element_number_of_bits + quotient.leading_one_pos.bit_pos;

			auto required_bits = log2_res_p1 + e - impl<Float>::significand_bits +
				log::floor_log2_pow10(k) - 1;

			// Check the right endpoint
			remainder = bigint_type::power_of_2(k + e - impl<Float>::significand_bits - 2) * four_fr;
			remainder.long_division(power_of_5);
			dividend = power_of_5 * four_fr;
			divisor = power_of_5 - remainder;
			quotient = dividend.long_division(divisor);

			log2_res_p1 = quotient.leading_one_pos.element_pos *
				quotient.element_number_of_bits + quotient.leading_one_pos.bit_pos;

			if (log2_res_p1 + e - impl<Float>::significand_bits +
				log::floor_log2_pow10(k) - 1 > required_bits)
			{
				required_bits = log2_res_p1 + e - impl<Float>::significand_bits +
					log::floor_log2_pow10(k) - 1;
			}

			on_each(e - impl<Float>::significand_bits, required_bits);

			if (required_bits > max_required_bits) {
				max_required_bits = required_bits;
			}
		}

		on_max(max_required_bits);
		return max_required_bits;
	}

	template <class Float, class F1, class F2>
	std::size_t check_positive_k_nearest_shorter_interval(F1&& on_each, F2&& on_max)
	{
		using namespace jkj::dragonbox::detail;

		// Compute required bits to hold big integers
		// 1. Should be able to hold 2^(-k - E + p + 2)
		//    One can show -k - E + p + 2 <= k log2(10) - k + log2(3)
		//    and the RHS is an increasing function of k;
		//    we need floor(k log2(10)) - k + 2 bits
		// 2. Should be able to hold 5^k * (2^(p + 2) + 2)
		//    Enough to have ceil(k log2(5)) + p + 3 bits,
		//    which is an increasing function of k, or equivalently,
		//    floor(k log2(10)) - k + p + 4 bits
		constexpr auto max_bits = std::size_t(std::max(
			log::floor_log2_pow10(impl<Float>::max_k) - impl<Float>::max_k + 2,
			log::floor_log2_pow10(impl<Float>::max_k) - impl<Float>::max_k + impl<Float>::significand_bits + 4));

		using bigint_type = bigint<max_bits>;

		constexpr auto four_fl = (std::uint64_t(1) << (impl<Float>::significand_bits + 2)) - 1;
		constexpr auto four_fr = (std::uint64_t(1) << (impl<Float>::significand_bits + 2)) + 2;
		constexpr int max_exponent = impl<Float>::significand_bits + 3;

		std::size_t max_required_bits = 0;

		bigint_type power_of_5 = 1;
		int prev_k = 0;
		for (int e = max_exponent; e >= impl<Float>::min_exponent; --e) {
			int k = -log::floor_log10_pow2_minus_log10_4_over_3(e - impl<Float>::significand_bits);
			assert(k >= 0);

			if (k != prev_k) {
				assert(k == prev_k + 1);
				power_of_5.multiply_5();
				prev_k = k;
			}

			std::size_t required_bits = 0;
			if (-k - e + impl<Float>::significand_bits + 2 >= 0) {
				// Check the left endpoint
				auto remainder = power_of_5 * four_fl;
				remainder.long_division(bigint_type::power_of_2(-k - e + impl<Float>::significand_bits + 2));
				auto dividend = remainder;
				auto divisor = four_fl;
				auto quotient = dividend.long_division(divisor);

				auto log2_res_p1 = quotient.leading_one_pos.element_pos *
					quotient.element_number_of_bits + quotient.leading_one_pos.bit_pos;

				required_bits = log::floor_log2_pow10(k) - k + 2 - log2_res_p1;

				// Check the right endpoint
				remainder = power_of_5 * four_fr;
				remainder.long_division(bigint_type::power_of_2(-k - e + impl<Float>::significand_bits + 2));
				dividend = remainder;
				divisor = four_fl;
				quotient = dividend.long_division(divisor);

				log2_res_p1 = quotient.leading_one_pos.element_pos *
					quotient.element_number_of_bits + quotient.leading_one_pos.bit_pos;

				if (log::floor_log2_pow10(k) - k + 2 - log2_res_p1 > required_bits)
				{
					required_bits = log::floor_log2_pow10(k) - k + 2 - log2_res_p1;
				}
			}
			else {
				required_bits = log::floor_log2_pow10(k) - k + 1;
			}

			on_each(e - impl<Float>::significand_bits, required_bits);

			if (required_bits > max_required_bits) {
				max_required_bits = required_bits;
			}
		}

		on_max(max_required_bits);
		return max_required_bits;
	}

	template <class Float, class F1, class F2>
	std::size_t check_negative_k_right_closed_directed_shorter_interval(F1&& on_each, F2&& on_max)
	{
		using namespace jkj::dragonbox::detail;

		// Compute required bits to hold big integers
		// 1. Should be able to hold 2^(E + k)
		//    Hence, (E + k + 1) bits should be sufficient
		//    One can show E + k + 1 < (kappa - k + 1)log2(10) + k + p + 1
		//    and the RHS is a decreasing function of k, so we need at most
		//    floor((kappa - k + 1)log2(10)) + k + p + 2 bits
		// 2. Should be able to hold 5^-k * 2^(p + 1)
		//    Enough to have ceil((-k)log2(5)) + p + 2 bits,
		//    which is a decreasing function of k, or equivalently,
		//    floor((-k)log2(10)) + k + p + 3 bits
		constexpr auto max_bits = std::size_t(std::max(
			log::floor_log2_pow10(impl<Float>::kappa - impl<Float>::min_k + 1)
				+ impl<Float>::min_k + impl<Float>::significand_bits + 2,
			log::floor_log2_pow10(-impl<Float>::min_k) + impl<Float>::min_k + impl<Float>::significand_bits + 3));

		using bigint_type = bigint<max_bits>;

		constexpr auto two_fl = (std::uint64_t(1) << (impl<Float>::significand_bits + 1)) - 1;
		constexpr auto two_fr = (std::uint64_t(1) << (impl<Float>::significand_bits + 1));
		constexpr int min_exponent = log::floor_log2_pow10(impl<Float>::kappa + 1) +
			impl<Float>::significand_bits + 2;

		std::size_t max_required_bits = 0;

		bigint_type power_of_5 = 1;
		int prev_k = 0;
		for (int e = min_exponent; e <= impl<Float>::max_exponent; ++e) {
			int k = -log::floor_log10_pow2(e - impl<Float>::significand_bits - 1) + impl<Float>::kappa;
			assert(k < 0);

			if (k != prev_k) {
				assert(k == prev_k - 1);
				power_of_5.multiply_5();
				prev_k = k;
			}

			assert(k + e - impl<Float>::significand_bits - 1 >= 0);

			// Check the left endpoint
			auto remainder = bigint_type::power_of_2(k + e - impl<Float>::significand_bits - 1) * two_fl;
			remainder.long_division(power_of_5);
			auto dividend = power_of_5 * two_fl;
			auto divisor = power_of_5 - remainder;
			auto quotient = dividend.long_division(divisor);

			auto log2_res_p1 = quotient.leading_one_pos.element_pos *
				quotient.element_number_of_bits + quotient.leading_one_pos.bit_pos;

			auto required_bits = log2_res_p1 + e - impl<Float>::significand_bits +
				log::floor_log2_pow10(k);

			// Check the right endpoint
			remainder = bigint_type::power_of_2(k + e - impl<Float>::significand_bits - 1) * two_fr;
			remainder.long_division(power_of_5);
			dividend = power_of_5 * two_fr;
			divisor = power_of_5 - remainder;
			quotient = dividend.long_division(divisor);

			log2_res_p1 = quotient.leading_one_pos.element_pos *
				quotient.element_number_of_bits + quotient.leading_one_pos.bit_pos;

			if (log2_res_p1 + e - impl<Float>::significand_bits +
				log::floor_log2_pow10(k) > required_bits)
			{
				required_bits = log2_res_p1 + e - impl<Float>::significand_bits +
					log::floor_log2_pow10(k);
			}

			on_each(e - impl<Float>::significand_bits, required_bits);

			if (required_bits > max_required_bits) {
				max_required_bits = required_bits;
			}
		}

		on_max(max_required_bits);
		return max_required_bits;
	}

	template <class Float, class F1, class F2>
	std::size_t check_positive_k_right_closed_directed_shorter_interval(F1&& on_each, F2&& on_max)
	{
		using namespace jkj::dragonbox::detail;

		// Compute required bits to hold big integers
		// 1. Should be able to hold 2^(-k - E + p + 1)
		//    One can show -k - E + p + 1 <= (k - kappa)log2(10) - k
		//    and the RHS is an increasing function of k;
		//    we need floor((k - kappa)log2(10)) - k + 1 bits
		// 2. Should be able to hold 5^k * 2^(p + 1)
		//    Enough to have ceil(k log2(5)) + p + 2 bits,
		//    which is an increasing function of k, or equivalently,
		//    floor(k log2(10)) - k + p + 3 bits
		constexpr auto max_bits = std::size_t(std::max(
			log::floor_log2_pow10(impl<Float>::max_k - impl<Float>::kappa) - impl<Float>::max_k + 1,
			log::floor_log2_pow10(impl<Float>::max_k) - impl<Float>::max_k + impl<Float>::significand_bits + 3));

		using bigint_type = bigint<max_bits>;

		constexpr auto two_fl = (std::uint64_t(1) << (impl<Float>::significand_bits + 1)) - 1;
		constexpr auto two_fr = (std::uint64_t(1) << (impl<Float>::significand_bits + 1));
		constexpr int max_exponent = log::floor_log2_pow10(impl<Float>::kappa + 1) +
			impl<Float>::significand_bits + 1;

		std::size_t max_required_bits = 0;

		bigint_type power_of_5 = 1;
		int prev_k = 0;
		for (int e = max_exponent; e >= impl<Float>::min_exponent; --e) {
			int k = -log::floor_log10_pow2(e - impl<Float>::significand_bits - 1) + impl<Float>::kappa;
			assert(k >= 0);

			if (k != prev_k) {
				assert(k == prev_k + 1);
				power_of_5.multiply_5();
				prev_k = k;
			}

			std::size_t required_bits = 0;
			if (-k - e + impl<Float>::significand_bits + 1 >= 0) {
				// Check the left endpoint
				auto remainder = power_of_5 * two_fl;
				remainder.long_division(bigint_type::power_of_2(-k - e + impl<Float>::significand_bits + 1));
				auto dividend = remainder;
				auto divisor = two_fl;
				auto quotient = dividend.long_division(divisor);

				auto log2_res_p1 = quotient.leading_one_pos.element_pos *
					quotient.element_number_of_bits + quotient.leading_one_pos.bit_pos;

				required_bits = log::floor_log2_pow10(k) - k + 2 - log2_res_p1;

				// Check the right endpoint
				remainder = power_of_5 * two_fr;
				remainder.long_division(bigint_type::power_of_2(-k - e + impl<Float>::significand_bits + 1));
				dividend = remainder;
				divisor = two_fr;
				quotient = dividend.long_division(divisor);

				log2_res_p1 = quotient.leading_one_pos.element_pos *
					quotient.element_number_of_bits + quotient.leading_one_pos.bit_pos;

				if (log::floor_log2_pow10(k) - k + 2 - log2_res_p1 > required_bits)
				{
					required_bits = log::floor_log2_pow10(k) - k + 2 - log2_res_p1;
				}
			}
			else {
				required_bits = log::floor_log2_pow10(k) - k + 1;
			}

			on_each(e - impl<Float>::significand_bits, required_bits);

			if (required_bits > max_required_bits) {
				max_required_bits = required_bits;
			}
		}

		on_max(max_required_bits);
		return max_required_bits;
	}
}

#include <fstream>
#include <iostream>

int main()
{
	std::cout << "[Computing upper bounds on required cache lengths...]\n";

	std::ofstream out;
	auto on_each = [&out](auto e, auto required_bits) {
		out << e << "," << required_bits << std::endl;
	};
	auto on_max = [&](auto max_required_bits) {
		std::cout << "Maximum required bits: " << max_required_bits << std::endl;
	};

	bool success = true;

	std::cout << "\nChecking IEEE-754 binary32 format for negative k...\n";	

	out.open("results/binary32_negative_k.csv");
	out << "e,required_bits\n";
	success &= (check_negative_k<float>(on_each, on_max) <=
		jkj::dragonbox::detail::cache_holder<jkj::dragonbox::ieee754_format::binary32>::cache_bits);
	out.close();

	out.open("results/binary32_negative_k_nearest_shorter_interval.csv");
	out << "e,required_bits\n";
	success &= (check_negative_k_nearest_shorter_interval<float>(on_each, on_max) <=
		jkj::dragonbox::detail::cache_holder<jkj::dragonbox::ieee754_format::binary32>::cache_bits);
	out.close();

	out.open("results/binary32_negative_k_right_closed_directed_shorter_interval.csv");
	out << "e,required_bits\n";
	success &= (check_negative_k_right_closed_directed_shorter_interval<float>(on_each, on_max) <=
		jkj::dragonbox::detail::cache_holder<jkj::dragonbox::ieee754_format::binary32>::cache_bits);
	out.close();


	std::cout << "\nChecking IEEE-754 binary32 format for positive k...\n";

	out.open("results/binary32_positive_k.csv");
	out << "e,required_bits\n";
	success &= (check_positive_k<float>(on_each, on_max) <=
		jkj::dragonbox::detail::cache_holder<jkj::dragonbox::ieee754_format::binary32>::cache_bits);
	out.close();

	out.open("results/binary32_positive_k_nearest_shorter_interval.csv");
	out << "e,required_bits\n";
	success &= (check_positive_k_nearest_shorter_interval<float>(on_each, on_max) <=
		jkj::dragonbox::detail::cache_holder<jkj::dragonbox::ieee754_format::binary32>::cache_bits);
	out.close();

	out.open("results/binary32_positive_k_right_closed_directed_shorter_interval.csv");
	out << "e,required_bits\n";
	success &= (check_positive_k_right_closed_directed_shorter_interval<float>(on_each, on_max) <=
		jkj::dragonbox::detail::cache_holder<jkj::dragonbox::ieee754_format::binary32>::cache_bits);
	out.close();


	std::cout << "\nChecking IEEE-754 binary64 format for negative k...\n";

	out.open("results/binary64_negative_k.csv");
	out << "e,required_bits\n";
	success &= (check_negative_k<double>(on_each, on_max) <=
		jkj::dragonbox::detail::cache_holder<jkj::dragonbox::ieee754_format::binary64>::cache_bits);
	out.close();

	out.open("results/binary64_negative_k_nearest_shorter_interval.csv");
	out << "e,required_bits\n";
	success &= (check_negative_k_nearest_shorter_interval<double>(on_each, on_max) <=
		jkj::dragonbox::detail::cache_holder<jkj::dragonbox::ieee754_format::binary64>::cache_bits);
	out.close();

	out.open("results/binary64_negative_k_right_closed_directed_shorter_interval.csv");
	out << "e,required_bits\n";
	success &= (check_negative_k_right_closed_directed_shorter_interval<double>(on_each, on_max) <=
		jkj::dragonbox::detail::cache_holder<jkj::dragonbox::ieee754_format::binary64>::cache_bits);
	out.close();


	std::cout << "\nChecking IEEE-754 binary64 format for positive k...\n";

	out.open("results/binary64_positive_k.csv");
	out << "e,required_bits\n";
	success &= (check_positive_k<double>(on_each, on_max) <=
		jkj::dragonbox::detail::cache_holder<jkj::dragonbox::ieee754_format::binary64>::cache_bits);
	out.close();

	out.open("results/binary64_positive_k_nearest_shorter_interval.csv");
	out << "e,required_bits\n";
	success &= (check_positive_k_nearest_shorter_interval<double>(on_each, on_max) <=
		jkj::dragonbox::detail::cache_holder<jkj::dragonbox::ieee754_format::binary64>::cache_bits);
	out.close();

	out.open("results/binary64_positive_k_right_closed_directed_shorter_interval.csv");
	out << "e,required_bits\n";
	success &= (check_positive_k_right_closed_directed_shorter_interval<double>(on_each, on_max) <=
		jkj::dragonbox::detail::cache_holder<jkj::dragonbox::ieee754_format::binary64>::cache_bits);
	out.close();

	std::cout << std::endl;
	std::cout << "Done.\n\n\n";

	if (!success) {
		return -1;
	}
}
