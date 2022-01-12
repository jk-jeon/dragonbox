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

#include "best_rational_approx.h"
#include "big_uint.h"
#include "rational_continued_fractions.h"
#include "dragonbox/dragonbox.h"
#include <algorithm>
#include <cstddef>
#include <utility>
#include <vector>

template <class FloatTraits>
bool verify_sufficiency_of_cache_precision() {
    using impl = jkj::dragonbox::detail::impl<typename FloatTraits::type, FloatTraits>;

    auto n_max = jkj::big_uint::power_of_2(impl::significand_bits + 2) - 1;

    jkj::unsigned_rational<jkj::big_uint> target_number;
    for (int k = impl::min_k; k <= impl::max_k; ++k) {
        // (2f_c +- 1) * 2^(beta - 1) * (2^(k - e_k - Q) * 5^k)
        // e_k = floor(k log2(10)) - Q + 1, so
        // k - e_k - Q = k - floor(k log2(10)) - 1.
        int exp_2 = k - jkj::dragonbox::detail::log::floor_log2_pow10(k) - 1;

        target_number.numerator = 1;
        target_number.denominator = 1;
        if (k >= 0) {
            target_number.numerator = jkj::big_uint::pow(5, k);
        }
        else {
            target_number.denominator = jkj::big_uint::pow(5, -k);
        }
        if (exp_2 >= 0) {
            target_number.numerator *= jkj::big_uint::power_of_2(exp_2);
        }
        else {
            target_number.denominator *= jkj::big_uint::power_of_2(-exp_2);
        }

        // Approximate floor(na/b) by nm/2^Q with m = ceil(2^Q * a/b).
        if (target_number.denominator <= n_max) {
            // We need three conditions:
            // (1) When na/b is an integer, nm/2^Q < floor(na/b) + 2^(q-Q).
            //     As m < 2^Q * a/b + 1, we always have
            //     nm/2^Q < na/b + n/2^Q, so it suffices to have n <= 2^q.
            //     This is obviously always true.

            // (2) When na/b is not an integer, floor(na/b) + 2^(q-Q) <= nm/2^Q.
            //     In this case, we always have floor(na/b) <= na/b - 1/b
            //     and nm/2^Q >= na/b, so it suffices to have 2^(q-Q) <= 1/b,
            //     or equivalently b <= 2^(Q-q).
            //     Since b <= n_max < 2^q is assumed, this is also guaranteed
            //     if we take Q >= 2q. We indeed take Q = 2q, so also no problem.

            // (3) nm/2^Q < floor(na/b) + 1.
            //     Recall that we have nm/2^Q < na/b + n/2^Q, so it suffices to
            //     have na/b + n/2^Q <= floor(na/b) + 1.
            //     Since floor(na/b) >= na/b - (1 - 1/b), it suffices to have
            //     n/2^Q <= 1/b, or equivalently, nb <= 2^Q.
            //     Again, as n, b <= n_max < 2^q, Q >= 2q is sufficient.

            // As a result, there is nothing to check in this case.
            static_assert(impl::cache_bits >= 2 * impl::carrier_bits);
            continue;
        }
        else {
            // Again we check the same three conditions.
            // (1) Since na/b is never an integer, so the first condition is vacuous.

            // For the other two conditions, we first obtain
            // the best rational appproximations of a/b from below and above.

            auto [below, above] =
                jkj::find_best_rational_approx<jkj::rational_continued_fractions<jkj::big_uint>>(
                    target_number, n_max);

            // (2) We need to have floor(na/b) + 2^(q-Q) <= nm/2^Q.
            //     Since nm/2^Q is always at least na/b, it suffices to show that
            //     2^(q-Q) <= na/b - floor(na/b).
            //     Since the right-hand side is minimized when floor(na/b)/n is
            //     the best rational approximation of a/b from below, we only need to check
            //     b <= 2^(Q-q) (na - b floor(na/b)) for that case.
            if (target_number.denominator >
                jkj::big_uint::power_of_2(impl::cache_bits - impl::carrier_bits) *
                    (below.denominator * target_number.numerator -
                     target_number.denominator * below.numerator)) {
                return false;
            }

            // (3) We need to have nm/2^Q < floor(na/b) + 1, or equivalently,
            //     m < 2^Q * (floor(na/b) + 1)/n.
            //     Since the right-hand side is minimized when (floor(na/b) + 1)/n
            //     is the best rational approximation of a/b from above, we only need to check
            //     the inequality above for that case.
            auto m = ((jkj::big_uint::power_of_2(impl::cache_bits) * target_number.numerator) /
                      target_number.denominator) +
                     1;
            if (m * above.denominator >=
                jkj::big_uint::power_of_2(impl::cache_bits) * above.numerator) {
                return false;
            }
        }
    }

    return true;
}

#include <iostream>

int main() {
    bool success = true;

    std::cout << "[Verifying sufficiency of cache precision for binary32...]\n";
    if (verify_sufficiency_of_cache_precision<jkj::dragonbox::default_float_traits<float>>()) {
        std::cout << "Verified. " << jkj::dragonbox::detail::impl<float>::cache_bits
                  << "-bits are sufficient.\n\n";
    }
    else {
        std::cout << "Verification failed. " << jkj::dragonbox::detail::impl<float>::cache_bits
                  << "-bits are not sufficient.\n\n";
        success = false;
    }
    
    std::cout << "[Verifying sufficiency of cache precision for binary64...]\n";
    if (verify_sufficiency_of_cache_precision<jkj::dragonbox::default_float_traits<double>>()) {
        std::cout << "Verified. " << jkj::dragonbox::detail::impl<double>::cache_bits
                  << "-bits are sufficient.\n\n";
    }
    else {
        std::cout << "Verification failed. " << jkj::dragonbox::detail::impl<double>::cache_bits
                  << "-bits are not sufficient.\n\n";
        success = false;
    }

    return success ? 0 : -1;
}