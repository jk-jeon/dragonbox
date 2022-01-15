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
#include <iostream>
#include <limits>
#include <stdexcept>
#include <utility>
#include <vector>

#include <iomanip>

void print_big_uint(std::ostream& out, jkj::big_uint const& n) {
    auto dec = n.to_decimal();
    assert(!dec.empty());

    out << dec.back();

    auto cur_fill = out.fill();
    out << std::setfill('0');

    for (std::size_t back_idx = 0; back_idx < dec.size() - 1; ++back_idx) {
        out << std::setw(19) << dec[dec.size() - back_idx - 2];
    }

    out << std::setfill(cur_fill);
}

struct analysis_result {
    struct result_per_cache_entry {
        int sufficient_bits_for_multiplication;
        int sufficient_bits_for_integer_checks;
        jkj::unsigned_rational<jkj::big_uint> distance_to_upper_bound;
    };
    std::vector<result_per_cache_entry> results;

    struct error_case {
        int e;
        int k;
        jkj::unsigned_rational<jkj::big_uint> target;
        jkj::unsigned_rational<jkj::big_uint> unit;
        std::vector<jkj::big_uint> candidate_multipliers{};
    };
    std::vector<error_case> error_cases;
};

template <class FloatTraits>
bool analyze() {
    using impl = jkj::dragonbox::detail::impl<typename FloatTraits::type, FloatTraits>;
    using namespace jkj::dragonbox::detail::log;

    auto n_max = jkj::big_uint::power_of_2(impl::significand_bits + 2) - 1;

    analysis_result result;
    result.results.resize(impl::max_k - impl::min_k + 1);

    jkj::unsigned_rational<jkj::big_uint> target{1, 1}, unit;
    int prev_k = impl::max_k + 1;
    for (int e = impl::min_exponent - impl::significand_bits;
         e <= impl::max_exponent - impl::significand_bits; ++e) {
        int k = impl::kappa - floor_log10_pow2(e);
        auto exp_2 = k - floor_log2_pow10(k) - 1;
        int beta = e + floor_log2_pow10(k) + 1;

        auto& results_for_k = result.results[k - impl::min_k];

        // target = 2^(k - klog2(10) - 1) * 5^k = a/b in [1/2, 1).
        if (k != prev_k) {
            target.numerator = 1;
            target.denominator = 1;
            if (k >= 0) {
                target.numerator = jkj::big_uint::pow(5, k);
            }
            else {
                target.denominator = jkj::big_uint::pow(5, -k);
            }
            if (exp_2 >= 0) {
                target.numerator *= jkj::big_uint::power_of_2(exp_2);
            }
            else {
                target.denominator *= jkj::big_uint::power_of_2(-exp_2);
            }
        }

        // unit = 2^(e + k - 1) * 5^k = a1/b1.
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


        jkj::unsigned_rational<jkj::big_uint> upper_bound;
        int sufficient_bits_for_integer_checks;
        if (unit.denominator <= n_max) {
            // We must have
            // m/2^Q < (floor(na1 / b1) + 1)/(2^(beta-1) * n) ... (*)
            // so obtain the minimizer of the right-hand side.

            if (unit.denominator == 1) {
                // In this case, (*) is simply
                // m/2^Q < a1/2^(beta-1) + 1/(2^(beta-1) * n), so the minimizer is the biggest n.
                upper_bound = {unit.numerator * n_max + 1,
                               n_max * jkj::big_uint::power_of_2(beta - 1)};
            }
            else {
                // We want to find the largest u <= n_max such that u*a1 == -1 (mod b1).
                // To obtain such u, we first optimize (*) for n < unit.denominator
                // and let the optimizer u0. Then u is u0 + floor((n_max - u0)/b1) * b1.
                auto u0 = jkj::find_best_rational_approx<
                              jkj::rational_continued_fractions<jkj::big_uint>>(
                              unit, unit.denominator - 1)
                              .above.denominator;
                auto u = u0 + ((n_max - u0) / unit.denominator) * unit.denominator;

                auto div_result = div(u * unit.numerator + 1, unit.denominator);
                assert(div_result.rem.is_zero());
                upper_bound = jkj::unsigned_rational<jkj::big_uint>{
                    div_result.quot, u * jkj::big_uint::power_of_2(beta - 1)};
            }

            // A sufficient condition for having successful integer checks is
            // m < 2^Q * a/b + 2^(q-beta+1)/b1 and
            // m >= 2^Q * a/b + (2^q - 2^Q * r/b1)/(2^(beta-1) * n)
            // for any n <= n_max and r = n * a1 - floor(n * a1/b1) * b1 with r != 0.
            // Hence a sufficient condition is:
            // 2^q >= 2^(beta-1) * b1 and 2^Q >= 2^q b1.
            // The first condition is automatically true because of how beta is chosen
            // and that b1 <= n_max.
            sufficient_bits_for_integer_checks =
                impl::carrier_bits + int(jkj::big_uint(1).multiply_2_until(unit.denominator));
        }
        else {
            // m/2^Q < (floor(na1 / b1) + 1)/(2^(beta-1) * n) ... (*)
            // The optimizer is the best rational approximation from above.
            auto [below, above] =
                jkj::find_best_rational_approx<jkj::rational_continued_fractions<jkj::big_uint>>(
                    unit, n_max);

            upper_bound = std::move(above);
            upper_bound.denominator *= jkj::big_uint::power_of_2(beta - 1);

            // A sufficient condition for having successful integer checks is
            // m >= 2^Q * a/b + (2^q - 2^Q * (n * a1/b1 - floor(n * a1/b1))) / (2^(beta-1) * n)
            // for any n <= n_max. Hence, a sufficient condition is
            // m >= 2^Q * a/b + (2^q - 2^Q * d) / 2^(beta-1)
            // where d is the minimum value of n * a1/b1 - floor(n * a1/b1).
            // Or, even more leniently, 2^q <= 2^Q * d is sufficient.
            sufficient_bits_for_integer_checks =
                impl::carrier_bits +
                int((below.denominator * unit.numerator - below.numerator * unit.denominator)
                        .multiply_2_until(unit.denominator));

            // Collect all cases where cache_bits seems insufficient.
            if (sufficient_bits_for_integer_checks > impl::cache_bits) {
                result.error_cases.push_back({e, k, target, unit});
            }
        }

        // Compute the required number of bits for successful multiplication.
        // The following is an upper bound.
        auto div_result = div(upper_bound.denominator * target.denominator,
                              upper_bound.numerator * target.denominator -
                                  upper_bound.denominator * target.numerator);
        if (!div_result.rem.is_zero()) {
            div_result.quot += 1;
        }
        auto sufficient_bits_for_multiplication =
            int(jkj::big_uint(1).multiply_2_until(div_result.quot));

        // Tentatively decrease the above result to find the minimum admissible value.
        while (sufficient_bits_for_multiplication > 0) {
            auto r = (jkj::big_uint::power_of_2(sufficient_bits_for_multiplication - 1) *
                      target.numerator) %
                     target.denominator;
            if (!r.is_zero()) {
                r = target.denominator - r;
            }

            if (r * upper_bound.denominator >=
                jkj::big_uint::power_of_2(sufficient_bits_for_multiplication - 1) *
                    (upper_bound.numerator * target.denominator -
                     upper_bound.denominator * target.numerator)) {
                break;
            }

            --sufficient_bits_for_multiplication;
        }

        // Update.
        if (results_for_k.sufficient_bits_for_multiplication < sufficient_bits_for_multiplication) {
            results_for_k.sufficient_bits_for_multiplication = sufficient_bits_for_multiplication;
        }
        if (results_for_k.sufficient_bits_for_integer_checks < sufficient_bits_for_integer_checks) {
            results_for_k.sufficient_bits_for_integer_checks = sufficient_bits_for_integer_checks;
        }
        auto distance = jkj::unsigned_rational<jkj::big_uint>{
            upper_bound.numerator * target.denominator - upper_bound.denominator * target.numerator,
            upper_bound.denominator * target.denominator};
        if (results_for_k.distance_to_upper_bound.denominator.is_zero()) {
            results_for_k.distance_to_upper_bound = std::move(distance);
        }
        else if (results_for_k.distance_to_upper_bound.numerator * distance.denominator >
            distance.numerator * results_for_k.distance_to_upper_bound.denominator) {
            results_for_k.distance_to_upper_bound = distance;
        }
    }

    // Compute the distances to upper bounds.


    // Analyze all error cases.
    auto reciprocal_error_threshold =
        jkj::big_uint::power_of_2(impl::cache_bits - impl::carrier_bits);
    for (auto& ec : result.error_cases) {
        // We want to find all n such that
        // d:= n * a1/b1 - floor(n * a1/b1) < 2^(q-Q).
        // To do so, we first find the last even convergent p/q of x:=a1/b1 such that
        // qx - p >= 2^(q-Q).
        jkj::rational_continued_fractions<jkj::big_uint> cf{ec.unit};
        jkj::unsigned_rational<jkj::big_uint> previous_convergent, current_convergent;
        while (true) {
            // Odd index.
            previous_convergent = cf.previous_convergent();
            current_convergent = cf.current_convergent();

            if (!cf.update()) {
                // This must not happen.
                assert(false);
            }

            // Even index.
            if (!cf.update()) {
                // This must not happen.
                assert(false);
            }

            // Odd index again.
            // Check if qx - p >= 2^(q-Q) is violated.
            if (reciprocal_error_threshold *
                    (cf.previous_convergent().denominator * ec.unit.numerator -
                     cf.previous_convergent().numerator * ec.unit.denominator) <
                ec.unit.denominator) {
                break;
            }
        }

        // Find the last semiconvergent p/q such that qx - p >= 2^(q-Q).
        // Then the next semiconvergent is the smallest rational number violating qx - p >= 2^(q-Q).
        auto s =
            (reciprocal_error_threshold * (previous_convergent.denominator * ec.unit.numerator -
                                           previous_convergent.numerator * ec.unit.denominator) -
             ec.unit.denominator) /
            (reciprocal_error_threshold * (current_convergent.numerator * ec.unit.denominator -
                                           current_convergent.denominator * ec.unit.numerator));
        s += 1;

        // For each following semiconvergents,
        jkj::unsigned_rational<jkj::big_uint> semiconvergent;
        jkj::big_uint k;

        auto loop_over_semiconvergents = [&]() {
            while (true) {
                semiconvergent.numerator =
                    previous_convergent.numerator + s * current_convergent.numerator;
                semiconvergent.denominator =
                    previous_convergent.denominator + s * current_convergent.denominator;

                // If its denominator is more than n_max, we are done.
                if (semiconvergent.denominator > n_max) {
                    return true;
                }

                // Or, if it is in fact a convergent, break.
                if (semiconvergent.denominator == cf.previous_convergent().denominator) {
                    break;
                }

                ec.candidate_multipliers.push_back(semiconvergent.denominator);

                // Enumerate all numbers of the form
                // (kp_{n-1} + (ks+1)p_{n}) / (kq_{n-1} + (ks+1)q_{n}),
                // where k is any positive integer making the denominator of the above <= n_max.
                if (n_max > current_convergent.denominator) {
                    k = (n_max - current_convergent.denominator) / semiconvergent.denominator;
                    while (!k.is_zero()) {
                        ec.candidate_multipliers.push_back(k * previous_convergent.denominator +
                                                           (k * s + 1) *
                                                               current_convergent.denominator);
                        k -= 1;
                    }
                }

                // Move to the next semiconvergent.
                s += 1;
            }
            return false;
        };

        if (loop_over_semiconvergents()) {
            goto case_ended;
        }

        // For each following convergents,
        while (true) {
            // Move to the next convergent.
            previous_convergent = cf.previous_convergent();
            current_convergent = cf.current_convergent();

            if (!cf.update()) {
                // This must not happen.
                assert(false);
            }

            // For each semiconvergents,
            s = 0;
            if (loop_over_semiconvergents()) {
                goto case_ended;
            }
        }
    case_ended:;
    }

    auto sufficient_bits_for_multiplication =
        std::max_element(result.results.cbegin(), result.results.cend(),
                          [](auto const& a, auto const& b) {
                              return a.sufficient_bits_for_multiplication <
                                     b.sufficient_bits_for_multiplication;
                          })
             ->sufficient_bits_for_multiplication;
    auto sufficient_bits_for_integer_checks =
        std::max_element(result.results.cbegin(), result.results.cend(),
                          [](auto const& a, auto const& b) {
                              return a.sufficient_bits_for_integer_checks <
                                     b.sufficient_bits_for_integer_checks;
                          })
             ->sufficient_bits_for_integer_checks;
    auto larger = std::max(sufficient_bits_for_multiplication, sufficient_bits_for_integer_checks);

    auto distance_to_upper_bound =
        std::min_element(
            result.results.cbegin(), result.results.cend(),
            [](auto const& a, auto const& b) {
                if (a.distance_to_upper_bound.denominator == 0) {
                    return false;
                }
                else if (b.distance_to_upper_bound.denominator == 0) {
                    return true;
                }
                return a.distance_to_upper_bound.numerator * b.distance_to_upper_bound.denominator <
                       b.distance_to_upper_bound.numerator * a.distance_to_upper_bound.denominator;
            })
            ->distance_to_upper_bound;

    // Reduce the fraction.
    distance_to_upper_bound =
        jkj::find_best_rational_approx<jkj::rational_continued_fractions<jkj::big_uint>>(
            distance_to_upper_bound, distance_to_upper_bound.denominator).below;

    std::cout << "An upper bound on the minimum required bits for successful multiplication is "
              << sufficient_bits_for_multiplication
              << "-bits.\nAn upper bound on the minimum required bits for successful integer "
                 "checks is "
              << sufficient_bits_for_integer_checks << "-bits.\n";
    std::cout << "A lower bound on the margin is ";
    print_big_uint(std::cout, distance_to_upper_bound.numerator);
    std::cout << " / ";
    print_big_uint(std::cout, distance_to_upper_bound.denominator);
    std::cout << ".\n";

    if (impl::cache_bits < larger) {
        auto success = true;
        std::cout << "Error cases:\n";
        auto threshold = jkj::big_uint::power_of_2(impl::significand_bits + 1) - 2;
        for (auto const& ec : result.error_cases) {
            for (auto const& n : ec.candidate_multipliers) {
                std::cout << "  e: " << ec.e << "  k: " << ec.k << "  n: ";
                print_big_uint(std::cout, n);

                // When e != min_e, n must be at least 2^(p+1)-2, otherwise this is a false
                // positive.

                if (ec.e != impl::min_exponent - impl::significand_bits && n < threshold) {
                    std::cout << "\n    n is smaller than ";
                    print_big_uint(std::cout, threshold);
                    std::cout << ", so this case is a false positive.";
                }
                else if (ec.e == -81 && n == 29711844) {
                    std::cout << "\n    This case has been carefully addressed.";
                }
                else {
                    success = false;
                }

                std::cout << "\n";
            }
        }

        if (!success) {
            std::cout << "Verification failed. " << impl::cache_bits
                      << "-bits are not sufficient.\n\n";
            return false;
        }
    }

    std::cout << "Verified. " << impl::cache_bits << "-bits are sufficient.\n\n";
    return true;
}



int main() {
    bool success = true;

    std::cout << "[Verifying sufficiency of cache precision for binary32...]\n";
    if (!analyze<jkj::dragonbox::default_float_traits<float>>()) {
        success = false;
    }

    std::cout << "[Verifying sufficiency of cache precision for binary64...]\n";
    if (!analyze<jkj::dragonbox::default_float_traits<double>>()) {
        success = false;
    }

    return success ? 0 : -1;
}