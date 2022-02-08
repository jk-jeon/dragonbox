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
#include "big_uint.h"
#include "rational_continued_fractions.h"

#include <fstream>
#include <iomanip>
#include <iostream>
#include <vector>

int main() {
    // We are trying to verify that an appropriate shift of phi_k * 5^a
    // can be used instead of phi_(a+k). Since phi_k is defined in terms of ceiling,
    // the shift of phi_k * 5^a will be phi_(a+k) + (error) for some nonnegative error.
    //
    // For correct multiplication, the margin for binary32 is at least
    // 2^64 * 5091154818982829 / 12349290596248284087255008291061760 = 7.60...,
    // so we are safe if the error is up to 7.
    // The margin for binary64 is at least
    // 2^128 * 723173431431821867556830303887 /
    // 18550103527668669801949286474444582643081334006759269899933694558208
    // = 13.26..., so we are safe if the error is up to 13.
    //
    // For correct integer checks, the case b > n_max is fine because the only condition on the
    // recovered cache is a lower bound which must be already true for phi_k.
    // For the case b <= n_max, we only need to check the upper bound
    // (recovered_cache) < 2^(Q-beta) * a/b + 2^(q-beta)/(floor(nmax/b) * b),
    // so we check it manually for each e.

    using namespace jkj::dragonbox::detail::log;
    using namespace jkj::dragonbox::detail::wuint;
    using info = jkj::dragonbox::detail::compressed_cache_detail;
    using impl = jkj::dragonbox::detail::impl<double>;

    std::cout << "[Verifying cache recovery for compressed cache...]\n";

    jkj::unsigned_rational<jkj::big_uint> unit;
    auto n_max = jkj::big_uint::power_of_2(impl::significand_bits + 2);
    int prev_k = impl::max_k + 1;
    for (int e = impl::min_exponent - impl::significand_bits;
         e <= impl::max_exponent - impl::significand_bits; ++e) {
        int const k = impl::kappa - floor_log10_pow2(e);

        using jkj::dragonbox::policy::cache::full;
        auto const real_cache = full.get_cache<jkj::dragonbox::ieee754_binary64>(k);

        // Compute the base index.
        int const kb =
            ((k - impl::min_k) / info::compression_ratio) * info::compression_ratio + impl::min_k;

        // Get the base cache.
        auto const base_cache = full.get_cache<jkj::dragonbox::ieee754_binary64>(kb);

        // Get the index offset.
        auto const offset = k - kb;

        if (offset != 0) {
            // Obtain the corresponding power of 5.
            auto const pow5 = info::pow5.table[offset];

            // Compute the required amount of bit-shifts.
            auto const alpha = floor_log2_pow10(kb + offset) - floor_log2_pow10(kb) - offset;
            assert(alpha > 0 && alpha < 64);

            // Try to recover the real cache.
            auto recovered_cache = umul128(base_cache.high(), pow5);
            auto const middle_low = umul128(base_cache.low(), pow5);

            recovered_cache += middle_low.high();

            auto const high_to_middle = recovered_cache.high() << (64 - alpha);
            auto const middle_to_low = recovered_cache.low() << (64 - alpha);

            recovered_cache = uint128{(recovered_cache.low() >> alpha) | high_to_middle,
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
            auto const diff = std::uint32_t(recovered_cache.low() - real_cache.low());

            if (diff != 0) {
                if (diff > 13) {
                    // Multiplication might be no longer valid.
                    std::cout << "Overflow detected.\n";
                    return -1;
                }

                // For the case b <= n_max, integer check might be no longer valid.
                int const beta = e + floor_log2_pow10(k);

                // unit = 2^(e + k - 1) * 5^k = a/b.
                unit.numerator = 1;
                unit.denominator = 1;
                if (k >= 0) {
                    unit.numerator = jkj::big_uint::pow(5, k);
                }
                else {
                    unit.denominator = jkj::big_uint::pow(5, -k);
                }
                if (e + k - 1 >= 0) {
                    unit.numerator *= jkj::big_uint::power_of_2(e + k - 1);
                }
                else {
                    unit.denominator *= jkj::big_uint::power_of_2(-e - k + 1);
                }

                if (unit.denominator <= n_max) {
                    // Check (recovered_cache) < 2^(Q-beta) * a/b + 2^(q-beta)/(floor(nmax/b) * b),
                    // or equivalently,
                    // b * (recovered_cache) - 2^(Q-beta) * a < 2^(q-beta) / floor(nmax/b).
                    auto const rc = jkj::big_uint{recovered_cache.low(), recovered_cache.high()};
                    auto const left_hand_side =
                        unit.denominator * rc -
                        jkj::big_uint::power_of_2(impl::cache_bits - beta) * unit.numerator;

                    if (left_hand_side * (n_max / unit.denominator) >=
                        jkj::big_uint::power_of_2(impl::carrier_bits - beta)) {
                        std::cout << "Overflow detected.\n";
                        return -1;
                    }
                }
            }
        }
    }
    std::cout << "Verification succeeded. No error detected.\n";
}
