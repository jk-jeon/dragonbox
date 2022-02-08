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
#include "continued_fractions.h"

#include <iostream>

template <class Float, class CachePolicy>
static bool verify_fast_multiplication_xz(CachePolicy&& cache_policy) {
    using impl = jkj::dragonbox::detail::impl<Float>;
    using format = typename impl::format;
    using carrier_uint = typename impl::carrier_uint;

    constexpr auto four_fl = (carrier_uint(1) << (impl::significand_bits + 2)) - 1;
    constexpr auto two_fr = (carrier_uint(1) << (impl::significand_bits + 1)) + 1;

    using jkj::dragonbox::detail::log::floor_log10_pow2_minus_log10_4_over_3;
    using jkj::dragonbox::detail::log::floor_log2_pow10;

    bool success = true;

    for (int e = impl::min_exponent + 1 - impl::significand_bits;
         e <= impl::max_exponent - impl::significand_bits; ++e) {

        // Compute k and beta.
        int const k = -floor_log10_pow2_minus_log10_4_over_3(e);
        int const beta = e + floor_log2_pow10(k);

        // Load cache.
        auto const cache = cache_policy.template get_cache<format>(k);

        // Compute the endpoints using the fast method.
        auto x_fast = impl::compute_left_endpoint_for_shorter_interval_case(cache, beta);
        auto z_fast = impl::compute_right_endpoint_for_shorter_interval_case(cache, beta);

        // Precisely compute the endpoints.
        jkj::unsigned_rational<jkj::big_uint> precise_multiplier{1, 1};
        if (k >= 0) {
            precise_multiplier.numerator = jkj::big_uint::pow(5, k);
        }
        else {
            precise_multiplier.denominator = jkj::big_uint::pow(5, -k);
        }
        if (e + k >= 0) {
            precise_multiplier.numerator *= jkj::big_uint::power_of_2(e + k);
        }
        else {
            precise_multiplier.denominator *= jkj::big_uint::power_of_2(-e - k);
        }

        auto x_exact =
            (four_fl * precise_multiplier.numerator) / (4 * precise_multiplier.denominator);
        auto z_exact =
            (two_fr * precise_multiplier.numerator) / (2 * precise_multiplier.denominator);

        if (x_exact != x_fast) {
            std::cout << "(e = " << e << ") left endpoint is not correct; computed = " << x_fast
                      << "; true_value = " << x_exact[0] << "\n";
            success = false;
        }
        if (z_exact != z_fast) {
            std::cout << "(e = " << e << ") right endpoint is not correct; computed = " << z_fast
                      << "; true_value = " << z_exact[0] << "\n";
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

template <class Float, class CachePolicy>
static bool verify_fast_multiplication_yru(CachePolicy&& cache_policy) {
    using impl = jkj::dragonbox::detail::impl<Float>;
    using format = typename impl::format;
    bool success = true;

    for (int k = impl::min_k; k <= impl::max_k; ++k) {
        auto const cache = cache_policy.template get_cache<format>(k);

        // Since Q - p - beta - 2 >= q, it suffices to check that the lower half of the cache is not 0.
        auto const lower_half = [cache] {
            if constexpr (std::is_same_v<typename impl::format, jkj::dragonbox::ieee754_binary32>) {
                return std::uint32_t(cache);
            }
            else {
                return cache.low();
            }
        }();

        // If the lower half is zero, we need to check if the cache is precise.
        if (lower_half == 0) {
            if (k < 0 || k > jkj::dragonbox::detail::log::floor_log5_pow2(impl::cache_bits)) {
                std::cout << "(k = " << k << ") computation might be incorrect\n";
                success = false;
            }
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

int main() {
    bool success = true;

    std::cout << "[Verifying fast computation of xi and zi for the shorter interval case "
                 "with full cache (binary32)...]\n";
    success &= verify_fast_multiplication_xz<float>(jkj::dragonbox::policy::cache::full);
    std::cout << "Done.\n\n\n";

    std::cout << "[Verifying fast computation of yru for the shorter interval case "
                 "with full cache (binary32)...]\n";
    success &= verify_fast_multiplication_yru<float>(jkj::dragonbox::policy::cache::full);
    std::cout << "Done.\n\n\n";

    std::cout << "[Verifying fast computation of xi and zi for the shorter interval case "
                 "with full cache (binary64)...]\n";
    success &= verify_fast_multiplication_xz<double>(jkj::dragonbox::policy::cache::full);
    std::cout << "Done.\n\n\n";

    std::cout << "[Verifying fast computation of xi and zi for the shorter interval case "
                 "with compressed cache (binary64)...]\n";
    success &= verify_fast_multiplication_xz<double>(jkj::dragonbox::policy::cache::compact);
    std::cout << "Done.\n\n\n";

    std::cout << "[Verifying fast computation of yru for the shorter interval case "
                 "with full cache (binary64)...]\n";
    success &= verify_fast_multiplication_yru<double>(jkj::dragonbox::policy::cache::full);
    std::cout << "Done.\n\n\n";

    std::cout << "[Verifying fast computation of yru for the shorter interval case "
                 "with compressed cache (binary64)...]\n";
    success &= verify_fast_multiplication_yru<double>(jkj::dragonbox::policy::cache::compact);
    std::cout << "Done.\n\n\n";

    if (!success) {
        return -1;
    }
}
