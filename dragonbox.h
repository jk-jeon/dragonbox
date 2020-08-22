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


#ifndef JKJ_DRAGONBOX
#define JKJ_DRAGONBOX

#include <bitset>
#include <cassert>
#include <cstdint>
#include <limits>
#include <type_traits>

// Suppress additional buffer overrun check
// I have no idea why MSVC thinks some functions here are vulnerable to the buffer overrun attacks
// No, they aren't.
#if defined(_MSC_VER) && !defined(__clang__)
#define JKJ_SAFEBUFFERS __declspec(safebuffers)
#else
#define JKJ_SAFEBUFFERS
#endif

#if (defined(__GNUC__) || defined(__clang__)) && defined(__x86_64__)
#include <immintrin.h>
#elif defined(_MSC_VER) && defined(_M_X64)
#include <intrin.h>	// this includes immintrin.h as well
#endif

namespace jkj {
	namespace dragonbox_detail {
		////////////////////////////////////////////////////////////////////////////////////////
		// Bit operation intrinsics
		////////////////////////////////////////////////////////////////////////////////////////

		namespace bits {
			template <class UInt>
			inline int countr_zero(UInt n) noexcept {
				static_assert(std::is_unsigned_v<UInt> && sizeof(UInt) <= 8);
				assert(n != 0);
#if (defined(__GNUC__) || defined(__clang__)) && defined(__x86_64__)
#define JKJ_HAS_COUNTR_ZERO_INTRINSIC 1
				if constexpr (std::is_same_v<UInt, unsigned long>) {
					return __builtin_ctzl(n);
				}
				else if constexpr (std::is_same_v<UInt, unsigned long long>) {
					return __builtin_ctzll(n);
				}
				else {
					static_assert(sizeof(UInt) <= sizeof(unsigned int));
					return __builtin_ctz((unsigned int)n);
				}
#elif defined(_MSC_VER) && defined(_M_X64)
#define JKJ_HAS_COUNTR_ZERO_INTRINSIC 1
				if constexpr (std::is_same_v<UInt, unsigned __int64>) {
					return int(_tzcnt_u64(n));
				}
				else {
					static_assert(sizeof(UInt) <= sizeof(unsigned int));
					return int(_tzcnt_u32((unsigned int)n));
				}
#else
#define JKJ_HAS_COUNTR_ZERO_INTRINSIC 0
				int count = int(sizeof(UInt) * 8);

				auto n32 = std::uint32_t(n);
				if constexpr (sizeof(UInt) == 8) {
					if (n == n32) {
						count -= 32;
					}
					else if (n32 == 0) {
						n32 = std::uint32_t(n >> 32);
					}
				}
				if constexpr (sizeof(UInt) >= 4) {
					if ((n32 & 0x0000ffff) != 0) count -= 16;
				}
				if constexpr (sizeof(UInt) >= 2) {
					if ((n32 & 0x00ff00ff) != 0) count -= 8;
				}
				if ((n32 & 0x0f0f0f0f) != 0) count -= 4;
				if ((n32 & 0x33333333) != 0) count -= 2;
				if ((n32 & 0x55555555) != 0) count -= 1;

				return count;
#endif
			}
		}

		////////////////////////////////////////////////////////////////////////////////////////
		// Utilities for 128-bit arithmetic
		////////////////////////////////////////////////////////////////////////////////////////

		namespace wide_uint {
			struct uint128 {
				uint128() = default;

#if (defined(__GNUC__) || defined(__clang__)) && defined(__SIZEOF_INT128__) && defined(__x86_64__)
				unsigned __int128	internal_;

				constexpr uint128(std::uint64_t high, std::uint64_t low) noexcept :
					internal_{ ((unsigned __int128)low) | (((unsigned __int128)high) << 64) } {}

				constexpr uint128(unsigned __int128 u) : internal_{ u } {}

				constexpr std::uint64_t high() const noexcept {
					return std::uint64_t(internal_ >> 64);
				}
				constexpr std::uint64_t low() const noexcept {
					return std::uint64_t(internal_);
				}
#else
				std::uint64_t	high_;
				std::uint64_t	low_;

				constexpr uint128(std::uint64_t high, std::uint64_t low) noexcept :
					high_{ high }, low_{ low } {}

				constexpr std::uint64_t high() const noexcept {
					return high_;
				}
				constexpr std::uint64_t low() const noexcept {
					return low_;
				}
#endif
			};

			// Get 128-bit result of multiplication of two 64-bit unsigned integers
			JKJ_SAFEBUFFERS
				inline uint128 umul128(std::uint64_t x, std::uint64_t y) noexcept {
#if (defined(__GNUC__) || defined(__clang__)) && defined(__SIZEOF_INT128__) && defined(__x86_64__)
				return (unsigned __int128)(x) * (unsigned __int128)(y);
#elif defined(_MSC_VER) && defined(_M_X64)
				uint128 result;
				result.low_ = _umul128(x, y, &result.high_);
				return result;
#else
				constexpr auto mask = (std::uint64_t(1) << 32) - std::uint64_t(1);

				auto a = x >> 32;
				auto b = x & mask;
				auto c = y >> 32;
				auto d = y & mask;

				auto ac = a * c;
				auto bc = b * c;
				auto ad = a * d;
				auto bd = b * d;

				auto intermediate = (bd >> 32) + (ad & mask) + (bc & mask);

				return{ ac + (intermediate >> 32) + (ad >> 32) + (bc >> 32),
					(intermediate << 32) + (bd & mask) };
#endif
			}

			JKJ_SAFEBUFFERS
				inline std::uint64_t umul128_upper64(std::uint64_t x, std::uint64_t y) noexcept {
#if (defined(__GNUC__) || defined(__clang__)) && defined(__SIZEOF_INT128__) && defined(__x86_64__)
				auto p = (unsigned __int128)(x) * (unsigned __int128)(y);
				return std::uint64_t(p >> 64);
#elif defined(_MSC_VER) && defined(_M_X64)
				return __umulh(x, y);
#else
				constexpr auto mask = (std::uint64_t(1) << 32) - std::uint64_t(1);

				auto a = x >> 32;
				auto b = x & mask;
				auto c = y >> 32;
				auto d = y & mask;

				auto ac = a * c;
				auto bc = b * c;
				auto ad = a * d;
				auto bd = b * d;

				auto intermediate = (bd >> 32) + (ad & mask) + (bc & mask);

				return ac + (intermediate >> 32) + (ad >> 32) + (bc >> 32);
#endif
			}

			// Get upper 64-bits of multiplication of a 64-bit unsigned integer and a 128-bit unsigned integer
			JKJ_SAFEBUFFERS
				inline std::uint64_t umul192_upper64(std::uint64_t x, uint128 y) noexcept {
				auto g0 = umul128(x, y.high());
				auto g10 = umul128_upper64(x, y.low());

#if (defined(__GNUC__) || defined(__clang__)) && defined(__SIZEOF_INT128__) && defined(__x86_64__)
				return uint128{ g0.internal_ + g10 }.high();
#elif defined(_MSC_VER) && defined(_M_X64)
				std::uint64_t high, low;
				auto carry = _addcarry_u64(0, g0.low(), g10, &low);
				_addcarry_u64(carry, g0.high(), 0, &high);
				return high;
#else
				auto intermediate = g0.low() + g10;
				return g0.high() + (intermediate < g10);
#endif
			}

			// Get upper 32-bits of multiplication of a 32-bit unsigned integer and a 64-bit unsigned integer
			inline std::uint32_t umul96_upper32(std::uint32_t x, std::uint64_t y) noexcept {
				return std::uint32_t(umul128_upper64(x, y));
			}
		}

		////////////////////////////////////////////////////////////////////////////////////////
		// Utilities for fast and constexpr log computation
		////////////////////////////////////////////////////////////////////////////////////////

		namespace log_compute {
			constexpr std::int32_t floor_shift(
				std::uint32_t integer_part,
				std::uint64_t fractional_digits,
				std::size_t shift_amount)
			{
				assert(shift_amount < 32);
				// Ensure no overflow
				assert(integer_part < (std::uint32_t(1) << (32 - shift_amount)));

				return std::int32_t(
					(integer_part << shift_amount) |
					(fractional_digits >> (64 - shift_amount)));
			}
			
			// Compute floor(e * c - s)
			template <
				std::uint32_t c_integer_part,
				std::uint64_t c_fractional_digits,
				std::uint32_t s_integer_part,
				std::uint64_t s_fractional_digits,
				std::size_t shift_amount,
				std::int32_t max_exponent
			>
			constexpr int compute(int e) noexcept {
				assert(e <= max_exponent && e >= -max_exponent);
				constexpr auto c = floor_shift(c_integer_part, c_fractional_digits, shift_amount);
				constexpr auto s = floor_shift(s_integer_part, s_fractional_digits, shift_amount);
				return int((std::int32_t(e) * c - s) >> shift_amount);
			}

			static constexpr std::uint64_t log10_2_fractional_digits{ 0x4d10'4d42'7de7'fbcc };
			static constexpr std::uint64_t log10_4_over_3_fractional_digits{ 0x1ffb'fc2b'bc78'0375 };
			constexpr std::size_t floor_log10_pow2_shift_amount = 22;

			static constexpr std::uint64_t log2_10_fractional_digits{ 0x5269'e12f'346e'2bf9 };
			constexpr std::size_t floor_log2_pow10_shift_amount = 19;

			static constexpr std::uint64_t log5_2_fractional_digits{ 0x6e40'd1a4'143d'cb94 };
			constexpr std::size_t floor_log5_pow2_shift_amount = 20;
		}

		constexpr int floor_log10_pow2(int e) noexcept {
			using namespace log_compute;
			return compute<
				0, log10_2_fractional_digits,
				0, 0,
				floor_log10_pow2_shift_amount, 1700>(e);
		}

		constexpr int floor_log10_pow2_minus_log10_4_over_3(int e) noexcept {
			using namespace log_compute;
			return compute<
				0, log10_2_fractional_digits,
				0, log10_4_over_3_fractional_digits,
				floor_log10_pow2_shift_amount, 1700>(e);
		}

		constexpr int floor_log2_pow10(int e) noexcept {
			using namespace log_compute;
			return compute<
				3, log2_10_fractional_digits,
				0, 0,
				floor_log2_pow10_shift_amount, 1233>(e);
		}

		constexpr int floor_log5_pow2(int e) noexcept {
			using namespace log_compute;
			return compute<
				0, log5_2_fractional_digits,
				0, 0,
				floor_log5_pow2_shift_amount, 1492>(e);
		}

		////////////////////////////////////////////////////////////////////////////////////////
		// Utilities for fast divisibility test
		////////////////////////////////////////////////////////////////////////////////////////

		namespace divtest {
			template <class UInt, UInt a, int N>
			struct table_t {
				static_assert(std::is_unsigned_v<UInt>);
				static_assert(a % 2 != 0);
				static_assert(N > 0);

				static constexpr int size = N;
				UInt mod_inverses[N];
				UInt max_quotients[N];
			};

			template <class UInt, UInt a, int N>
			struct table_holder {
				static constexpr table_t<UInt, a, N> table = [] {
					// By Euler's theorem, a^phi(2^n) == 1 (mod 2^n),
					// where phi(2^n) = 2^(n-1), so the modular inverse of a is
					// a^(2^(n-1) - 1) = a^(1 + 2 + 2^2 + ... + 2^(n-2))
					std::common_type_t<UInt, unsigned int> mod_inverse = 1;
					constexpr int n = sizeof(UInt) * 8;
					for (int i = 1; i < n; ++i) {
						mod_inverse = mod_inverse * mod_inverse * a;
					}

					// Now, generate the table
					table_t<UInt, a, N> table{};
					std::common_type_t<UInt, unsigned int> pow_of_mod_inverse = 1;
					UInt pow_of_a = 1;
					for (int i = 0; i < N; ++i) {
						table.mod_inverses[i] = UInt(pow_of_mod_inverse);
						table.max_quotients[i] = UInt(std::numeric_limits<UInt>::max() / pow_of_a);

						pow_of_mod_inverse *= mod_inverse;
						pow_of_a *= a;
					}

					return table;
				}();
			};

			template <std::size_t table_size, class UInt>
			constexpr bool divisible_by_power_of_5(UInt x, unsigned int exp) noexcept {
				static_assert(sizeof(UInt) == 4 || sizeof(UInt) == 8);
				auto const& table = table_holder<UInt, 5, table_size>::table;
				if (exp >= (unsigned int)(table.size)) {
					return false;
				}
				return (x * table.mod_inverses[exp]) <= table.max_quotients[exp];
			}

			template <class UInt>
			constexpr bool divisible_by_power_of_2(UInt x, unsigned int exp) noexcept {
				assert(exp >= 1);
				assert(x != 0);
#if JKJ_HAS_COUNTR_ZERO_INTRINSIC
				return bits::countr_zero(x) >= int(exp);
#else
				if (exp >= int(sizeof(UInt) * 8)) {
					return false;
			}
				return x == ((x >> exp) << exp);
#endif
			}
		}

		template <class Float>
		struct common_info {
			using float_type = Float;

			static_assert(std::numeric_limits<Float>::is_iec559&&
				std::numeric_limits<Float>::radix == 2 &&
				(sizeof(Float) == 4 || sizeof(Float) == 8),
				"Dragonbox algorithm only applies to IEEE-754 binary32 and binary64 formats!");

			static constexpr std::size_t precision = std::numeric_limits<Float>::digits - 1;

			using extended_significand_type = std::conditional_t<
				sizeof(Float) == 4,
				std::uint32_t,
				std::uint64_t>;

			static_assert(sizeof(extended_significand_type) == sizeof(Float));

			static constexpr std::size_t extended_precision =
				sizeof(extended_significand_type) * std::numeric_limits<unsigned char>::digits;

			static_assert(extended_precision - precision >= 9);

			static constexpr auto sign_bit_mask =
				(extended_significand_type(1) << (extended_precision - 1));
			static constexpr std::size_t exponent_bits =
				extended_precision - precision - 1;
			static constexpr int exponent_bias = 1 - (1 << (exponent_bits - 1));
			static constexpr auto exponent_bits_mask =
				((extended_significand_type(1) << exponent_bits) - 1) << precision;

			static constexpr auto boundary_bit =
				(extended_significand_type(1) << (extended_precision - precision - 2));
			static constexpr auto normal_interval_length = boundary_bit << 1;
			static constexpr auto closer_boundary_bit = boundary_bit >> 1;

			static constexpr int min_exponent =
				std::numeric_limits<Float>::min_exponent - int(extended_precision);
			static constexpr int max_exponent =
				std::numeric_limits<Float>::max_exponent - int(extended_precision);
			static_assert(min_exponent < 0 && max_exponent > 0 && -min_exponent >= max_exponent);
			static_assert(min_exponent == 1 + exponent_bias - int(extended_precision) + 1);

			static constexpr int min_k = -floor_log10_pow2(
				int(max_exponent + extended_precision - precision - 1)) + 1;
			static constexpr int max_k = -floor_log10_pow2_minus_log10_4_over_3(
					int(min_exponent + extended_precision - precision - 3)) + 1;

			static constexpr std::size_t cache_precision = extended_precision * 2;

			using cache_entry_type = std::conditional_t<
				sizeof(Float) == 4,
				std::uint64_t,
				wide_uint::uint128>;

			static constexpr int decimal_digits = sizeof(Float) == 4 ? 9 : 17;
			static constexpr int max_power_of_factor_of_5 = floor_log5_pow2(int(precision + 2));
			static constexpr int divtest_table_size = (decimal_digits > max_power_of_factor_of_5)
				? decimal_digits : max_power_of_factor_of_5 + 1;
		};

		////////////////////////////////////////////////////////////////////////////////////////
		// Computed cache entries
		////////////////////////////////////////////////////////////////////////////////////////

		template <class Float>
		struct cache_holder;

		template <>
		struct cache_holder<float> {
			static constexpr std::uint64_t cache[] = {
				0xa2425ff75e14fc32,
				0xcad2f7f5359a3b3f,
				0xfd87b5f28300ca0e,
				0x9e74d1b791e07e49,
				0xc612062576589ddb,
				0xf79687aed3eec552,
				0x9abe14cd44753b53,
				0xc16d9a0095928a28,
				0xf1c90080baf72cb2,
				0x971da05074da7bef,
				0xbce5086492111aeb,
				0xec1e4a7db69561a6,
				0x9392ee8e921d5d08,
				0xb877aa3236a4b44a,
				0xe69594bec44de15c,
				0x901d7cf73ab0acda,
				0xb424dc35095cd810,
				0xe12e13424bb40e14,
				0x8cbccc096f5088cc,
				0xafebff0bcb24aaff,
				0xdbe6fecebdedd5bf,
				0x89705f4136b4a598,
				0xabcc77118461cefd,
				0xd6bf94d5e57a42bd,
				0x8637bd05af6c69b6,
				0xa7c5ac471b478424,
				0xd1b71758e219652c,
				0x83126e978d4fdf3c,
				0xa3d70a3d70a3d70b,
				0xcccccccccccccccd,
				0x8000000000000000,
				0xa000000000000000,
				0xc800000000000000,
				0xfa00000000000000,
				0x9c40000000000000,
				0xc350000000000000,
				0xf424000000000000,
				0x9896800000000000,
				0xbebc200000000000,
				0xee6b280000000000,
				0x9502f90000000000,
				0xba43b74000000000,
				0xe8d4a51000000000,
				0x9184e72a00000000,
				0xb5e620f480000000,
				0xe35fa931a0000000,
				0x8e1bc9bf04000000,
				0xb1a2bc2ec5000000,
				0xde0b6b3a76400000,
				0x8ac7230489e80000,
				0xad78ebc5ac620000,
				0xd8d726b7177a8000,
				0x878678326eac9000,
				0xa968163f0a57b400,
				0xd3c21bcecceda100,
				0x84595161401484a0,
				0xa56fa5b99019a5c8,
				0xcecb8f27f4200f3a,
				0x813f3978f8940984,
				0xa18f07d736b90be5,
				0xc9f2c9cd04674ede,
				0xfc6f7c4045812296,
				0x9dc5ada82b70b59d,
				0xc5371912364ce305,
				0xf684df56c3e01bc6,
				0x9a130b963a6c115c,
				0xc097ce7bc90715b3,
				0xf0bdc21abb48db20,
				0x96769950b50d88f4,
				0xbc143fa4e250eb31,
				0xeb194f8e1ae525fd,
				0x92efd1b8d0cf37be,
				0xb7abc627050305ad,
				0xe596b7b0c643c719,
				0x8f7e32ce7bea5c6f,
				0xb35dbf821ae4f38b,
				0xe0352f62a19e306e,
				0x8c213d9da502de45
			};
		};

		template <>
		struct cache_holder<double> {
			static constexpr wide_uint::uint128 cache[] = {
				{ 0x9faacf3df73609b1, 0x77b191618c54e9ad },
				{ 0xc795830d75038c1d, 0xd59df5b9ef6a2418 },
				{ 0xf97ae3d0d2446f25, 0x4b0573286b44ad1e },
				{ 0x9becce62836ac577, 0x4ee367f9430aec33 },
				{ 0xc2e801fb244576d5, 0x229c41f793cda740 },
				{ 0xf3a20279ed56d48a, 0x6b43527578c11110 },
				{ 0x9845418c345644d6, 0x830a13896b78aaaa },
				{ 0xbe5691ef416bd60c, 0x23cc986bc656d554 },
				{ 0xedec366b11c6cb8f, 0x2cbfbe86b7ec8aa9 },
				{ 0x94b3a202eb1c3f39, 0x7bf7d71432f3d6aa },
				{ 0xb9e08a83a5e34f07, 0xdaf5ccd93fb0cc54 },
				{ 0xe858ad248f5c22c9, 0xd1b3400f8f9cff69 },
				{ 0x91376c36d99995be, 0x23100809b9c21fa2 },
				{ 0xb58547448ffffb2d, 0xabd40a0c2832a78b },
				{ 0xe2e69915b3fff9f9, 0x16c90c8f323f516d },
				{ 0x8dd01fad907ffc3b, 0xae3da7d97f6792e4 },
				{ 0xb1442798f49ffb4a, 0x99cd11cfdf41779d },
				{ 0xdd95317f31c7fa1d, 0x40405643d711d584 },
				{ 0x8a7d3eef7f1cfc52, 0x482835ea666b2573 },
				{ 0xad1c8eab5ee43b66, 0xda3243650005eed0 },
				{ 0xd863b256369d4a40, 0x90bed43e40076a83 },
				{ 0x873e4f75e2224e68, 0x5a7744a6e804a292 },
				{ 0xa90de3535aaae202, 0x711515d0a205cb37 },
				{ 0xd3515c2831559a83, 0x0d5a5b44ca873e04 },
				{ 0x8412d9991ed58091, 0xe858790afe9486c3 },
				{ 0xa5178fff668ae0b6, 0x626e974dbe39a873 },
				{ 0xce5d73ff402d98e3, 0xfb0a3d212dc81290 },
				{ 0x80fa687f881c7f8e, 0x7ce66634bc9d0b9a },
				{ 0xa139029f6a239f72, 0x1c1fffc1ebc44e81 },
				{ 0xc987434744ac874e, 0xa327ffb266b56221 },
				{ 0xfbe9141915d7a922, 0x4bf1ff9f0062baa9 },
				{ 0x9d71ac8fada6c9b5, 0x6f773fc3603db4aa },
				{ 0xc4ce17b399107c22, 0xcb550fb4384d21d4 },
				{ 0xf6019da07f549b2b, 0x7e2a53a146606a49 },
				{ 0x99c102844f94e0fb, 0x2eda7444cbfc426e },
				{ 0xc0314325637a1939, 0xfa911155fefb5309 },
				{ 0xf03d93eebc589f88, 0x793555ab7eba27cb },
				{ 0x96267c7535b763b5, 0x4bc1558b2f3458df },
				{ 0xbbb01b9283253ca2, 0x9eb1aaedfb016f17 },
				{ 0xea9c227723ee8bcb, 0x465e15a979c1cadd },
				{ 0x92a1958a7675175f, 0x0bfacd89ec191eca },
				{ 0xb749faed14125d36, 0xcef980ec671f667c },
				{ 0xe51c79a85916f484, 0x82b7e12780e7401b },
				{ 0x8f31cc0937ae58d2, 0xd1b2ecb8b0908811 },
				{ 0xb2fe3f0b8599ef07, 0x861fa7e6dcb4aa16 },
				{ 0xdfbdcece67006ac9, 0x67a791e093e1d49b },
				{ 0x8bd6a141006042bd, 0xe0c8bb2c5c6d24e1 },
				{ 0xaecc49914078536d, 0x58fae9f773886e19 },
				{ 0xda7f5bf590966848, 0xaf39a475506a899f },
				{ 0x888f99797a5e012d, 0x6d8406c952429604 },
				{ 0xaab37fd7d8f58178, 0xc8e5087ba6d33b84 },
				{ 0xd5605fcdcf32e1d6, 0xfb1e4a9a90880a65 },
				{ 0x855c3be0a17fcd26, 0x5cf2eea09a550680 },
				{ 0xa6b34ad8c9dfc06f, 0xf42faa48c0ea481f },
				{ 0xd0601d8efc57b08b, 0xf13b94daf124da27 },
				{ 0x823c12795db6ce57, 0x76c53d08d6b70859 },
				{ 0xa2cb1717b52481ed, 0x54768c4b0c64ca6f },
				{ 0xcb7ddcdda26da268, 0xa9942f5dcf7dfd0a },
				{ 0xfe5d54150b090b02, 0xd3f93b35435d7c4d },
				{ 0x9efa548d26e5a6e1, 0xc47bc5014a1a6db0 },
				{ 0xc6b8e9b0709f109a, 0x359ab6419ca1091c },
				{ 0xf867241c8cc6d4c0, 0xc30163d203c94b63 },
				{ 0x9b407691d7fc44f8, 0x79e0de63425dcf1e },
				{ 0xc21094364dfb5636, 0x985915fc12f542e5 },
				{ 0xf294b943e17a2bc4, 0x3e6f5b7b17b2939e },
				{ 0x979cf3ca6cec5b5a, 0xa705992ceecf9c43 },
				{ 0xbd8430bd08277231, 0x50c6ff782a838354 },
				{ 0xece53cec4a314ebd, 0xa4f8bf5635246429 },
				{ 0x940f4613ae5ed136, 0x871b7795e136be9a },
				{ 0xb913179899f68584, 0x28e2557b59846e40 },
				{ 0xe757dd7ec07426e5, 0x331aeada2fe589d0 },
				{ 0x9096ea6f3848984f, 0x3ff0d2c85def7622 },
				{ 0xb4bca50b065abe63, 0x0fed077a756b53aa },
				{ 0xe1ebce4dc7f16dfb, 0xd3e8495912c62895 },
				{ 0x8d3360f09cf6e4bd, 0x64712dd7abbbd95d },
				{ 0xb080392cc4349dec, 0xbd8d794d96aacfb4 },
				{ 0xdca04777f541c567, 0xecf0d7a0fc5583a1 },
				{ 0x89e42caaf9491b60, 0xf41686c49db57245 },
				{ 0xac5d37d5b79b6239, 0x311c2875c522ced6 },
				{ 0xd77485cb25823ac7, 0x7d633293366b828c },
				{ 0x86a8d39ef77164bc, 0xae5dff9c02033198 },
				{ 0xa8530886b54dbdeb, 0xd9f57f830283fdfd },
				{ 0xd267caa862a12d66, 0xd072df63c324fd7c },
				{ 0x8380dea93da4bc60, 0x4247cb9e59f71e6e },
				{ 0xa46116538d0deb78, 0x52d9be85f074e609 },
				{ 0xcd795be870516656, 0x67902e276c921f8c },
				{ 0x806bd9714632dff6, 0x00ba1cd8a3db53b7 },
				{ 0xa086cfcd97bf97f3, 0x80e8a40eccd228a5 },
				{ 0xc8a883c0fdaf7df0, 0x6122cd128006b2ce },
				{ 0xfad2a4b13d1b5d6c, 0x796b805720085f82 },
				{ 0x9cc3a6eec6311a63, 0xcbe3303674053bb1 },
				{ 0xc3f490aa77bd60fc, 0xbedbfc4411068a9d },
				{ 0xf4f1b4d515acb93b, 0xee92fb5515482d45 },
				{ 0x991711052d8bf3c5, 0x751bdd152d4d1c4b },
				{ 0xbf5cd54678eef0b6, 0xd262d45a78a0635e },
				{ 0xef340a98172aace4, 0x86fb897116c87c35 },
				{ 0x9580869f0e7aac0e, 0xd45d35e6ae3d4da1 },
				{ 0xbae0a846d2195712, 0x8974836059cca10a },
				{ 0xe998d258869facd7, 0x2bd1a438703fc94c },
				{ 0x91ff83775423cc06, 0x7b6306a34627ddd0 },
				{ 0xb67f6455292cbf08, 0x1a3bc84c17b1d543 },
				{ 0xe41f3d6a7377eeca, 0x20caba5f1d9e4a94 },
				{ 0x8e938662882af53e, 0x547eb47b7282ee9d },
				{ 0xb23867fb2a35b28d, 0xe99e619a4f23aa44 },
				{ 0xdec681f9f4c31f31, 0x6405fa00e2ec94d5 },
				{ 0x8b3c113c38f9f37e, 0xde83bc408dd3dd05 },
				{ 0xae0b158b4738705e, 0x9624ab50b148d446 },
				{ 0xd98ddaee19068c76, 0x3badd624dd9b0958 },
				{ 0x87f8a8d4cfa417c9, 0xe54ca5d70a80e5d7 },
				{ 0xa9f6d30a038d1dbc, 0x5e9fcf4ccd211f4d },
				{ 0xd47487cc8470652b, 0x7647c32000696720 },
				{ 0x84c8d4dfd2c63f3b, 0x29ecd9f40041e074 },
				{ 0xa5fb0a17c777cf09, 0xf468107100525891 },
				{ 0xcf79cc9db955c2cc, 0x7182148d4066eeb5 },
				{ 0x81ac1fe293d599bf, 0xc6f14cd848405531 },
				{ 0xa21727db38cb002f, 0xb8ada00e5a506a7d },
				{ 0xca9cf1d206fdc03b, 0xa6d90811f0e4851d },
				{ 0xfd442e4688bd304a, 0x908f4a166d1da664 },
				{ 0x9e4a9cec15763e2e, 0x9a598e4e043287ff },
				{ 0xc5dd44271ad3cdba, 0x40eff1e1853f29fe },
				{ 0xf7549530e188c128, 0xd12bee59e68ef47d },
				{ 0x9a94dd3e8cf578b9, 0x82bb74f8301958cf },
				{ 0xc13a148e3032d6e7, 0xe36a52363c1faf02 },
				{ 0xf18899b1bc3f8ca1, 0xdc44e6c3cb279ac2 },
				{ 0x96f5600f15a7b7e5, 0x29ab103a5ef8c0ba },
				{ 0xbcb2b812db11a5de, 0x7415d448f6b6f0e8 },
				{ 0xebdf661791d60f56, 0x111b495b3464ad22 },
				{ 0x936b9fcebb25c995, 0xcab10dd900beec35 },
				{ 0xb84687c269ef3bfb, 0x3d5d514f40eea743 },
				{ 0xe65829b3046b0afa, 0x0cb4a5a3112a5113 },
				{ 0x8ff71a0fe2c2e6dc, 0x47f0e785eaba72ac },
				{ 0xb3f4e093db73a093, 0x59ed216765690f57 },
				{ 0xe0f218b8d25088b8, 0x306869c13ec3532d },
				{ 0x8c974f7383725573, 0x1e414218c73a13fc },
				{ 0xafbd2350644eeacf, 0xe5d1929ef90898fb },
				{ 0xdbac6c247d62a583, 0xdf45f746b74abf3a },
				{ 0x894bc396ce5da772, 0x6b8bba8c328eb784 },
				{ 0xab9eb47c81f5114f, 0x066ea92f3f326565 },
				{ 0xd686619ba27255a2, 0xc80a537b0efefebe },
				{ 0x8613fd0145877585, 0xbd06742ce95f5f37 },
				{ 0xa798fc4196e952e7, 0x2c48113823b73705 },
				{ 0xd17f3b51fca3a7a0, 0xf75a15862ca504c6 },
				{ 0x82ef85133de648c4, 0x9a984d73dbe722fc },
				{ 0xa3ab66580d5fdaf5, 0xc13e60d0d2e0ebbb },
				{ 0xcc963fee10b7d1b3, 0x318df905079926a9 },
				{ 0xffbbcfe994e5c61f, 0xfdf17746497f7053 },
				{ 0x9fd561f1fd0f9bd3, 0xfeb6ea8bedefa634 },
				{ 0xc7caba6e7c5382c8, 0xfe64a52ee96b8fc1 },
				{ 0xf9bd690a1b68637b, 0x3dfdce7aa3c673b1 },
				{ 0x9c1661a651213e2d, 0x06bea10ca65c084f },
				{ 0xc31bfa0fe5698db8, 0x486e494fcff30a63 },
				{ 0xf3e2f893dec3f126, 0x5a89dba3c3efccfb },
				{ 0x986ddb5c6b3a76b7, 0xf89629465a75e01d },
				{ 0xbe89523386091465, 0xf6bbb397f1135824 },
				{ 0xee2ba6c0678b597f, 0x746aa07ded582e2d },
				{ 0x94db483840b717ef, 0xa8c2a44eb4571cdd },
				{ 0xba121a4650e4ddeb, 0x92f34d62616ce414 },
				{ 0xe896a0d7e51e1566, 0x77b020baf9c81d18 },
				{ 0x915e2486ef32cd60, 0x0ace1474dc1d122f },
				{ 0xb5b5ada8aaff80b8, 0x0d819992132456bb },
				{ 0xe3231912d5bf60e6, 0x10e1fff697ed6c6a },
				{ 0x8df5efabc5979c8f, 0xca8d3ffa1ef463c2 },
				{ 0xb1736b96b6fd83b3, 0xbd308ff8a6b17cb3 },
				{ 0xddd0467c64bce4a0, 0xac7cb3f6d05ddbdf },
				{ 0x8aa22c0dbef60ee4, 0x6bcdf07a423aa96c },
				{ 0xad4ab7112eb3929d, 0x86c16c98d2c953c7 },
				{ 0xd89d64d57a607744, 0xe871c7bf077ba8b8 },
				{ 0x87625f056c7c4a8b, 0x11471cd764ad4973 },
				{ 0xa93af6c6c79b5d2d, 0xd598e40d3dd89bd0 },
				{ 0xd389b47879823479, 0x4aff1d108d4ec2c4 },
				{ 0x843610cb4bf160cb, 0xcedf722a585139bb },
				{ 0xa54394fe1eedb8fe, 0xc2974eb4ee658829 },
				{ 0xce947a3da6a9273e, 0x733d226229feea33 },
				{ 0x811ccc668829b887, 0x0806357d5a3f5260 },
				{ 0xa163ff802a3426a8, 0xca07c2dcb0cf26f8 },
				{ 0xc9bcff6034c13052, 0xfc89b393dd02f0b6 },
				{ 0xfc2c3f3841f17c67, 0xbbac2078d443ace3 },
				{ 0x9d9ba7832936edc0, 0xd54b944b84aa4c0e },
				{ 0xc5029163f384a931, 0x0a9e795e65d4df12 },
				{ 0xf64335bcf065d37d, 0x4d4617b5ff4a16d6 },
				{ 0x99ea0196163fa42e, 0x504bced1bf8e4e46 },
				{ 0xc06481fb9bcf8d39, 0xe45ec2862f71e1d7 },
				{ 0xf07da27a82c37088, 0x5d767327bb4e5a4d },
				{ 0x964e858c91ba2655, 0x3a6a07f8d510f870 },
				{ 0xbbe226efb628afea, 0x890489f70a55368c },
				{ 0xeadab0aba3b2dbe5, 0x2b45ac74ccea842f },
				{ 0x92c8ae6b464fc96f, 0x3b0b8bc90012929e },
				{ 0xb77ada0617e3bbcb, 0x09ce6ebb40173745 },
				{ 0xe55990879ddcaabd, 0xcc420a6a101d0516 },
				{ 0x8f57fa54c2a9eab6, 0x9fa946824a12232e },
				{ 0xb32df8e9f3546564, 0x47939822dc96abfa },
				{ 0xdff9772470297ebd, 0x59787e2b93bc56f8 },
				{ 0x8bfbea76c619ef36, 0x57eb4edb3c55b65b },
				{ 0xaefae51477a06b03, 0xede622920b6b23f2 },
				{ 0xdab99e59958885c4, 0xe95fab368e45ecee },
				{ 0x88b402f7fd75539b, 0x11dbcb0218ebb415 },
				{ 0xaae103b5fcd2a881, 0xd652bdc29f26a11a },
				{ 0xd59944a37c0752a2, 0x4be76d3346f04960 },
				{ 0x857fcae62d8493a5, 0x6f70a4400c562ddc },
				{ 0xa6dfbd9fb8e5b88e, 0xcb4ccd500f6bb953 },
				{ 0xd097ad07a71f26b2, 0x7e2000a41346a7a8 },
				{ 0x825ecc24c873782f, 0x8ed400668c0c28c9 },
				{ 0xa2f67f2dfa90563b, 0x728900802f0f32fb },
				{ 0xcbb41ef979346bca, 0x4f2b40a03ad2ffba },
				{ 0xfea126b7d78186bc, 0xe2f610c84987bfa9 },
				{ 0x9f24b832e6b0f436, 0x0dd9ca7d2df4d7ca },
				{ 0xc6ede63fa05d3143, 0x91503d1c79720dbc },
				{ 0xf8a95fcf88747d94, 0x75a44c6397ce912b },
				{ 0x9b69dbe1b548ce7c, 0xc986afbe3ee11abb },
				{ 0xc24452da229b021b, 0xfbe85badce996169 },
				{ 0xf2d56790ab41c2a2, 0xfae27299423fb9c4 },
				{ 0x97c560ba6b0919a5, 0xdccd879fc967d41b },
				{ 0xbdb6b8e905cb600f, 0x5400e987bbc1c921 },
				{ 0xed246723473e3813, 0x290123e9aab23b69 },
				{ 0x9436c0760c86e30b, 0xf9a0b6720aaf6522 },
				{ 0xb94470938fa89bce, 0xf808e40e8d5b3e6a },
				{ 0xe7958cb87392c2c2, 0xb60b1d1230b20e05 },
				{ 0x90bd77f3483bb9b9, 0xb1c6f22b5e6f48c3 },
				{ 0xb4ecd5f01a4aa828, 0x1e38aeb6360b1af4 },
				{ 0xe2280b6c20dd5232, 0x25c6da63c38de1b1 },
				{ 0x8d590723948a535f, 0x579c487e5a38ad0f },
				{ 0xb0af48ec79ace837, 0x2d835a9df0c6d852 },
				{ 0xdcdb1b2798182244, 0xf8e431456cf88e66 },
				{ 0x8a08f0f8bf0f156b, 0x1b8e9ecb641b5900 },
				{ 0xac8b2d36eed2dac5, 0xe272467e3d222f40 },
				{ 0xd7adf884aa879177, 0x5b0ed81dcc6abb10 },
				{ 0x86ccbb52ea94baea, 0x98e947129fc2b4ea },
				{ 0xa87fea27a539e9a5, 0x3f2398d747b36225 },
				{ 0xd29fe4b18e88640e, 0x8eec7f0d19a03aae },
				{ 0x83a3eeeef9153e89, 0x1953cf68300424ad },
				{ 0xa48ceaaab75a8e2b, 0x5fa8c3423c052dd8 },
				{ 0xcdb02555653131b6, 0x3792f412cb06794e },
				{ 0x808e17555f3ebf11, 0xe2bbd88bbee40bd1 },
				{ 0xa0b19d2ab70e6ed6, 0x5b6aceaeae9d0ec5 },
				{ 0xc8de047564d20a8b, 0xf245825a5a445276 },
				{ 0xfb158592be068d2e, 0xeed6e2f0f0d56713 },
				{ 0x9ced737bb6c4183d, 0x55464dd69685606c },
				{ 0xc428d05aa4751e4c, 0xaa97e14c3c26b887 },
				{ 0xf53304714d9265df, 0xd53dd99f4b3066a9 },
				{ 0x993fe2c6d07b7fab, 0xe546a8038efe402a },
				{ 0xbf8fdb78849a5f96, 0xde98520472bdd034 },
				{ 0xef73d256a5c0f77c, 0x963e66858f6d4441 },
				{ 0x95a8637627989aad, 0xdde7001379a44aa9 },
				{ 0xbb127c53b17ec159, 0x5560c018580d5d53 },
				{ 0xe9d71b689dde71af, 0xaab8f01e6e10b4a7 },
				{ 0x9226712162ab070d, 0xcab3961304ca70e9 },
				{ 0xb6b00d69bb55c8d1, 0x3d607b97c5fd0d23 },
				{ 0xe45c10c42a2b3b05, 0x8cb89a7db77c506b },
				{ 0x8eb98a7a9a5b04e3, 0x77f3608e92adb243 },
				{ 0xb267ed1940f1c61c, 0x55f038b237591ed4 },
				{ 0xdf01e85f912e37a3, 0x6b6c46dec52f6689 },
				{ 0x8b61313bbabce2c6, 0x2323ac4b3b3da016 },
				{ 0xae397d8aa96c1b77, 0xabec975e0a0d081b },
				{ 0xd9c7dced53c72255, 0x96e7bd358c904a22 },
				{ 0x881cea14545c7575, 0x7e50d64177da2e55 },
				{ 0xaa242499697392d2, 0xdde50bd1d5d0b9ea },
				{ 0xd4ad2dbfc3d07787, 0x955e4ec64b44e865 },
				{ 0x84ec3c97da624ab4, 0xbd5af13bef0b113f },
				{ 0xa6274bbdd0fadd61, 0xecb1ad8aeacdd58f },
				{ 0xcfb11ead453994ba, 0x67de18eda5814af3 },
				{ 0x81ceb32c4b43fcf4, 0x80eacf948770ced8 },
				{ 0xa2425ff75e14fc31, 0xa1258379a94d028e },
				{ 0xcad2f7f5359a3b3e, 0x096ee45813a04331 },
				{ 0xfd87b5f28300ca0d, 0x8bca9d6e188853fd },
				{ 0x9e74d1b791e07e48, 0x775ea264cf55347e },
				{ 0xc612062576589dda, 0x95364afe032a819e },
				{ 0xf79687aed3eec551, 0x3a83ddbd83f52205 },
				{ 0x9abe14cd44753b52, 0xc4926a9672793543 },
				{ 0xc16d9a0095928a27, 0x75b7053c0f178294 },
				{ 0xf1c90080baf72cb1, 0x5324c68b12dd6339 },
				{ 0x971da05074da7bee, 0xd3f6fc16ebca5e04 },
				{ 0xbce5086492111aea, 0x88f4bb1ca6bcf585 },
				{ 0xec1e4a7db69561a5, 0x2b31e9e3d06c32e6 },
				{ 0x9392ee8e921d5d07, 0x3aff322e62439fd0 },
				{ 0xb877aa3236a4b449, 0x09befeb9fad487c3 },
				{ 0xe69594bec44de15b, 0x4c2ebe687989a9b4 },
				{ 0x901d7cf73ab0acd9, 0x0f9d37014bf60a11 },
				{ 0xb424dc35095cd80f, 0x538484c19ef38c95 },
				{ 0xe12e13424bb40e13, 0x2865a5f206b06fba },
				{ 0x8cbccc096f5088cb, 0xf93f87b7442e45d4 },
				{ 0xafebff0bcb24aafe, 0xf78f69a51539d749 },
				{ 0xdbe6fecebdedd5be, 0xb573440e5a884d1c },
				{ 0x89705f4136b4a597, 0x31680a88f8953031 },
				{ 0xabcc77118461cefc, 0xfdc20d2b36ba7c3e },
				{ 0xd6bf94d5e57a42bc, 0x3d32907604691b4d },
				{ 0x8637bd05af6c69b5, 0xa63f9a49c2c1b110 },
				{ 0xa7c5ac471b478423, 0x0fcf80dc33721d54 },
				{ 0xd1b71758e219652b, 0xd3c36113404ea4a9 },
				{ 0x83126e978d4fdf3b, 0x645a1cac083126ea },
				{ 0xa3d70a3d70a3d70a, 0x3d70a3d70a3d70a4 },
				{ 0xcccccccccccccccc, 0xcccccccccccccccd },
				{ 0x8000000000000000, 0x0000000000000000 },
				{ 0xa000000000000000, 0x0000000000000000 },
				{ 0xc800000000000000, 0x0000000000000000 },
				{ 0xfa00000000000000, 0x0000000000000000 },
				{ 0x9c40000000000000, 0x0000000000000000 },
				{ 0xc350000000000000, 0x0000000000000000 },
				{ 0xf424000000000000, 0x0000000000000000 },
				{ 0x9896800000000000, 0x0000000000000000 },
				{ 0xbebc200000000000, 0x0000000000000000 },
				{ 0xee6b280000000000, 0x0000000000000000 },
				{ 0x9502f90000000000, 0x0000000000000000 },
				{ 0xba43b74000000000, 0x0000000000000000 },
				{ 0xe8d4a51000000000, 0x0000000000000000 },
				{ 0x9184e72a00000000, 0x0000000000000000 },
				{ 0xb5e620f480000000, 0x0000000000000000 },
				{ 0xe35fa931a0000000, 0x0000000000000000 },
				{ 0x8e1bc9bf04000000, 0x0000000000000000 },
				{ 0xb1a2bc2ec5000000, 0x0000000000000000 },
				{ 0xde0b6b3a76400000, 0x0000000000000000 },
				{ 0x8ac7230489e80000, 0x0000000000000000 },
				{ 0xad78ebc5ac620000, 0x0000000000000000 },
				{ 0xd8d726b7177a8000, 0x0000000000000000 },
				{ 0x878678326eac9000, 0x0000000000000000 },
				{ 0xa968163f0a57b400, 0x0000000000000000 },
				{ 0xd3c21bcecceda100, 0x0000000000000000 },
				{ 0x84595161401484a0, 0x0000000000000000 },
				{ 0xa56fa5b99019a5c8, 0x0000000000000000 },
				{ 0xcecb8f27f4200f3a, 0x0000000000000000 },
				{ 0x813f3978f8940984, 0x4000000000000000 },
				{ 0xa18f07d736b90be5, 0x5000000000000000 },
				{ 0xc9f2c9cd04674ede, 0xa400000000000000 },
				{ 0xfc6f7c4045812296, 0x4d00000000000000 },
				{ 0x9dc5ada82b70b59d, 0xf020000000000000 },
				{ 0xc5371912364ce305, 0x6c28000000000000 },
				{ 0xf684df56c3e01bc6, 0xc732000000000000 },
				{ 0x9a130b963a6c115c, 0x3c7f400000000000 },
				{ 0xc097ce7bc90715b3, 0x4b9f100000000000 },
				{ 0xf0bdc21abb48db20, 0x1e86d40000000000 },
				{ 0x96769950b50d88f4, 0x1314448000000000 },
				{ 0xbc143fa4e250eb31, 0x17d955a000000000 },
				{ 0xeb194f8e1ae525fd, 0x5dcfab0800000000 },
				{ 0x92efd1b8d0cf37be, 0x5aa1cae500000000 },
				{ 0xb7abc627050305ad, 0xf14a3d9e40000000 },
				{ 0xe596b7b0c643c719, 0x6d9ccd05d0000000 },
				{ 0x8f7e32ce7bea5c6f, 0xe4820023a2000000 },
				{ 0xb35dbf821ae4f38b, 0xdda2802c8a800000 },
				{ 0xe0352f62a19e306e, 0xd50b2037ad200000 },
				{ 0x8c213d9da502de45, 0x4526f422cc340000 },
				{ 0xaf298d050e4395d6, 0x9670b12b7f410000 },
				{ 0xdaf3f04651d47b4c, 0x3c0cdd765f114000 },
				{ 0x88d8762bf324cd0f, 0xa5880a69fb6ac800 },
				{ 0xab0e93b6efee0053, 0x8eea0d047a457a00 },
				{ 0xd5d238a4abe98068, 0x72a4904598d6d880 },
				{ 0x85a36366eb71f041, 0x47a6da2b7f864750 },
				{ 0xa70c3c40a64e6c51, 0x999090b65f67d924 },
				{ 0xd0cf4b50cfe20765, 0xfff4b4e3f741cf6d },
				{ 0x82818f1281ed449f, 0xbff8f10e7a8921a4 },
				{ 0xa321f2d7226895c7, 0xaff72d52192b6a0d },
				{ 0xcbea6f8ceb02bb39, 0x9bf4f8a69f764490 },
				{ 0xfee50b7025c36a08, 0x02f236d04753d5b4 },
				{ 0x9f4f2726179a2245, 0x01d762422c946590 },
				{ 0xc722f0ef9d80aad6, 0x424d3ad2b7b97ef5 },
				{ 0xf8ebad2b84e0d58b, 0xd2e0898765a7deb2 },
				{ 0x9b934c3b330c8577, 0x63cc55f49f88eb2f },
				{ 0xc2781f49ffcfa6d5, 0x3cbf6b71c76b25fb },
				{ 0xf316271c7fc3908a, 0x8bef464e3945ef7a },
				{ 0x97edd871cfda3a56, 0x97758bf0e3cbb5ac },
				{ 0xbde94e8e43d0c8ec, 0x3d52eeed1cbea317 },
				{ 0xed63a231d4c4fb27, 0x4ca7aaa863ee4bdd },
				{ 0x945e455f24fb1cf8, 0x8fe8caa93e74ef6a },
				{ 0xb975d6b6ee39e436, 0xb3e2fd538e122b44 },
				{ 0xe7d34c64a9c85d44, 0x60dbbca87196b616 },
				{ 0x90e40fbeea1d3a4a, 0xbc8955e946fe31cd },
				{ 0xb51d13aea4a488dd, 0x6babab6398bdbe41 },
				{ 0xe264589a4dcdab14, 0xc696963c7eed2dd1 },
				{ 0x8d7eb76070a08aec, 0xfc1e1de5cf543ca2 },
				{ 0xb0de65388cc8ada8, 0x3b25a55f43294bcb },
				{ 0xdd15fe86affad912, 0x49ef0eb713f39ebe },
				{ 0x8a2dbf142dfcc7ab, 0x6e3569326c784337 },
				{ 0xacb92ed9397bf996, 0x49c2c37f07965404 },
				{ 0xd7e77a8f87daf7fb, 0xdc33745ec97be906 },
				{ 0x86f0ac99b4e8dafd, 0x69a028bb3ded71a3 },
				{ 0xa8acd7c0222311bc, 0xc40832ea0d68ce0c },
				{ 0xd2d80db02aabd62b, 0xf50a3fa490c30190 },
				{ 0x83c7088e1aab65db, 0x792667c6da79e0fa },
				{ 0xa4b8cab1a1563f52, 0x577001b891185938 },
				{ 0xcde6fd5e09abcf26, 0xed4c0226b55e6f86 },
				{ 0x80b05e5ac60b6178, 0x544f8158315b05b4 },
				{ 0xa0dc75f1778e39d6, 0x696361ae3db1c721 },
				{ 0xc913936dd571c84c, 0x03bc3a19cd1e38e9 },
				{ 0xfb5878494ace3a5f, 0x04ab48a04065c723 },
				{ 0x9d174b2dcec0e47b, 0x62eb0d64283f9c76 },
				{ 0xc45d1df942711d9a, 0x3ba5d0bd324f8394 },
				{ 0xf5746577930d6500, 0xca8f44ec7ee36479 },
				{ 0x9968bf6abbe85f20, 0x7e998b13cf4e1ecb },
				{ 0xbfc2ef456ae276e8, 0x9e3fedd8c321a67e },
				{ 0xefb3ab16c59b14a2, 0xc5cfe94ef3ea101e },
				{ 0x95d04aee3b80ece5, 0xbba1f1d158724a12 },
				{ 0xbb445da9ca61281f, 0x2a8a6e45ae8edc97 },
				{ 0xea1575143cf97226, 0xf52d09d71a3293bd },
				{ 0x924d692ca61be758, 0x593c2626705f9c56 },
				{ 0xb6e0c377cfa2e12e, 0x6f8b2fb00c77836c },
				{ 0xe498f455c38b997a, 0x0b6dfb9c0f956447 },
				{ 0x8edf98b59a373fec, 0x4724bd4189bd5eac },
				{ 0xb2977ee300c50fe7, 0x58edec91ec2cb657 },
				{ 0xdf3d5e9bc0f653e1, 0x2f2967b66737e3ed },
				{ 0x8b865b215899f46c, 0xbd79e0d20082ee74 },
				{ 0xae67f1e9aec07187, 0xecd8590680a3aa11 },
				{ 0xda01ee641a708de9, 0xe80e6f4820cc9495 },
				{ 0x884134fe908658b2, 0x3109058d147fdcdd },
				{ 0xaa51823e34a7eede, 0xbd4b46f0599fd415 },
				{ 0xd4e5e2cdc1d1ea96, 0x6c9e18ac7007c91a },
				{ 0x850fadc09923329e, 0x03e2cf6bc604ddb0 },
				{ 0xa6539930bf6bff45, 0x84db8346b786151c },
				{ 0xcfe87f7cef46ff16, 0xe612641865679a63 },
				{ 0x81f14fae158c5f6e, 0x4fcb7e8f3f60c07e },
				{ 0xa26da3999aef7749, 0xe3be5e330f38f09d },
				{ 0xcb090c8001ab551c, 0x5cadf5bfd3072cc5 },
				{ 0xfdcb4fa002162a63, 0x73d9732fc7c8f7f6 },
				{ 0x9e9f11c4014dda7e, 0x2867e7fddcdd9afa },
				{ 0xc646d63501a1511d, 0xb281e1fd541501b8 },
				{ 0xf7d88bc24209a565, 0x1f225a7ca91a4226 },
				{ 0x9ae757596946075f, 0x3375788de9b06958 },
				{ 0xc1a12d2fc3978937, 0x0052d6b1641c83ae },
				{ 0xf209787bb47d6b84, 0xc0678c5dbd23a49a },
				{ 0x9745eb4d50ce6332, 0xf840b7ba963646e0 },
				{ 0xbd176620a501fbff, 0xb650e5a93bc3d898 },
				{ 0xec5d3fa8ce427aff, 0xa3e51f138ab4cebe },
				{ 0x93ba47c980e98cdf, 0xc66f336c36b10137 },
				{ 0xb8a8d9bbe123f017, 0xb80b0047445d4184 },
				{ 0xe6d3102ad96cec1d, 0xa60dc059157491e5 },
				{ 0x9043ea1ac7e41392, 0x87c89837ad68db2f },
				{ 0xb454e4a179dd1877, 0x29babe4598c311fb },
				{ 0xe16a1dc9d8545e94, 0xf4296dd6fef3d67a },
				{ 0x8ce2529e2734bb1d, 0x1899e4a65f58660c },
				{ 0xb01ae745b101e9e4, 0x5ec05dcff72e7f8f },
				{ 0xdc21a1171d42645d, 0x76707543f4fa1f73 },
				{ 0x899504ae72497eba, 0x6a06494a791c53a8 },
				{ 0xabfa45da0edbde69, 0x0487db9d17636892 },
				{ 0xd6f8d7509292d603, 0x45a9d2845d3c42b6 },
				{ 0x865b86925b9bc5c2, 0x0b8a2392ba45a9b2 },
				{ 0xa7f26836f282b732, 0x8e6cac7768d7141e },
				{ 0xd1ef0244af2364ff, 0x3207d795430cd926 },
				{ 0x8335616aed761f1f, 0x7f44e6bd49e807b8 },
				{ 0xa402b9c5a8d3a6e7, 0x5f16206c9c6209a6 },
				{ 0xcd036837130890a1, 0x36dba887c37a8c0f },
				{ 0x802221226be55a64, 0xc2494954da2c9789 },
				{ 0xa02aa96b06deb0fd, 0xf2db9baa10b7bd6c },
				{ 0xc83553c5c8965d3d, 0x6f92829494e5acc7 },
				{ 0xfa42a8b73abbf48c, 0xcb772339ba1f17f9 },
				{ 0x9c69a97284b578d7, 0xff2a760414536efb },
				{ 0xc38413cf25e2d70d, 0xfef5138519684aba },
				{ 0xf46518c2ef5b8cd1, 0x7eb258665fc25d69 },
				{ 0x98bf2f79d5993802, 0xef2f773ffbd97a61 },
				{ 0xbeeefb584aff8603, 0xaafb550ffacfd8fa },
				{ 0xeeaaba2e5dbf6784, 0x95ba2a53f983cf38 },
				{ 0x952ab45cfa97a0b2, 0xdd945a747bf26183 },
				{ 0xba756174393d88df, 0x94f971119aeef9e4 },
				{ 0xe912b9d1478ceb17, 0x7a37cd5601aab85d },
				{ 0x91abb422ccb812ee, 0xac62e055c10ab33a },
				{ 0xb616a12b7fe617aa, 0x577b986b314d6009 },
				{ 0xe39c49765fdf9d94, 0xed5a7e85fda0b80b },
				{ 0x8e41ade9fbebc27d, 0x14588f13be847307 },
				{ 0xb1d219647ae6b31c, 0x596eb2d8ae258fc8 },
				{ 0xde469fbd99a05fe3, 0x6fca5f8ed9aef3bb },
				{ 0x8aec23d680043bee, 0x25de7bb9480d5854 },
				{ 0xada72ccc20054ae9, 0xaf561aa79a10ae6a },
				{ 0xd910f7ff28069da4, 0x1b2ba1518094da04 },
				{ 0x87aa9aff79042286, 0x90fb44d2f05d0842 },
				{ 0xa99541bf57452b28, 0x353a1607ac744a53 },
				{ 0xd3fa922f2d1675f2, 0x42889b8997915ce8 },
				{ 0x847c9b5d7c2e09b7, 0x69956135febada11 },
				{ 0xa59bc234db398c25, 0x43fab9837e699095 },
				{ 0xcf02b2c21207ef2e, 0x94f967e45e03f4bb },
				{ 0x8161afb94b44f57d, 0x1d1be0eebac278f5 },
				{ 0xa1ba1ba79e1632dc, 0x6462d92a69731732 },
				{ 0xca28a291859bbf93, 0x7d7b8f7503cfdcfe },
				{ 0xfcb2cb35e702af78, 0x5cda735244c3d43e },
				{ 0x9defbf01b061adab, 0x3a0888136afa64a7 },
				{ 0xc56baec21c7a1916, 0x088aaa1845b8fdd0 },
				{ 0xf6c69a72a3989f5b, 0x8aad549e57273d45 },
				{ 0x9a3c2087a63f6399, 0x36ac54e2f678864b },
				{ 0xc0cb28a98fcf3c7f, 0x84576a1bb416a7dd },
				{ 0xf0fdf2d3f3c30b9f, 0x656d44a2a11c51d5 },
				{ 0x969eb7c47859e743, 0x9f644ae5a4b1b325 },
				{ 0xbc4665b596706114, 0x873d5d9f0dde1fee },
				{ 0xeb57ff22fc0c7959, 0xa90cb506d155a7ea },
				{ 0x9316ff75dd87cbd8, 0x09a7f12442d588f2 },
				{ 0xb7dcbf5354e9bece, 0x0c11ed6d538aeb2f },
				{ 0xe5d3ef282a242e81, 0x8f1668c8a86da5fa },
				{ 0x8fa475791a569d10, 0xf96e017d694487bc },
				{ 0xb38d92d760ec4455, 0x37c981dcc395a9ac },
				{ 0xe070f78d3927556a, 0x85bbe253f47b1417 },
				{ 0x8c469ab843b89562, 0x93956d7478ccec8e },
				{ 0xaf58416654a6babb, 0x387ac8d1970027b2 },
				{ 0xdb2e51bfe9d0696a, 0x06997b05fcc0319e },
				{ 0x88fcf317f22241e2, 0x441fece3bdf81f03 },
				{ 0xab3c2fddeeaad25a, 0xd527e81cad7626c3 },
				{ 0xd60b3bd56a5586f1, 0x8a71e223d8d3b074 },
				{ 0x85c7056562757456, 0xf6872d5667844e49 },
				{ 0xa738c6bebb12d16c, 0xb428f8ac016561db },
				{ 0xd106f86e69d785c7, 0xe13336d701beba52 },
				{ 0x82a45b450226b39c, 0xecc0024661173473 },
				{ 0xa34d721642b06084, 0x27f002d7f95d0190 },
				{ 0xcc20ce9bd35c78a5, 0x31ec038df7b441f4 },
				{ 0xff290242c83396ce, 0x7e67047175a15271 },
				{ 0x9f79a169bd203e41, 0x0f0062c6e984d386 },
				{ 0xc75809c42c684dd1, 0x52c07b78a3e60868 },
				{ 0xf92e0c3537826145, 0xa7709a56ccdf8a82 },
				{ 0x9bbcc7a142b17ccb, 0x88a66076400bb691 },
				{ 0xc2abf989935ddbfe, 0x6acff893d00ea435 },
				{ 0xf356f7ebf83552fe, 0x0583f6b8c4124d43 },
				{ 0x98165af37b2153de, 0xc3727a337a8b704a },
				{ 0xbe1bf1b059e9a8d6, 0x744f18c0592e4c5c },
				{ 0xeda2ee1c7064130c, 0x1162def06f79df73 },
				{ 0x9485d4d1c63e8be7, 0x8addcb5645ac2ba8 },
				{ 0xb9a74a0637ce2ee1, 0x6d953e2bd7173692 },
				{ 0xe8111c87c5c1ba99, 0xc8fa8db6ccdd0437 },
				{ 0x910ab1d4db9914a0, 0x1d9c9892400a22a2 },
				{ 0xb54d5e4a127f59c8, 0x2503beb6d00cab4b },
				{ 0xe2a0b5dc971f303a, 0x2e44ae64840fd61d },
				{ 0x8da471a9de737e24, 0x5ceaecfed289e5d2 },
				{ 0xb10d8e1456105dad, 0x7425a83e872c5f47 },
				{ 0xdd50f1996b947518, 0xd12f124e28f77719 },
				{ 0x8a5296ffe33cc92f, 0x82bd6b70d99aaa6f },
				{ 0xace73cbfdc0bfb7b, 0x636cc64d1001550b },
				{ 0xd8210befd30efa5a, 0x3c47f7e05401aa4e },
				{ 0x8714a775e3e95c78, 0x65acfaec34810a71 },
				{ 0xa8d9d1535ce3b396, 0x7f1839a741a14d0d },
				{ 0xd31045a8341ca07c, 0x1ede48111209a050 },
				{ 0x83ea2b892091e44d, 0x934aed0aab460432 },
				{ 0xa4e4b66b68b65d60, 0xf81da84d5617853f },
				{ 0xce1de40642e3f4b9, 0x36251260ab9d668e },
				{ 0x80d2ae83e9ce78f3, 0xc1d72b7c6b426019 },
				{ 0xa1075a24e4421730, 0xb24cf65b8612f81f },
				{ 0xc94930ae1d529cfc, 0xdee033f26797b627 },
				{ 0xfb9b7cd9a4a7443c, 0x169840ef017da3b1 },
				{ 0x9d412e0806e88aa5, 0x8e1f289560ee864e },
				{ 0xc491798a08a2ad4e, 0xf1a6f2bab92a27e2 },
				{ 0xf5b5d7ec8acb58a2, 0xae10af696774b1db },
				{ 0x9991a6f3d6bf1765, 0xacca6da1e0a8ef29 },
				{ 0xbff610b0cc6edd3f, 0x17fd090a58d32af3 },
				{ 0xeff394dcff8a948e, 0xddfc4b4cef07f5b0 },
				{ 0x95f83d0a1fb69cd9, 0x4abdaf101564f98e },
				{ 0xbb764c4ca7a4440f, 0x9d6d1ad41abe37f1 },
				{ 0xea53df5fd18d5513, 0x84c86189216dc5ed },
				{ 0x92746b9be2f8552c, 0x32fd3cf5b4e49bb4 },
				{ 0xb7118682dbb66a77, 0x3fbc8c33221dc2a1 },
				{ 0xe4d5e82392a40515, 0x0fabaf3feaa5334a },
				{ 0x8f05b1163ba6832d, 0x29cb4d87f2a7400e },
				{ 0xb2c71d5bca9023f8, 0x743e20e9ef511012 },
				{ 0xdf78e4b2bd342cf6, 0x914da9246b255416 },
				{ 0x8bab8eefb6409c1a, 0x1ad089b6c2f7548e },
				{ 0xae9672aba3d0c320, 0xa184ac2473b529b1 },
				{ 0xda3c0f568cc4f3e8, 0xc9e5d72d90a2741e },
				{ 0x8865899617fb1871, 0x7e2fa67c7a658892 },
				{ 0xaa7eebfb9df9de8d, 0xddbb901b98feeab7 },
				{ 0xd51ea6fa85785631, 0x552a74227f3ea565 },
				{ 0x8533285c936b35de, 0xd53a88958f87275f },
				{ 0xa67ff273b8460356, 0x8a892abaf368f137 },
				{ 0xd01fef10a657842c, 0x2d2b7569b0432d85 },
				{ 0x8213f56a67f6b29b, 0x9c3b29620e29fc73 },
				{ 0xa298f2c501f45f42, 0x8349f3ba91b47b8f },
				{ 0xcb3f2f7642717713, 0x241c70a936219a73 },
				{ 0xfe0efb53d30dd4d7, 0xed238cd383aa0110 },
				{ 0x9ec95d1463e8a506, 0xf4363804324a40aa },
				{ 0xc67bb4597ce2ce48, 0xb143c6053edcd0d5 },
				{ 0xf81aa16fdc1b81da, 0xdd94b7868e94050a },
				{ 0x9b10a4e5e9913128, 0xca7cf2b4191c8326 },
				{ 0xc1d4ce1f63f57d72, 0xfd1c2f611f63a3f0 },
				{ 0xf24a01a73cf2dccf, 0xbc633b39673c8cec },
				{ 0x976e41088617ca01, 0xd5be0503e085d813 },
				{ 0xbd49d14aa79dbc82, 0x4b2d8644d8a74e18 },
				{ 0xec9c459d51852ba2, 0xddf8e7d60ed1219e },
				{ 0x93e1ab8252f33b45, 0xcabb90e5c942b503 },
				{ 0xb8da1662e7b00a17, 0x3d6a751f3b936243 },
				{ 0xe7109bfba19c0c9d, 0x0cc512670a783ad4 },
				{ 0x906a617d450187e2, 0x27fb2b80668b24c5 },
				{ 0xb484f9dc9641e9da, 0xb1f9f660802dedf6 },
				{ 0xe1a63853bbd26451, 0x5e7873f8a0396973 },
				{ 0x8d07e33455637eb2, 0xdb0b487b6423e1e8 },
				{ 0xb049dc016abc5e5f, 0x91ce1a9a3d2cda62 },
				{ 0xdc5c5301c56b75f7, 0x7641a140cc7810fb },
				{ 0x89b9b3e11b6329ba, 0xa9e904c87fcb0a9d },
				{ 0xac2820d9623bf429, 0x546345fa9fbdcd44 },
				{ 0xd732290fbacaf133, 0xa97c177947ad4095 },
				{ 0x867f59a9d4bed6c0, 0x49ed8eabcccc485d },
				{ 0xa81f301449ee8c70, 0x5c68f256bfff5a74 },
				{ 0xd226fc195c6a2f8c, 0x73832eec6fff3111 },
				{ 0x83585d8fd9c25db7, 0xc831fd53c5ff7eab },
				{ 0xa42e74f3d032f525, 0xba3e7ca8b77f5e55 },
				{ 0xcd3a1230c43fb26f, 0x28ce1bd2e55f35eb },
				{ 0x80444b5e7aa7cf85, 0x7980d163cf5b81b3 },
				{ 0xa0555e361951c366, 0xd7e105bcc332621f },
				{ 0xc86ab5c39fa63440, 0x8dd9472bf3fefaa7 },
				{ 0xfa856334878fc150, 0xb14f98f6f0feb951 },
				{ 0x9c935e00d4b9d8d2, 0x6ed1bf9a569f33d3 },
				{ 0xc3b8358109e84f07, 0x0a862f80ec4700c8 },
				{ 0xf4a642e14c6262c8, 0xcd27bb612758c0fa },
				{ 0x98e7e9cccfbd7dbd, 0x8038d51cb897789c },
				{ 0xbf21e44003acdd2c, 0xe0470a63e6bd56c3 },
				{ 0xeeea5d5004981478, 0x1858ccfce06cac74 },
				{ 0x95527a5202df0ccb, 0x0f37801e0c43ebc8 },
				{ 0xbaa718e68396cffd, 0xd30560258f54e6ba },
				{ 0xe950df20247c83fd, 0x47c6b82ef32a2069 },
				{ 0x91d28b7416cdd27e, 0x4cdc331d57fa5441 },
				{ 0xb6472e511c81471d, 0xe0133fe4adf8e952 },
				{ 0xe3d8f9e563a198e5, 0x58180fddd97723a6 },
				{ 0x8e679c2f5e44ff8f, 0x570f09eaa7ea7648 },
				{ 0xb201833b35d63f73, 0x2cd2cc6551e513da },
				{ 0xde81e40a034bcf4f, 0xf8077f7ea65e58d1 },
				{ 0x8b112e86420f6191, 0xfb04afaf27faf782 },
				{ 0xadd57a27d29339f6, 0x79c5db9af1f9b563 },
				{ 0xd94ad8b1c7380874, 0x18375281ae7822bc },
				{ 0x87cec76f1c830548, 0x8f2293910d0b15b5 },
				{ 0xa9c2794ae3a3c69a, 0xb2eb3875504ddb22 },
				{ 0xd433179d9c8cb841, 0x5fa60692a46151eb },
				{ 0x849feec281d7f328, 0xdbc7c41ba6bcd333 },
				{ 0xa5c7ea73224deff3, 0x12b9b522906c0800 },
				{ 0xcf39e50feae16bef, 0xd768226b34870a00 },
				{ 0x81842f29f2cce375, 0xe6a1158300d46640 },
				{ 0xa1e53af46f801c53, 0x60495ae3c1097fd0 },
				{ 0xca5e89b18b602368, 0x385bb19cb14bdfc4 },
				{ 0xfcf62c1dee382c42, 0x46729e03dd9ed7b5 },
				{ 0x9e19db92b4e31ba9, 0x6c07a2c26a8346d1 },
				{ 0xc5a05277621be293, 0xc7098b7305241885 },
				{ 0xf70867153aa2db38, 0xb8cbee4fc66d1ea7 }
			};
		};

		template <class Float>
		constexpr typename common_info<Float>::cache_entry_type const& get_cache(int k) noexcept {
			assert(k >= common_info<Float>::min_k && k <= common_info<Float>::max_k);
			return cache_holder<Float>::cache[std::size_t(k - common_info<Float>::min_k)];
		}

		// Forward declaration of the main class
		template <class Float>
		struct dragonbox_impl;
	}

	////////////////////////////////////////////////////////////////////////////////////////
	// DIY floating-point data type
	////////////////////////////////////////////////////////////////////////////////////////

	template <class Float, bool is_signed>
	struct fp_t;

	template <class Float>
	struct fp_t<Float, false> {
		using extended_significand_type =
			typename dragonbox_detail::common_info<Float>::extended_significand_type;

		extended_significand_type	significand;
		int							exponent;
	};

	template <class Float>
	struct fp_t<Float, true> {
		using extended_significand_type =
			typename dragonbox_detail::common_info<Float>::extended_significand_type;

		extended_significand_type	significand;
		int							exponent;
		bool						is_negative;
	};

	template <class Float>
	using unsigned_fp_t = fp_t<Float, false>;

	template <class Float>
	using signed_fp_t = fp_t<Float, true>;

	// In order to reduce the argument passing overhead,
	// this class should be as simple as possible.
	// (e.g., no inheritance, no private non-static data member, etc.;
	// this is an unfortunate fact about x64 calling convention.)
	template <class Float>
	struct bit_representation_t
	{
	private:
		using common_info = dragonbox_detail::common_info<Float>;

		static constexpr auto precision = common_info::precision;
		static constexpr auto extended_precision = common_info::extended_precision;
		static constexpr auto sign_bit_mask = common_info::sign_bit_mask;
		static constexpr auto exponent_bits = common_info::exponent_bits;
		static constexpr auto exponent_bias = common_info::exponent_bias;
		static constexpr auto exponent_bits_mask = common_info::exponent_bits_mask;

	public:
		using extended_significand_type =
			typename common_info::extended_significand_type;
		extended_significand_type f;

		//// Inspector methods

		Float as_ieee754() const noexcept {
			Float x;
			std::memcpy(&x, &f, sizeof(Float));
			return x;
		}

		constexpr extended_significand_type extract_significand_bits() const noexcept {
			constexpr auto significand_bits_mask =
				(extended_significand_type(1) << precision) - 1;
			return f & significand_bits_mask;
		}

		constexpr std::uint32_t extract_exponent_bits() const noexcept {
			constexpr auto exponent_bits_mask =
				(std::uint32_t(1) << exponent_bits) - 1;
			return std::uint32_t(f >> precision) & exponent_bits_mask;
		}

		constexpr bool is_finite() const noexcept {
			return (f & exponent_bits_mask) != exponent_bits_mask;
		}

		constexpr bool is_nonzero() const noexcept {
			// vs (f & ~sign_bit_mask) != 0;
			// It seems that there is no AND instruction for 64-bit immediate value in x86,
			// thus (f & ~sign_bit_mask) != 0 generates 3 instructions (load, and, compare),
			// while this generates only two (shift, compare).
			return (f << 1) != 0;
		}

		// Allows positive and negative zeros
		constexpr bool is_subnormal() const noexcept {
			return (f & exponent_bits_mask) == 0;
		}

		// Allows negative zero and negative NaN's, but not allow positive zero
		constexpr bool is_negative() const noexcept {
			// vs (f & sign_bit_mask) != 0;
			// It seems that there is no AND instruction for 64-bit immediate value in x86,
			// thus (f & sign_bit_mask) != 0 generates 3 instructions (load, and, compare),
			// while this generates only two (shift, compare).
			return (f >> (extended_precision - 1)) != 0;
		}

		// Allows positive zero and positive NaN's, but not allow negative zero
		constexpr bool is_positive() const noexcept {
			// vs (f & sign_bit_mask) == 0;
			// Ditto
			return (f >> (extended_precision - 1)) == 0;
		}

		constexpr bool is_positive_infinity() const noexcept {
			constexpr auto positive_infinity = exponent_bits_mask;
			return f == positive_infinity;
		}

		constexpr bool is_negative_infinity() const noexcept {
			constexpr auto negative_infinity = exponent_bits_mask | sign_bit_mask;
			return f == negative_infinity;
		}

		// Allows both plus and minus infinities
		constexpr bool is_infinity() const noexcept {
			return is_positive_infinity() || is_negative_infinity();
		}

		constexpr bool is_nan() const noexcept {
			return !is_finite() && (extract_significand_bits() != 0);
		}

		bool is_quiet_nan() const noexcept {
			if (!is_nan()) {
				return false;
			}

			auto quiet_or_signal_indicator = extended_significand_type(1) << (precision - 1);
			auto quiet_or_signal = f & quiet_or_signal_indicator;

			constexpr auto a_quiet_nan = std::numeric_limits<Float>::quiet_NaN();
			extended_significand_type a_quiet_nan_bit_representation;
			std::memcpy(&a_quiet_nan_bit_representation, &a_quiet_nan, sizeof(Float));

			return (a_quiet_nan_bit_representation & quiet_or_signal_indicator)
				== quiet_or_signal;
		}

		bool is_signaling_nan() const noexcept {
			return is_nan() && !is_quiet_nan();
		}

		static constexpr std::size_t nan_payload_length = precision - 1;
		std::bitset<nan_payload_length> get_nan_payload() const noexcept {
			constexpr auto payload_mask =
				(extended_significand_type(1) << (precision - 1)) - 1;
			return{ f & payload_mask };
		}
	};

	template <class Float>
	bit_representation_t<Float> get_bit_representation(Float x) noexcept {
		bit_representation_t<Float> br;
		std::memcpy(&br.f, &x, sizeof(Float));
		return br;
	}

	////////////////////////////////////////////////////////////////////////////////////////
	// Determine what to do about the correct rounding guarantee
	////////////////////////////////////////////////////////////////////////////////////////

	namespace dragonbox_correct_rounding {
		enum tag_t {
			do_not_care_tag,
			tie_to_even_tag,
			tie_to_odd_tag,
			tie_to_up_tag,
			tie_to_down_tag
		};

		// Do not perform correct rounding search
		struct do_not_care {
			static constexpr tag_t tag = do_not_care_tag;
			template <bool return_sign, class Float, class IntervalTypeProvider>
			fp_t<Float, return_sign> delegate(bit_representation_t<Float> br,
				IntervalTypeProvider&&) const
			{
				return dragonbox_detail::dragonbox_impl<Float>::template compute<return_sign,
					std::remove_cv_t<std::remove_reference_t<IntervalTypeProvider>>, do_not_care>(br);
			}
		};

		// Perform correct rounding search; tie-to-even
		struct tie_to_even {
			static constexpr tag_t tag = tie_to_even_tag;
			template <bool return_sign, class Float, class IntervalTypeProvider>
			fp_t<Float, return_sign> delegate(bit_representation_t<Float> br,
				IntervalTypeProvider&&) const
			{
				return dragonbox_detail::dragonbox_impl<Float>::template compute<return_sign,
					std::remove_cv_t<std::remove_reference_t<IntervalTypeProvider>>, tie_to_even>(br);
			}
		};

		// Perform correct rounding search; tie-to-odd
		struct tie_to_odd {
			static constexpr tag_t tag = tie_to_even_tag;
			template <bool return_sign, class Float, class IntervalTypeProvider>
			fp_t<Float, return_sign> delegate(bit_representation_t<Float> br,
				IntervalTypeProvider&&) const
			{
				return dragonbox_detail::dragonbox_impl<Float>::template compute<return_sign,
					std::remove_cv_t<std::remove_reference_t<IntervalTypeProvider>>, tie_to_odd>(br);
			}
		};

		// Perform correct rounding search; tie-to-up
		struct tie_to_up {
			static constexpr tag_t tag = tie_to_up_tag;
			template <bool return_sign, class Float, class IntervalTypeProvider>
			fp_t<Float, return_sign> delegate(bit_representation_t<Float> br,
				IntervalTypeProvider&&) const
			{
				return dragonbox_detail::dragonbox_impl<Float>::template compute<return_sign,
					std::remove_cv_t<std::remove_reference_t<IntervalTypeProvider>>, tie_to_up>(br);
			}
		};

		// Perform correct rounding search; tie-to-down
		struct tie_to_down {
			static constexpr tag_t tag = tie_to_down_tag;
			template <bool return_sign, class Float, class IntervalTypeProvider>
			fp_t<Float, return_sign> delegate(bit_representation_t<Float> br,
				IntervalTypeProvider&&) const
			{
				return dragonbox_detail::dragonbox_impl<Float>::template compute<return_sign,
					std::remove_cv_t<std::remove_reference_t<IntervalTypeProvider>>, tie_to_down>(br);
			}
		};
	}

	////////////////////////////////////////////////////////////////////////////////////////
	// Rounding modes
	////////////////////////////////////////////////////////////////////////////////////////

	namespace dragonbox_rounding_modes {
		enum tag_t {
			to_nearest_tag,
			left_closed_directed_tag,
			right_closed_directed_tag
		};
		namespace interval_type {
			struct symmetric_boundary {
				static constexpr bool is_symmetric = true;
				bool is_closed;

				constexpr bool include_left_endpoint() const noexcept {
					return is_closed;
				}
				constexpr bool include_right_endpoint() const noexcept {
					return is_closed;
				}
			};
			struct asymmetric_boundary {
				static constexpr bool is_symmetric = false;
				bool is_left_closed;
				constexpr bool include_left_endpoint() const noexcept {
					return is_left_closed;
				}
				constexpr bool include_right_endpoint() const noexcept {
					return !is_left_closed;
				}
			};
			struct closed {
				static constexpr bool is_symmetric = true;
				static constexpr bool include_left_endpoint() noexcept {
					return true;
				}
				static constexpr bool include_right_endpoint() noexcept {
					return true;
				}
			};
			struct open {
				static constexpr bool is_symmetric = true;
				static constexpr bool include_left_endpoint() noexcept {
					return false;
				}
				static constexpr bool include_right_endpoint() noexcept {
					return false;
				}
			};
			struct left_closed_right_open {
				static constexpr bool is_symmetric = false;
				static constexpr bool include_left_endpoint() noexcept {
					return true;
				}
				static constexpr bool include_right_endpoint() noexcept {
					return false;
				}
			};
			struct right_closed_left_open {
				static constexpr bool is_symmetric = false;
				static constexpr bool include_left_endpoint() noexcept {
					return false;
				}
				static constexpr bool include_right_endpoint() noexcept {
					return true;
				}
			};
		}

		struct nearest_to_even {
			static constexpr tag_t tag = to_nearest_tag;

			template <bool return_sign, class Float, class CorrectRoundingSearch>
			fp_t<Float, return_sign> delegate(bit_representation_t<Float> br,
				CorrectRoundingSearch&& crs) const
			{
				return std::forward<CorrectRoundingSearch>(crs).template delegate<return_sign>(
					br, *this);
			}
			template <class Float>
			constexpr interval_type::symmetric_boundary operator()(bit_representation_t<Float> br) const noexcept {
				return{ br.f % 2 == 0 };
			}
		};
	};

	namespace dragonbox_detail {
		////////////////////////////////////////////////////////////////////////////////////////
		// The main algorithm
		////////////////////////////////////////////////////////////////////////////////////////

		// Get sign/decimal significand/decimal exponent from
		// the bit representation of a floating-point number
		template <class Float>
		struct dragonbox_impl : private common_info<Float>
		{
			using extended_significand_type =
				typename common_info<Float>::extended_significand_type;
			using cache_entry_type =
				typename common_info<Float>::cache_entry_type;

			using common_info<Float>::precision;
			using common_info<Float>::extended_precision;
			using common_info<Float>::cache_precision;
			using common_info<Float>::sign_bit_mask;
			using common_info<Float>::exponent_bits;
			using common_info<Float>::exponent_bias;
			using common_info<Float>::exponent_bits_mask;
			using common_info<Float>::min_exponent;
			using common_info<Float>::boundary_bit;
			using common_info<Float>::normal_interval_length;
			using common_info<Float>::closer_boundary_bit;
			using common_info<Float>::decimal_digits;
			using common_info<Float>::divtest_table_size;

			//// The main algorithm assumes the input is a normal/subnormal finite number

			template <bool return_sign, class IntervalTypeProvider, class CorrectRoundingSearch>
			JKJ_SAFEBUFFERS
			static fp_t<Float, return_sign> compute(bit_representation_t<Float> br) noexcept
			{
				//////////////////////////////////////////////////////////////////////
				// Step 1: integer promotion & Schubfach multiplier calculation
				//////////////////////////////////////////////////////////////////////

				fp_t<Float, return_sign> ret_value;

				auto interval_type = IntervalTypeProvider{}(br);

				if constexpr (return_sign) {
					ret_value.is_negative = br.is_negative();
				}
				auto significand = br.f << exponent_bits;
				auto exponent = int((br.f >> precision) & (exponent_bits_mask >> precision));
				bool closer_boundary = ((significand << 1) == 0) && (exponent != 0);

				// Deal with normal/subnormal dichotomy
				if (exponent != 0) {
					significand |= sign_bit_mask;
					exponent = exponent + exponent_bias - int(extended_precision) + 1;
				}
				else {
					exponent = min_exponent;
				}

				// Compute the endpoints
				extended_significand_type fr;
				// For nearest rounding
				if constexpr (IntervalTypeProvider::tag ==
					dragonbox_rounding_modes::to_nearest_tag)
				{
					fr = significand | boundary_bit;
				}
				// For left-closed directed rounding
				else if constexpr (IntervalTypeProvider::tag ==
					dragonbox_rounding_modes::left_closed_directed_tag)
				{
					fr = significand + normal_interval_length;
				}
				// For right-closed directed rounding
				else {
					fr = significand;
				}

				// Compute k and beta
				int const minus_k = closer_boundary
					? floor_log10_pow2_minus_log10_4_over_3(exponent + int(extended_precision - precision - 1)) - 1
					: floor_log10_pow2(exponent + int(extended_precision - precision - 1)) - 1;
				int const minus_beta = -(exponent + floor_log2_pow10(-minus_k) + 1);
				assert(extended_precision - precision - 9 <= minus_beta &&
					minus_beta <= extended_precision - precision - 5);

				// Compute zi and deltai
				auto const cache = jkj::dragonbox_detail::get_cache<Float>(-minus_k);

				extended_significand_type zi;
				if constexpr (IntervalTypeProvider::tag ==
					dragonbox_rounding_modes::left_closed_directed_tag)
				{
					// Take care of the case when overflow occurs
					if (fr == 0) {
						if constexpr (sizeof(Float) == 4) {
							zi = extended_significand_type(cache >>
								extended_precision >> minus_beta);
						}
						else {
							zi = cache.high() >> minus_beta;
						}
					}
					else {
						zi = compute_mul(fr, cache, minus_beta);
					}
				}
				else {
					zi = compute_mul(fr, cache, minus_beta);
				}
				auto deltai = compute_delta<IntervalTypeProvider::tag>(closer_boundary, cache, minus_beta);

				ret_value.exponent = minus_k + 2;


				//////////////////////////////////////////////////////////////////////
				// Step 2: Compute the exponent
				//////////////////////////////////////////////////////////////////////

				// Can we reduce the significand twice?
				ret_value.significand = zi / 100;
				auto remainder = std::uint32_t(zi % 100);

				if (remainder > deltai) {
					goto correct_rounding_search_label;
				}
				else if (remainder == deltai) {
					// Compare fractional parts
					if (!is_zf_smaller_than_deltaf<IntervalTypeProvider::tag>(significand, closer_boundary,
						minus_beta, cache, interval_type, exponent, minus_k))
					{
						goto correct_rounding_search_label;
					}
				}

				if (!interval_type.include_right_endpoint() && remainder == 0) {
					if (is_right_boundary_integer<IntervalTypeProvider::tag>(fr, closer_boundary, exponent, minus_k))
					{
						// For right-closed directed rounding, the greatest one is the closest one
						if constexpr (IntervalTypeProvider::tag ==
							dragonbox_rounding_modes::right_closed_directed_tag)
						{
							--ret_value.exponent;
							ret_value.significand *= 10;
							ret_value.significand += (remainder / 10);
							goto remove_trailing_zeros_label;
						}
						// For the others, we need to exclude the right endpoint if necessary
						else if constexpr (CorrectRoundingSearch::tag ==
							dragonbox_correct_rounding::do_not_care_tag)
						{
							--ret_value.exponent;
							ret_value.significand *= 10;
							ret_value.significand += (remainder / 10);

							if (remainder % 10 == 0) {
								// We are sure that the intersection should be nonempty
								--ret_value.significand;
							}
							goto remove_trailing_zeros_label;
						}
						else {
							goto correct_rounding_search_label;
						}
					}
				}
				goto remove_trailing_zeros_label;


				//////////////////////////////////////////////////////////////////////
				// Step 3: Correct rounding search
				//////////////////////////////////////////////////////////////////////

			correct_rounding_search_label:
				--ret_value.exponent;
				ret_value.significand *= 10;
				ret_value.significand += (remainder / 10);
				remainder %= 10;

				// For right-closed directed rounding, the greatest one is the closest one
				if constexpr (IntervalTypeProvider::tag ==
					dragonbox_rounding_modes::right_closed_directed_tag)
				{
					goto remove_trailing_zeros_label;
				}
				// For other rounding modes, we might need to care about the case
				// when r = 0 and the right endpoint is excluded from the interval
				else if constexpr (CorrectRoundingSearch::tag ==
					dragonbox_correct_rounding::do_not_care_tag)
				{
					if (!interval_type.include_right_endpoint() && remainder == 0) {
						if (is_right_boundary_integer<IntervalTypeProvider::tag>(fr, closer_boundary, exponent, minus_k))
						{
							// We are sure that the intersection should be nonempty
							--ret_value.significand;
						}					
						goto remove_trailing_zeros_label;
					}
				}
				// Correct rounding search for left-closed directed rounding
				else if constexpr (IntervalTypeProvider::tag ==
					dragonbox_rounding_modes::left_closed_directed_tag)
				{
					// TODO
				}
				// Correct rounding search for nearest rounding
				else {
					auto displacement = remainder + 5;
					auto const epsiloni = compute_delta<dragonbox_rounding_modes::left_closed_directed_tag>(
						false, cache, minus_beta + 1);

					// n' + 1 >= 1?
					if (displacement <= epsiloni) {
						auto const approx_y = zi - epsiloni;

						displacement = epsiloni - displacement;
						auto steps = displacement / 10 + 1;

						// Check fractional if necessary
						if (displacement % 10 == 0) {
							auto const yi = compute_mul(significand, cache, minus_beta);
							// We have either yi == approx_y or yi == approx_y - 1
							if (yi == approx_y) {
								[[maybe_unused]] bool is_center_integer =
									closer_boundary ?
									is_product_integer<integer_check_case_id::fc_irregular_delta>(
										significand, exponent, minus_k) :
									is_product_integer<integer_check_case_id::fc_long_delta>(
										significand, exponent, minus_k);

								if constexpr (CorrectRoundingSearch::tag ==
									dragonbox_correct_rounding::tie_to_even_tag ||
									CorrectRoundingSearch::tag ==
									dragonbox_correct_rounding::tie_to_odd_tag)
								{
									// Compare round-up vs round-down
									// round-up  : steps - 1
									// round-down: steps - 1 if !is_center_integer, steps otherwise
									// If they differ, that is, if is_center_integer,
									// then prefer even/odd
									if (is_center_integer) {
										// steps vs steps - 1
										if constexpr (CorrectRoundingSearch::tag ==
											dragonbox_correct_rounding::tie_to_even_tag)
										{
											steps = (ret_value.significand & 1) != extended_significand_type(steps & 1)
												? steps - 1 : steps;
										}
										else
										{
											steps = (ret_value.significand & 1) == extended_significand_type(steps & 1)
												? steps - 1 : steps;
										}
									}
									else {
										--steps;
									}
								}
								else if constexpr (CorrectRoundingSearch::tag ==
									dragonbox_correct_rounding::tie_to_up_tag)
								{
									--steps;
								}
								else {
									if (!is_center_integer) {
										--steps;
									}
								}
							}
						}

						// The calculated steps might be too much if the left endpoint is closer than usual
						if (steps == 1 && closer_boundary) {
							// We know already r is at most deltai
							deltai -= std::uint32_t(remainder);
							if (deltai < 10) {
								goto remove_trailing_zeros_label;
							}
							else if (deltai == 10) {
								// See the test result of verify_incorrect_rounding_removal.cpp
								if constexpr (sizeof(Float) == 4) {
									if (exponent == 59) {
										goto remove_trailing_zeros_label;
									}
								}
								else {
									if (exponent == -203) {
										goto remove_trailing_zeros_label;
									}
								}
							}
						}

						ret_value.significand -= steps;
					}
				}


				//////////////////////////////////////////////////////////////////////
				// Step 4: Remove trailingzeros and return
				//////////////////////////////////////////////////////////////////////
				
			remove_trailing_zeros_label:
				// Remove trailing zeros and return
				auto t = bits::countr_zero(ret_value.significand);
				if (t > decimal_digits - 1) {
					t = decimal_digits - 1;
				}

				constexpr auto const& divtest_table =
					divtest::table_holder<extended_significand_type, 5, divtest_table_size>::table;

				if constexpr (sizeof(Float) == 4) {
					constexpr auto mod_inverse = divtest_table.mod_inverses[1];
					constexpr auto max_quotient = divtest_table.max_quotients[1];

					int s = 0;
					for (; s < t; ++s) {
						if (ret_value.significand * mod_inverse > max_quotient) {
							break;
						}
						ret_value.significand *= mod_inverse;
					}
					ret_value.significand >>= s;
					ret_value.exponent += s;
				}
				else {
					static_assert(sizeof(Float) == 8);

					// Divide by 10^8 and reduce to 32-bits
					// Since ret_value.significand <= 9'9999'9999'9999'9999,
					// both of the quotient and the remainder should fit in 32-bits
					auto quotient_candidate = ret_value.significand * divtest_table.mod_inverses[8];

					if (t >= 8 && quotient_candidate <= divtest_table.max_quotients[8]) {
						auto quotient = std::uint32_t(quotient_candidate >> 8);
						constexpr auto mod_inverse = std::uint32_t(divtest_table.mod_inverses[1]);
						constexpr auto max_quotient = std::numeric_limits<std::uint32_t>::max() / 5;

						ret_value.exponent += 8;
						t -= 8;

						int s = 0;
						for (; s < t; ++s) {
							if (quotient * mod_inverse > max_quotient) {
								break;
							}
							quotient *= mod_inverse;
						}
						quotient >>= s;
						ret_value.significand = quotient;
						ret_value.exponent += s;
					}
					else {
						constexpr auto mod_inverse = divtest_table.mod_inverses[1];
						constexpr auto max_quotient = divtest_table.max_quotients[1];

						int s = 0;
						for (; s < t; ++s) {
							if (ret_value.significand * mod_inverse > max_quotient) {
								break;
							}
							ret_value.significand *= mod_inverse;
						}
						ret_value.significand >>= s;
						ret_value.exponent += s;
					}
				}

				return ret_value;
			}

			static extended_significand_type compute_mul(
				extended_significand_type f, cache_entry_type const& cache, int minus_beta) noexcept
			{
				if constexpr (sizeof(Float) == 4) {
					return wide_uint::umul96_upper32(f, cache) >> minus_beta;
				}
				else {
					return wide_uint::umul192_upper64(f, cache) >> minus_beta;
				}
			}

			template <dragonbox_rounding_modes::tag_t tag>
			static std::uint32_t compute_delta([[maybe_unused]] bool closer_boundary,
				cache_entry_type const& cache, int minus_beta) noexcept
			{
				constexpr auto q_mp_m1 = extended_precision - precision - 1;
				constexpr auto intermediate_precision =
					sizeof(Float) == 4 ? cache_precision : extended_precision;
				using intermediate_type = std::conditional_t<sizeof(Float) == 4,
					cache_entry_type, extended_significand_type>;

				intermediate_type r;
				if constexpr (sizeof(Float) == 4) {
					r = cache;
				}
				else {
					r = cache.high();
				}

				// For nearest rounding
				if constexpr (tag == dragonbox_rounding_modes::to_nearest_tag)
				{
					if (closer_boundary) {
						r = (r >> 1) + (r >> 2);
					}

					assert(minus_beta < q_mp_m1);
					return std::uint32_t(r >> (intermediate_precision - q_mp_m1 + minus_beta));
				}
				// For left-directed rounding
				else if constexpr (tag == dragonbox_rounding_modes::left_closed_directed_tag)
				{
					assert(minus_beta < q_mp_m1);
					return std::uint32_t(r >> (intermediate_precision - q_mp_m1 + minus_beta));
				}
				// For right-directed rounding
				else {
					if (closer_boundary) {
						assert(minus_beta < q_mp_m1 - 1);
						return std::uint32_t(r >> (intermediate_precision - (q_mp_m1 - 1) + minus_beta));
					}
					else {
						assert(minus_beta < q_mp_m1);
						return std::uint32_t(r >> (intermediate_precision - q_mp_m1 + minus_beta));
					}
				}
			}

			static bool is_zf_strictly_smaller_than_deltaf(
				extended_significand_type fl,
				int minus_beta, cache_entry_type const& cache) noexcept
			{
				auto mul = compute_mul(fl, cache, minus_beta);
				return (mul & 1) != 0;
			}

			//      long delta: 2^(e+q-p-1)
			// irregular delta: 3 * 2^(e+q-p-3)
			//     short delta: 2^(e+q-p-2)
			enum class integer_check_case_id {
				fc_minus_2_to_the_q_mp_m3,
				fc_pm_2_to_the_q_mp_m2_long_delta,
				fc_plus_2_to_the_q_mp_m2_irregular_delta,
				fc_plus_2_to_the_q_mp_m1_long_delta,
				fc_long_delta,
				fc_irregular_delta
			};
			template <integer_check_case_id case_id>
			static bool is_product_integer([[maybe_unused]] extended_significand_type f,
				int exponent, [[maybe_unused]] int minus_k) noexcept
			{
				// Case I: f = fc - 2^(q-p-3), delta = 3 * 2^(e+q-p-3)
				if constexpr (case_id == integer_check_case_id::fc_minus_2_to_the_q_mp_m3)
				{
					return exponent <= -int(extended_precision - precision - 8) &&
						exponent >= -int(extended_precision - precision - 3);
				}
				// Case II: f = fc +- 2^(q-p-2), delta = 2^(e+q-p-1)
				else if constexpr (case_id == integer_check_case_id::fc_pm_2_to_the_q_mp_m2_long_delta)
				{
					if (exponent < -int(extended_precision - precision - 1)) {
						return false;
					}
					// For k >= 0
					else if (exponent <= -int(extended_precision - precision - 7)) {
						return true;
					}
					// For k < 0
					else {
						return divtest::divisible_by_power_of_5<divtest_table_size>(f, minus_k);
					}
				}
				// Case III: f = fc + 2^(q-p-2), delta = 3 * 2^(e+q-p-3)
				else if constexpr (case_id == integer_check_case_id::fc_plus_2_to_the_q_mp_m2_irregular_delta)
				{
					return exponent <= -int(extended_precision - precision - 8) &&
						exponent >= -int(extended_precision - precision);
				}
				// Case IV: f = fc + 2^(q-p-1), delta = 2^(e+q-p-1)
				// Case V: f = fc, delta = 2^(e+q-p-1)
				// Case VI: f = fc, delta = 3 * 2^(e+q-p-3)
				else
				{
					constexpr int upper_threshold =
						case_id == integer_check_case_id::fc_irregular_delta ?
						-int(extended_precision - precision - 8) :
						-int(extended_precision - precision - 7);

					constexpr int lower_threshold =
						case_id == integer_check_case_id::fc_irregular_delta ?
						-int(extended_precision - precision + 2) :
						-int(extended_precision - precision + 1);

					// Exponent for 5 is negative
					if (exponent > upper_threshold) {
						return divtest::divisible_by_power_of_5<divtest_table_size>(f, minus_k);
					}
					// Both exponents are nonnegative
					else if (exponent >= lower_threshold) {
						return true;
					}
					// Exponent for 2 is negative
					else {
						return divtest::divisible_by_power_of_2(f, minus_k - exponent);
					}
				}
			}

			template <dragonbox_rounding_modes::tag_t tag>
			static bool equal_fractional_parts([[maybe_unused]] extended_significand_type fl,
				[[maybe_unused]] bool closer_boundary, [[maybe_unused]] int exponent,
				[[maybe_unused]] int minus_k) noexcept
			{
				if constexpr (tag == dragonbox_rounding_modes::to_nearest_tag)
				{
					// Generic case
					if (!closer_boundary)
					{
						return is_product_integer<integer_check_case_id::fc_pm_2_to_the_q_mp_m2_long_delta>(
							fl, exponent, minus_k);
					}
					// Edge case
					else {
						return is_product_integer<integer_check_case_id::fc_minus_2_to_the_q_mp_m3>(
							fl, exponent, minus_k);
					}
				}
				// For left-closed directed rounding
				else if constexpr (tag == dragonbox_rounding_modes::left_closed_directed_tag)
				{
					// Returns zf_smaller if zf = deltaf
					return is_product_integer<integer_check_case_id::others>(
						fl, exponent, minus_k);
				}
				// For right-closed directed rounding
				else {
					// Since left endpoint is always not included,
					// we do not actually have to call this function at all
					return false;
				}
			}

			template <dragonbox_rounding_modes::tag_t tag, class IntervalType>
			static bool is_zf_smaller_than_deltaf(extended_significand_type fc,
				bool closer_boundary, int minus_beta, cache_entry_type const& cache,
				IntervalType& interval_type, int exponent, int minus_k) noexcept
			{
				// Compute fl
				extended_significand_type fl;
				if constexpr (tag == dragonbox_rounding_modes::to_nearest_tag)
				{
					fl = closer_boundary ?
						sign_bit_mask - closer_boundary_bit :
						fc - boundary_bit;
				}
				// For left-closed directed rounding
				else if constexpr (tag == dragonbox_rounding_modes::left_closed_directed_tag)
				{
					fl = fc;
				}
				// For right-closed directed rounding
				else {
					fl = closer_boundary ?
						sign_bit_mask - boundary_bit :
						fc - normal_interval_length;
				}

				return is_zf_strictly_smaller_than_deltaf(fl, minus_beta, cache) ||
					(interval_type.include_left_endpoint() &&
						equal_fractional_parts<tag>(fl, closer_boundary, exponent, minus_k));
			}

			template <dragonbox_rounding_modes::tag_t tag>
			static bool is_right_boundary_integer(extended_significand_type fr,
				bool closer_boundary, int exponent, int minus_k)
			{
				if constexpr (tag == dragonbox_rounding_modes::to_nearest_tag)
				{
					if (!closer_boundary) {
						return is_product_integer<integer_check_case_id::fc_pm_2_to_the_q_mp_m2_long_delta>(
							fr, exponent, minus_k);
					}
					else {
						return is_product_integer<integer_check_case_id::fc_plus_2_to_the_q_mp_m2_irregular_delta>(
							fr, exponent, minus_k);
					}
				}
				// Assumes left-closed directed rounding
				else
				{
					return is_product_integer<integer_check_case_id::fc_plus_2_to_the_q_mp_m1_long_delta>(
						fr, exponent, minus_k);
				}
			}
		};
	}

	// What to do with non-finite inputs?
	namespace dragonbox_case_handlers {
		struct assert_finite {
			template <class Float>
			void operator()([[maybe_unused]] bit_representation_t<Float> br) const
			{
				assert(br.is_finite());
			}
		};

		// This policy is mainly for debugging purpose
		struct ignore_special_cases {
			template <class Float>
			void operator()(bit_representation_t<Float>) const
			{
			}
		};
	}

	template <bool return_sign = true, class Float,
		class RoundingMode = dragonbox_rounding_modes::nearest_to_even,
		class CorrectRoundingSearch = int,
		class CaseHandler = dragonbox_case_handlers::assert_finite
	>
	fp_t<Float, return_sign> dragonbox(Float x,
		RoundingMode&& rounding_mode = {},
		CorrectRoundingSearch&& crs = {},
		CaseHandler&& case_handler = {})
	{
		auto br = get_bit_representation(x);
		case_handler(br);
		return std::forward<RoundingMode>(rounding_mode).template delegate<return_sign>(
			br, std::forward<CorrectRoundingSearch>(crs));
	}
}

#undef JKJ_HAS_COUNTR_ZERO_INTRINSIC
#undef JKJ_SAFEBUFFERS

#endif
