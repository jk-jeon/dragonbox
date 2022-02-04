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

#include "big_uint.h"
#include "dragonbox/dragonbox.h"

#include <functional>
#include <iostream>

int floor_log10_pow2_precise(int e) {
    using namespace jkj::dragonbox::detail::log;
    bool is_negative;
    if (e < 0) {
        is_negative = true;
        e = -e;
    }
    else {
        is_negative = false;
    }

    auto power_of_2 = jkj::big_uint::power_of_2(std::size_t(e));
    auto power_of_10 = jkj::big_uint(1);
    int k = 0;
    while (power_of_10 <= power_of_2) {
        power_of_10.multiply_5();
        power_of_10.multiply_2();
        ++k;
    }

    return is_negative ? -k : k - 1;
}

int floor_log10_pow2_minus_log10_4_over_3_precise(int e) {
    e -= 2;

    if (e < 0) {
        e = -e;
        auto power_of_2 = jkj::big_uint::power_of_2(std::size_t(e));
        auto power_of_10_times_3 = jkj::big_uint(3);
        int k = 0;
        while (power_of_10_times_3 < power_of_2) {
            power_of_10_times_3.multiply_5();
            power_of_10_times_3.multiply_2();
            ++k;
        }
        return -k;
    }
    else {
        auto power_of_2_times_3 = jkj::big_uint::power_of_2(std::size_t(e)) * 3;
        auto power_of_10 = jkj::big_uint(1);
        int k = 0;
        while (power_of_10 <= power_of_2_times_3) {
            power_of_10.multiply_5();
            power_of_10.multiply_2();
            ++k;
        }
        return k - 1;
    }
}

int floor_log2_pow10_precise(int e) {
    bool is_negative;
    if (e < 0) {
        is_negative = true;
        e = -e;
    }
    else {
        is_negative = false;
    }

    auto power_of_10 = jkj::big_uint(1);
    for (int i = 0; i < e; ++i) {
        power_of_10.multiply_5();
        power_of_10.multiply_2();
    }

    auto k = int(log2p1(power_of_10));

    return is_negative ? -k : k - 1;
}

int floor_log5_pow2_precise(int e) {
    bool is_negative;
    if (e < 0) {
        is_negative = true;
        e = -e;
    }
    else {
        is_negative = false;
    }

    auto power_of_2 = jkj::big_uint::power_of_2(std::size_t(e));
    auto power_of_5 = jkj::big_uint(1);
    int k = 0;
    while (power_of_5 <= power_of_2) {
        power_of_5.multiply_5();
        ++k;
    }

    return is_negative ? -k : k - 1;
}

int floor_log5_pow2_minus_log5_3_precise(int e) {
    if (e >= 0) {
        auto power_of_2 = jkj::big_uint::power_of_2(std::size_t(e));
        auto power_of_5_times_3 = jkj::big_uint(3);
        int k = 0;
        while (power_of_5_times_3 <= power_of_2) {
            power_of_5_times_3.multiply_5();
            ++k;
        }
        return k - 1;
    }
    else {
        e = -e;
        auto power_of_2_times_3 = jkj::big_uint::power_of_2(std::size_t(e)) * 3;
        auto power_of_5 = jkj::big_uint(1);
        int k = 0;
        while (power_of_5 < power_of_2_times_3) {
            power_of_5.multiply_5();
            ++k;
        }
        return -k;
    }
}

struct verify_result {
    int min_exponent;
    int max_exponent;
};

template <jkj::dragonbox::detail::log::multiply m, jkj::dragonbox::detail::log::subtract f,
          jkj::dragonbox::detail::log::shift k>
verify_result verify(std::string_view name, std::function<int(int)> precise_calculator = nullptr) {
    // Compute the maximum possible e
    constexpr auto max_exponent_upper_bound =
        std::numeric_limits<std::int32_t>::max() / std::int32_t(m);
    constexpr auto min_exponent_lower_bound =
        -std::int32_t(-std::int64_t(std::numeric_limits<std::int32_t>::min() + std::int32_t(f)) /
                      std::int32_t(m));

    verify_result result{int(min_exponent_lower_bound), int(max_exponent_upper_bound)};

    bool reach_upper_bound = false;
    bool reach_lower_bound = false;
    for (std::int32_t e = 0; e <= std::max(-min_exponent_lower_bound, max_exponent_upper_bound);
         ++e) {
        if (!reach_upper_bound) {
            auto true_value = precise_calculator(int(e));
            auto computed_value = int((e * std::int32_t(m) - std::int32_t(f)) >> std::size_t(k));
            if (computed_value != true_value) {
                std::cout << "  - error with positive e ("
                          << "e: " << e << ", true value: " << true_value
                          << ", computed value: " << computed_value << ")\n";

                reach_upper_bound = true;
                result.max_exponent = int(e) - 1;
            }
        }

        if (!reach_lower_bound) {
            auto true_value = precise_calculator(-int(e));
            auto computed_value = int((-e * std::int32_t(m) - std::int32_t(f)) >> std::size_t(k));
            if (computed_value != true_value) {
                std::cout << "  - error with negative e ("
                          << "e: " << (-int(e)) << ", true value: " << true_value
                          << ", computed value: " << computed_value << ")\n";

                reach_lower_bound = true;
                result.min_exponent = -int(e) + 1;
            }
        }

        if (reach_upper_bound && reach_lower_bound) {
            break;
        }
    }

    std::cout << name << " is correct for e in [" << result.min_exponent << ", "
              << result.max_exponent << "]\n\n";

    return result;
}

int main() {
    using namespace jkj::dragonbox::detail::log;

    bool success = true;
    std::cout << "[Verifying log computation...]\n";

    {
        auto result = verify<multiply(315653), subtract(0), shift(20)>("floor_log10_pow2",
                                                                       floor_log10_pow2_precise);
        if (result.min_exponent > floor_log10_pow2_min_exponent ||
            result.max_exponent < floor_log10_pow2_max_exponent) {
            success = false;
        }
    }
    {
        auto result = verify<multiply(1741647), subtract(0), shift(19)>("floor_log2_pow10",
                                                                        floor_log2_pow10_precise);
        if (result.min_exponent > floor_log2_pow10_min_exponent ||
            result.max_exponent < floor_log2_pow10_max_exponent) {
            success = false;
        }
    }
    {
        auto result = verify<multiply(631305), subtract(261663), shift(21)>(
            "floor_log10_pow2_minus_log10_4_over_3", floor_log10_pow2_minus_log10_4_over_3_precise);
        if (result.min_exponent > floor_log10_pow2_minus_log10_4_over_3_min_exponent ||
            result.max_exponent < floor_log10_pow2_minus_log10_4_over_3_max_exponent) {
            success = false;
        }
    }
    {
        auto result = verify<multiply(225799), subtract(0), shift(19)>("floor_log5_pow2",
                                                                       floor_log5_pow2_precise);
        if (result.min_exponent > floor_log5_pow2_min_exponent ||
            result.max_exponent < floor_log5_pow2_max_exponent) {
            success = false;
        }
    }
    {
        auto result = verify<multiply(451597), subtract(715764), shift(20)>(
            "floor_log5_pow2_minus_log5_3", floor_log5_pow2_minus_log5_3_precise);
        if (result.min_exponent > floor_log5_pow2_minus_log5_3_min_exponent ||
            result.max_exponent < floor_log5_pow2_minus_log5_3_max_exponent) {
            success = false;
        }
    }

    std::cout << "Done.\n\n\n";

    if (!success) {
        return -1;
    }
}
