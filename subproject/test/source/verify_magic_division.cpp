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

#include <iostream>
#include <iomanip>

template <class Float>
static bool verify_check_divisibility_and_divide_by_pow10() {
    using namespace jkj::dragonbox::detail;

    constexpr int kappa = impl<Float>::kappa;
    constexpr auto max_n = compute_power<kappa + 1>(std::uint32_t(10));
    constexpr auto divisor = compute_power<kappa>(std::uint32_t(10));

    bool success = true;
    for (std::uint32_t n = 0; n <= max_n; ++n) {
        std::uint32_t computed_quotient = n;
        auto computed_divisibility =
            div::check_divisibility_and_divide_by_pow10<kappa>(computed_quotient);

        if (computed_quotient != (n / divisor)) {
            std::cout << "Dividing n = " << n << " by " << divisor
                      << "; computed_quotient = " << computed_quotient
                      << "; true_quotient = " << (n / divisor) << std::endl;
            success = false;
        }
        if (computed_divisibility != (n % divisor == 0)) {
            std::cout << "Dividing n = " << n << " by " << divisor
                      << "; computed_divisibility = " << std::boolalpha << computed_divisibility
                      << "; true_divisibility = " << (n % divisor == 0) << std::endl;
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

template <class Float>
static bool verify_divide_by_pow10() {
    using namespace jkj::dragonbox::detail;

    constexpr int kappa = impl<Float>::kappa;
    constexpr auto max_n = compute_power<kappa + 1>(std::uint32_t(10));
    constexpr auto divisor = compute_power<kappa>(std::uint32_t(10));

    bool success = true;
    for (std::uint32_t n = 0; n <= max_n; ++n) {
        auto computed_quotient = div::small_division_by_pow10<kappa>(n);

        if (computed_quotient != (n / divisor)) {
            std::cout << "Dividing n = " << n << " by " << divisor
                      << "; computed_quotient = " << computed_quotient
                      << "; true_quotient = " << (n / divisor) << std::endl;
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

int main() {
    bool success = true;

    std::cout << "[Verifying divisibility check and division by 10^kappa for binary32...]\n";
    success &= verify_check_divisibility_and_divide_by_pow10<float>();
    std::cout << "Done.\n\n\n";

    std::cout << "[Verifying division by 10^kappa for binary32...]\n";
    success &= verify_divide_by_pow10<float>();
    std::cout << "Done.\n\n\n";

    std::cout << "[Verifying divisibility check and division by 10^kappa for binary64...]\n";
    success &= verify_check_divisibility_and_divide_by_pow10<double>();
    std::cout << "Done.\n\n\n";

    std::cout << "[Verifying division by 10^kappa for binary64...]\n";
    success &= verify_divide_by_pow10<double>();
    std::cout << "Done.\n\n\n";

    if (!success) {
        return -1;
    }
}
