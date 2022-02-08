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

#include "big_uint.h"
#include "continued_fractions.h"
#include "dragonbox/dragonbox.h"
#include <algorithm>
#include <utility>
#include <stdexcept>
#include <vector>

template <class FloatTraits>
auto generate_cache() {
    using impl = jkj::dragonbox::detail::impl<typename FloatTraits::type, FloatTraits>;

    std::vector<jkj::big_uint> results;
    jkj::unsigned_rational<jkj::big_uint> target_number;
    for (int k = impl::min_k; k <= impl::max_k; ++k) {
        // (2f_c +- 1) * 2^beta * (2^(k - e_k - Q) * 5^k)
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

        auto div_res = div(jkj::big_uint::power_of_2(impl::cache_bits) * target_number.numerator,
                           target_number.denominator);
        auto m = std::move(div_res.quot);
        if (!div_res.rem.is_zero()) {
            m += 1;
        }

        // Recheck that m is in the correct range.
        if (m < jkj::big_uint::power_of_2(impl::cache_bits - 1) ||
            m >= jkj::big_uint::power_of_2(impl::cache_bits)) {
            throw std::logic_error{"Generated cache entry is not in the correct range"};
        }

        results.push_back(std::move(m));
    }

    return results;
}

#include <fstream>
#include <iomanip>
#include <iostream>

int main() {
    std::cout << "[Generating cache...]\n";

    using jkj::dragonbox::detail::impl;

    auto write_file = [](std::ofstream& out, auto type_tag, auto const& cache_array,
                         auto&& ieee_754_type_name_string, auto&& element_printer) {
        using float_type = typename decltype(type_tag)::type;

        out << "static constexpr int min_k = " << std::dec << impl<float_type>::min_k << ";\n";
        out << "static constexpr int max_k = " << std::dec << impl<float_type>::max_k << ";\n";
        out << "static constexpr cache_entry_type cache[] = {";
        for (int k = impl<float_type>::min_k; k < impl<float_type>::max_k; ++k) {
            auto idx = std::size_t(k - impl<float_type>::min_k);
            out << "\n\t";
            element_printer(out, cache_array[idx]);
            out << ",";
        }
        out << "\n\t";
        element_printer(out, cache_array.back());
        out << "\n};";
    };

    std::ofstream out;

    try {
        out.open("results/binary32_generated_cache.txt");
        auto binary32_cache = generate_cache<jkj::dragonbox::default_float_traits<float>>();
        write_file(out, jkj::dragonbox::default_float_traits<float>{}, binary32_cache, "binary32",
                   [](std::ofstream& out, jkj::big_uint const& value) {
                       out << "0x" << std::hex << std::setw(16) << std::setfill('0') << value[0];
                   });
        out.close();

        out.open("results/binary64_generated_cache.txt");
        auto binary64_cache = generate_cache<jkj::dragonbox::default_float_traits<double>>();
        write_file(out, jkj::dragonbox::default_float_traits<double>{}, binary64_cache, "binary64",
                   [](std::ofstream& out, jkj::big_uint const& value) {
                       out << "{ 0x" << std::hex << std::setw(16) << std::setfill('0') << value[1]
                           << ", 0x" << std::hex << std::setw(16) << std::setfill('0') << value[0]
                           << " }";
                   });
        out.close();
    }
    catch (std::logic_error const& ex) {
        std::cout << ex.what() << "\n";
        return -1;
    }

    std::cout << "Done.\n\n\n";
}
