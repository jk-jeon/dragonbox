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
#include <array>
#include <bitset>
#include <type_traits>

namespace {
	template <class Float>
	struct bitset_to_uint;

	template <>
	struct bitset_to_uint<float> {
		static_assert(sizeof(float)* std::numeric_limits<unsigned char>::digits == 32);

		static std::uint64_t convert(std::bitset<64> const& bs) noexcept
		{
			return static_cast<std::uint64_t>(bs.to_ullong());
		}
	};

	template <>
	struct bitset_to_uint<double> {
		static_assert(sizeof(double)* std::numeric_limits<unsigned char>::digits == 64);

		static jkj::dragonbox::detail::wuint::uint128 convert(std::bitset<128> const& bs) noexcept
		{
			std::uint64_t low = 0;
			for (std::size_t i = 0; i < 64; ++i) {
				low <<= 1;
				low |= std::uint64_t(bs[63 - i]);
			}

			std::uint64_t high = 0;
			for (std::size_t i = 0; i < 64; ++i) {
				high <<= 1;
				high |= std::uint64_t(bs[127 - i]);
			}

			return{ high, low };
		}
	};

	template <std::size_t precision, int min_k, int max_k>
	std::array<std::bitset<precision>, std::size_t(max_k - min_k + 1)> generate_cache_bitset()
	{
		static_assert(max_k + min_k >= 0 && min_k <= 0 && max_k >= 0);
		constexpr auto power_of_5_max_bits =
			std::size_t(jkj::dragonbox::detail::log::floor_log2_pow10(max_k) - max_k + 1);
		using bigint_type = jkj::dragonbox::detail::bigint<power_of_5_max_bits>;
		using element_type = typename bigint_type::element_type;

		auto get_upper_bits = [](bigint_type const& n) {
			std::bitset<precision> upper_bits;

			std::size_t remaining = precision;
			if (n.leading_one_pos.bit_pos >= precision) {
				upper_bits = n.elements[n.leading_one_pos.element_pos] >>
					(n.leading_one_pos.bit_pos - precision);
			}
			else {
				auto mask = element_type(1) << (n.leading_one_pos.bit_pos - 1);

				for (std::size_t idx = precision - 1;
					idx >= precision - n.leading_one_pos.bit_pos; --idx)
				{
					upper_bits[idx] =
						(n.elements[n.leading_one_pos.element_pos] & mask) == 0 ? false : true;
					mask >>= 1;
				}
				remaining -= n.leading_one_pos.bit_pos;

				std::size_t element_idx = n.leading_one_pos.element_pos;
				while (remaining >= bigint_type::element_number_of_bits) {
					if (element_idx == 0) {
						for (std::size_t i = 0; i < remaining; ++i)
							upper_bits.reset(i);
						return upper_bits;
					}
					--element_idx;

					mask = element_type(1) << (bigint_type::element_number_of_bits - 1);
					for (std::size_t idx = remaining - 1;
						idx > remaining - bigint_type::element_number_of_bits; --idx)
					{
						upper_bits[idx] =
							(n.elements[element_idx] & mask) == 0 ? false : true;
						mask >>= 1;
					}
					remaining -= bigint_type::element_number_of_bits;
					upper_bits[remaining] =
						(n.elements[element_idx] & mask) == 0 ? false : true;
				}

				if (element_idx == 0) {
					for (std::size_t i = 0; i < remaining; ++i)
						upper_bits.reset(i);
					return upper_bits;
				}
				--element_idx;

				mask = element_type(1) <<
					(bigint_type::element_number_of_bits - remaining);
				for (std::size_t idx = 0; idx < remaining; ++idx) {
					upper_bits[idx] =
						(n.elements[element_idx] & mask) == 0 ? false : true;
					mask <<= 1;
				}
			}

			return upper_bits;
		};

		using return_type = std::array<std::bitset<precision>, std::size_t(max_k - min_k + 1)>;
		return_type ret;
		auto cache = [&ret](int k) -> std::bitset<precision>& {
			return ret[std::size_t(k - min_k)];
		};

		bigint_type power_of_5 = 1;

		cache(0) = get_upper_bits(power_of_5);

		int k = 1;
		for (; k <= -min_k; ++k) {
			power_of_5.multiply_5();

			// Compute positive power
			// - Compute 5^k
			cache(k) = get_upper_bits(power_of_5);

			// Compute negative power
			// - Again, we can factor out 2^-k part by decrementing the exponent by k
			// - To compute 1/5^k, set d = 1 and repeat the following procedure:
			//   - Find the minimum n >= 0 such that d * 2^n >= 5^k; this means that d/5^k >= 1/2^n,
			//     thus the nth digit of the binary expansion of d/5^k is 1
			//   - Set d = d * 2^n - 5^k; this effectively calculates d/5^k - 1/2^n
			//   - Now we conclude that the next (n-1) digits of the binary expansion of 1/5^k are zero,
			//     while the next digit is one
			//   - Repeat until reaching the maximum precision
			bigint_type dividend = 1;
			dividend.multiply_2_until(power_of_5);
			std::bitset<precision> negative_power_digits = 1;

			std::size_t accumulated_exp = 0;
			while (true) {
				dividend -= power_of_5;
				auto new_exp = dividend.multiply_2_until(power_of_5);

				accumulated_exp += new_exp;
				if (accumulated_exp >= precision) {
					negative_power_digits <<= (precision - 1 - (accumulated_exp - new_exp));
					break;
				}

				negative_power_digits <<= new_exp;
				negative_power_digits.set(0);
			}

			cache(-k) = negative_power_digits;
		}

		// Compute remaining positive powers
		for (; k <= max_k; ++k) {
			power_of_5.multiply_5();
			cache(k) = get_upper_bits(power_of_5);
		}

		return ret;
	}
}

#include <fstream>
#include <iomanip>
#include <iostream>

int main()
{
	std::cout << "[Generating cache...]\n";

	using namespace jkj::dragonbox::detail;
	using jkj::dragonbox::detail::wuint::uint128;

	bool fail = false;
	auto write_file = [&](std::ofstream& out,
		auto type_tag,
		auto const& cache_bitset,
		auto&& ieee_754_type_name_string,
		auto&& detect_overflow_and_increment,		
		auto&& element_printer)
	{
		using float_type = typename decltype(type_tag)::type;

		out << "static constexpr int min_k = " << std::dec << impl<float_type>::min_k << ";\n";
		out << "static constexpr int max_k = " << std::dec << impl<float_type>::max_k << ";\n";
		out << "static constexpr cache_entry_type cache[] = {";
		for (int k = impl<float_type>::min_k; k < 0; ++k) {
			auto idx = std::size_t(k - impl<float_type>::min_k);
			auto value = bitset_to_uint<float_type>::convert(cache_bitset[idx]);

			if (detect_overflow_and_increment(value)) {
				std::cout << "Overflow detected while generating caches for " <<
					ieee_754_type_name_string << "!\n";
				fail = true;
			}
			out << "\n\t";
			element_printer(out, value);
			out << ",";

		}
		for (int k = 0; k < impl<float_type>::max_k; ++k) {
			auto idx = std::size_t(k - impl<float_type>::min_k);

			out << "\n\t";
			element_printer(out, bitset_to_uint<float_type>::convert(cache_bitset[idx]));
			out << ",";
		}
		out << "\n\t";
		element_printer(out, bitset_to_uint<float_type>::convert(cache_bitset.back()));
		out << "\n};";
	};

	std::ofstream out;

	out.open("results/binary32_generated_cache.txt");
	auto binary32_cache_bitset = generate_cache_bitset<
		impl<float>::cache_bits,
		impl<float>::min_k,
		impl<float>::max_k>();
	write_file(out, jkj::dragonbox::ieee754_traits<float>{}, binary32_cache_bitset,
		"binary32",
		[](std::uint64_t& value) {
			++value;
			return value == 0;
		},
		[](std::ofstream& out, std::uint64_t value) {
			out << "0x" << std::hex << std::setw(16) << std::setfill('0') << value;
		});
	out.close();

	out.open("results/binary64_generated_cache.txt");
	auto binary64_cache_bitset = generate_cache_bitset<
		impl<double>::cache_bits,
		impl<double>::min_k,
		impl<double>::max_k>();
	write_file(out, jkj::dragonbox::ieee754_traits<double>{}, binary64_cache_bitset,
		"binary64",
		[](uint128& value) {
			value = uint128(value.high(), value.low() + 1);
			return value.low() == 0;
		},
		[](std::ofstream& out, uint128 const& value) {
			out << "{ 0x" << std::hex << std::setw(16) << std::setfill('0') << value.high()
				<< ", 0x" << std::hex << std::setw(16) << std::setfill('0') << value.low()
				<< " }";
		});
	out.close();

	std::cout << "Done.\n\n\n";

	if (fail) {
		return -1;
	}
}
