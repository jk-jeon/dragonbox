#include "bigint.h"

#include <functional>
#include <iostream>

int floor_log10_pow2_precise(int e)
{
	using namespace jkj::dragonbox_detail::log_compute;
	constexpr auto c = floor_shift(0, log10_2_fractional_digits, floor_log10_pow2_shift_amount);

	// Compute the maximum possible e
	constexpr auto max_exponent_upper_bound =
		std::uint32_t(std::numeric_limits<std::int32_t>::max() / c);

	// Compute the required number of bits
	constexpr std::size_t required_bits = max_exponent_upper_bound + 1;
	using bigint = jkj::dragonbox_detail::bigint<required_bits>;

	bool is_negative;
	if (e < 0) {
		is_negative = true;
		e = -e;
	}
	else {
		is_negative = false;
	}

	auto power_of_2 = bigint::power_of_2(std::size_t(e));
	auto power_of_10 = bigint(1);
	int k = 0;
	while (power_of_10 <= power_of_2) {
		power_of_10.multiply_5();
		power_of_10.multiply_2();
		++k;
	}

	return is_negative ? -k : k - 1;
}

int floor_log10_pow2_minus_log10_4_over_3_precise(int e)
{
	using namespace jkj::dragonbox_detail::log_compute;
	constexpr auto c = floor_shift(0, log10_2_fractional_digits, floor_log10_pow2_shift_amount);

	// Compute the maximum possible e
	constexpr auto max_exponent_upper_bound =
		std::uint32_t(std::numeric_limits<std::int32_t>::max() / c);

	// Compute the required number of bits
	constexpr std::size_t required_bits = max_exponent_upper_bound + 1;
	using bigint = jkj::dragonbox_detail::bigint<required_bits>;

	e -= 2;

	if (e < 0) {
		e = -e;
		auto power_of_2 = bigint::power_of_2(std::size_t(e));
		auto power_of_10_times_3 = bigint(3);
		int k = 0;
		while (power_of_10_times_3 < power_of_2) {
			power_of_10_times_3.multiply_5();
			power_of_10_times_3.multiply_2();
			++k;
		}
		return -k;
	}
	else {
		auto power_of_2_times_3 = bigint::power_of_2(std::size_t(e)) * 3;
		auto power_of_10 = bigint(1);
		int k = 0;
		while (power_of_10 <= power_of_2_times_3) {
			power_of_10.multiply_5();
			power_of_10.multiply_2();
			++k;
		}
		return k - 1;
	}
}

int floor_log2_pow10_precise(int e)
{
	using namespace jkj::dragonbox_detail::log_compute;
	constexpr auto c = floor_shift(0, log2_10_fractional_digits, floor_log2_pow10_shift_amount);

	// Compute the maximum possible e
	constexpr auto max_exponent_upper_bound =
		std::uint32_t(std::numeric_limits<std::int32_t>::max() / c);

	// Compute the required number of bits
	constexpr std::size_t required_bits = max_exponent_upper_bound + 1;
	using bigint = jkj::dragonbox_detail::bigint<required_bits>;

	bool is_negative;
	if (e < 0) {
		is_negative = true;
		e = -e;
	}
	else {
		is_negative = false;
	}

	auto power_of_10 = bigint(1);
	for (int i = 0; i < e; ++i) {
		power_of_10.multiply_5();
		power_of_10.multiply_2();
	}

	auto k = int(power_of_10.leading_one_pos.element_pos * bigint::element_number_of_bits
		+ power_of_10.leading_one_pos.bit_pos);

	return is_negative ? -k : k - 1;
}

int floor_log5_pow2_precise(int e)
{
	using namespace jkj::dragonbox_detail::log_compute;
	constexpr auto c = floor_shift(0, log10_2_fractional_digits, floor_log10_pow2_shift_amount);

	// Compute the maximum possible e
	constexpr auto max_exponent_upper_bound =
		std::uint32_t(std::numeric_limits<std::int32_t>::max() / c);

	// Compute the required number of bits
	constexpr std::size_t required_bits = max_exponent_upper_bound + 1;
	using bigint = jkj::dragonbox_detail::bigint<required_bits>;

	bool is_negative;
	if (e < 0) {
		is_negative = true;
		e = -e;
	}
	else {
		is_negative = false;
	}

	auto power_of_2 = bigint::power_of_2(std::size_t(e));
	auto power_of_5 = bigint(1);
	int k = 0;
	while (power_of_5 <= power_of_2) {
		power_of_5.multiply_5();
		++k;
	}

	return is_negative ? -k : k - 1;
}

template <
	std::int32_t c_integer_part,
	std::uint64_t c_fractional_digits,
	std::int32_t s_integer_part,
	std::uint64_t s_fractional_digits,
	std::size_t shift_amount
>
static void verify(std::string_view name,
	std::function<int(int)> precise_calculator = nullptr)
{
	// Compute the constants
	using jkj::dragonbox_detail::log_compute::floor_shift;
	constexpr auto c = floor_shift(c_integer_part, c_fractional_digits, shift_amount);
	constexpr auto s = floor_shift(s_integer_part, s_fractional_digits, shift_amount);

	// Compute the maximum possible e
	constexpr auto max_exponent_upper_bound =
		std::uint32_t(std::numeric_limits<std::int32_t>::max() / c);
	static_assert(max_exponent_upper_bound > 1);

	// Compute a conservative upper bound on bits needed for the fractional part
	constexpr int ceil_log2_max_exponent_upper_bound = [] {
		int c = 0;
		std::uint32_t u = 1;
		while (u < max_exponent_upper_bound) {
			u <<= 1;
			++c;
		}
		return c;
	}();

	// Compute the bits for the fractional part
	constexpr auto frac_bits = std::uint32_t(
		((c_fractional_digits << shift_amount)
			>> (64 - ceil_log2_max_exponent_upper_bound)) + 1);

	// To extract the lower bits
	constexpr auto lower_bits_mask = std::uint32_t((std::uint32_t(1) << shift_amount) - 1);

	std::int32_t max_exponent = std::int32_t(max_exponent_upper_bound);
	for (std::uint32_t e = 0; e <= max_exponent_upper_bound; ++e) {
		// Detect overflow
		auto frac_part = std::uint32_t(((frac_bits * e) >> ceil_log2_max_exponent_upper_bound) + 1);
		auto lower_bits = std::uint32_t(lower_bits_mask & (e * c - s));

		if (frac_part + lower_bits >= std::uint32_t(std::uint32_t(1) << shift_amount)) {
			std::cout << name << ": overflow detected at e = " << e << std::endl;

			if (precise_calculator) {
				bool actual_error = false;

				auto true_value = precise_calculator(int(e));
				auto computed_value = int((std::int32_t(e) * c - s) >> shift_amount);
				if (computed_value != true_value) {
					std::cout << "  - actual error with positive e ("
						<< "true value: " << true_value
						<< ", computed value: " << computed_value << ")\n";

					actual_error = true;
				}

				true_value = precise_calculator(-int(e));
				computed_value = int((-std::int32_t(e) * c - s) >> shift_amount);
				if (computed_value != true_value) {
					std::cout << "  - actual error with negative e ("
						<< "true value: " << true_value
						<< ", computed value: " << computed_value << ")\n";

					actual_error = true;
				}

				if (actual_error) {
					max_exponent = std::int32_t(e) - 1;
					break;
				}
				else {
					std::cout << "  - turned out to be okay\n";
				}				
			}
			else {
				max_exponent = std::int32_t(e) - 1;
				break;
			}
		}
	}

	std::cout << name << " is correct up to |e| <= " << max_exponent << std::endl;
}

void verify_log_computation()
{
	using namespace jkj::dragonbox_detail::log_compute;

	std::cout << "[Verifying log computation...]\n";

	verify<
		0, log10_2_fractional_digits,
		0, 0,
		floor_log10_pow2_shift_amount
	>("floor_log10_pow2", floor_log10_pow2_precise);

	verify<
		0, log10_2_fractional_digits,
		0, log10_4_over_3_fractional_digits,
		floor_log10_pow2_shift_amount
	>("floor_log10_pow2_minus_log10_4_over_3", floor_log10_pow2_minus_log10_4_over_3_precise);

	verify<
		3, log2_10_fractional_digits,
		0, 0,
		floor_log2_pow10_shift_amount
	>("floor_log2_pow10", floor_log2_pow10_precise);

	verify<
		0, log5_2_fractional_digits,
		0, 0,
		floor_log5_pow2_shift_amount
	>("floor_log5_pow2", floor_log5_pow2_precise);
}