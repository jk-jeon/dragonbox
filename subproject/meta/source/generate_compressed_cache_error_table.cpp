// Copyright 2020-2022 Junekey Jeon
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

int main() {
    // For correct multiplication, the margin for binary32 is at least
    // 2^64 * 5091154818982829 / 12349290596248284087255008291061760 = 7.60...,
    // so the recovered cache can be larger than the original cache up to by 7.
    // The margin for binary64 is at least
    // 2^128 * 723173431431821867556830303887 /
    // 18550103527668669801949286474444582643081334006759269899933694558208
    // = 13.26..., so the recovered cache can be larger than the original cache up to by 7.
    // For correct integer checks, we need to ensure
    // m < 2^Q * a/b + 2^(q-beta+1)/b1 when b1 <= n_max.
    // As b1 is either a power of 2 or a power of 5, the maximum possible value
    // for b1 is 2^24 for binary32 and 5^23 for binary64.
    // With kappa = 1 for binary32, beta is at most floor((kappa+1)log2(10))+1 = 7,
    // so 2^(q-beta+1) is at least 2^26, so 2^(q-beta+1)/b1 >= 4.
    // Hence, the recovered cache can be larger than the original cache up to by 4
    // in this case.
    // With kappa = 2 for binary64, beta is at most floor((kappa+1)log2(10))+1 = 10,
    // so 2^(q-beta+1) is at least 2^55, so 2^(q-beta+1)/b1 >= 2^55/5^23 > 3.
    // Hence, the recovered cache can be larger than the original cache up to by 3
    // in this case.


    using namespace jkj::dragonbox::detail;

    std::cout << "[Verifying cache recovery for compressed cache...]\n";

    std::vector<std::uint32_t> results;

    constexpr int recov_size = 27;

    std::uint32_t error = 0;
    int error_count = 0;
    for (int k = impl<double>::min_k; k <= impl<double>::max_k; ++k) {
        using jkj::dragonbox::policy::cache::full;
        auto real_cache = full.get_cache<jkj::dragonbox::ieee754_binary64>(k);

        // Compute base index
        int kb = ((k - impl<double>::min_k) / recov_size) * recov_size + impl<double>::min_k;

        // Get base cache
        auto base_cache = full.get_cache<jkj::dragonbox::ieee754_binary64>(kb);

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
            auto middle_low = wuint::umul128(base_cache.low(), pow5);

            recovered_cache += middle_low.high();

            auto high_to_middle = recovered_cache.high() << (64 - alpha);
            auto middle_to_low = recovered_cache.low() << (64 - alpha);

            recovered_cache = wuint::uint128{(recovered_cache.low() >> alpha) | high_to_middle,
                                             ((middle_low.low() >> alpha) | middle_to_low)};

            if (recovered_cache.low() + 1 == 0) {
                std::cout << "Overflow detected.\n";
                return -1;
            }
            else {
                recovered_cache = {recovered_cache.high(), recovered_cache.low() + 1};
            }

            // Measure the difference
            if (real_cache.high() != recovered_cache.high() ||
                real_cache.low() > recovered_cache.low()) {
                std::cout << "Overflow detected.\n";
                return -1;
            }
            auto diff = std::uint32_t(recovered_cache.low() - real_cache.low());

            if (diff > 3) {
                std::cout << "Recovery error is too big.\n";
                return -1;
            }
        }
    }
}
