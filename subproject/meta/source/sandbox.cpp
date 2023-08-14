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

#include "dragonbox/dragonbox_to_chars.h"
#include <iostream>
#include <fstream>
#include <limits>
#include <sstream>
#include <iomanip>

#define ECHO 0

int main() {
    std::string filename;
    std::size_t count = 0;
    std::size_t total_count = 0;
    std::size_t filenum = 0;
    std::ofstream out{"9_digits_case_0.txt"};
    std::uint32_t min_significand = std::numeric_limits<std::uint32_t>::max(), max_significand = 0;
#if ECHO == 1
    std::stringstream ss;
#else
    std::ofstream& ss = out;
#endif
    for (std::uint32_t i = 0x0000'0001; i <= 0x7f7f'ffff; ++i) {
        auto significand_bits = i & (0x007fffff);
        auto exponent_bits = i >> 23;
        auto result = jkj::dragonbox::to_decimal(
            jkj::dragonbox::signed_significand_bits<float>{significand_bits}, exponent_bits,
            jkj::dragonbox::policy::sign::ignore, jkj::dragonbox::policy::trailing_zero::ignore);
        if (result.significand >= 1'0000'0000) {
            if (++count == 500'0000) {
                filename = "9_digits_case_";
                filename += std::to_string(++filenum);
                filename += ".txt";
                out.close();
                out.open(filename);
                total_count += count;
                count = 0;
            }

            if (result.significand < min_significand) {
                min_significand = result.significand;
            }
            if (result.significand > max_significand) {
                max_significand = result.significand;
            }

            float x;
            std::memcpy(&x, &i, sizeof(float));
            char buffer[32];
            *jkj::dragonbox::to_chars_detail::to_chars<float,
                                                       jkj::dragonbox::default_float_traits<float>>(
                result.significand, result.exponent, buffer) = '\0';
            ss << buffer << " (0x" << std::hex << std::setw(8) << std::setfill('0') << i << ")\n";

#if ECHO == 1
            std::cout << ss.str();
            out << ss.str();
            ss.clear();
#endif
        }
    }
    std::cout << "total: " << (total_count += count) << "\n";
    std::cout << "min significand: " << min_significand << "\n";
    std::cout << "max significand: " << max_significand << "\n";
}