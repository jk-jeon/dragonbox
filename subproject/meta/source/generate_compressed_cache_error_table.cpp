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

#include <fstream>
#include <iomanip>
#include <iostream>
#include <vector>

int main()
{
	using namespace jkj::dragonbox::detail;

	std::cout << "[Generating error table for compressed cache...]\n";

	std::vector<std::uint32_t> results;

	constexpr int recov_size = 27;

	std::uint32_t error = 0;
	int error_count = 0;
	for (int k = impl<double>::min_k; k <= impl<double>::max_k; ++k)
	{
		using jkj::dragonbox::policy::cache::normal;
		auto real_cache = normal.get_cache<jkj::dragonbox::ieee754_format::binary64>(k);

		// Compute base index
		int kb = ((k - impl<double>::min_k) / recov_size) * recov_size + impl<double>::min_k;

		// Get base cache
		auto base_cache = normal.get_cache<jkj::dragonbox::ieee754_format::binary64>(kb);

		// Get index offset
		auto offset = k - kb;

		if (offset != 0) {
			// Compute corresponding power of 5
			std::uint64_t pow5 = 1;
			for (int i = 0; i < offset; ++i) {
				pow5 *= 5;
			}

			// Compute the required amount of bit-shift
			auto alpha = log::floor_log2_pow10(kb + offset) - log::floor_log2_pow10(kb) - offset;
			assert(alpha > 0 && alpha < 64);

			// Try to recover the real cache
			auto recovered_cache = wuint::umul128(base_cache.high(), pow5);
			auto middle_low = wuint::umul128(base_cache.low() - (kb < 0 ? 1 : 0), pow5);

			recovered_cache += middle_low.high();

			auto high_to_middle = recovered_cache.high() << (64 - alpha);
			auto middle_to_low = recovered_cache.low() << (64 - alpha);

			recovered_cache = wuint::uint128{
				(recovered_cache.low() >> alpha) | high_to_middle,
				((middle_low.low() >> alpha) | middle_to_low)
			};

			if (kb < 0) {
				if (recovered_cache.low() + 1 == 0) {
					recovered_cache = { recovered_cache.high() + 1, 0 };
				}
				else {
					recovered_cache = { recovered_cache.high(), recovered_cache.low() + 1 };
				}
			}

			// Measure the difference
			assert(real_cache.high() == recovered_cache.high());
			assert(real_cache.low() >= recovered_cache.low());
			auto diff = std::uint32_t(real_cache.low() - recovered_cache.low());

			assert((diff >> 2) == 0);

			error |= std::uint32_t(diff << (error_count * 2));
		}

		if (++error_count == 16) {
			results.push_back(error);
			error = 0;
			error_count = 0;
		}
	}

	if (error_count != 0) {
		results.push_back(error);
	}

	// Print out
	std::ofstream out{ "results/binary64_compressed_cache_error_table.txt" };
	out << "static constexpr std::uint32_t errors[] = {\n\t";
	for (std::size_t i = 0; i < results.size(); ++i) {
		if (i != 0) {
			if (i % 5 == 0) {
				out << ",\n\t";
			}
			else {
				out << ", ";
			}
		}
		out << std::hex << std::setfill('0') << "0x" << std::setw(8) << results[i];
	}
	out << "\n};";

	std::cout << "Done.\n\n\n";
}
