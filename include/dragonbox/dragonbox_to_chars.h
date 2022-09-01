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

#ifndef JKJ_HEADER_DRAGONBOX_TO_CHARS
#define JKJ_HEADER_DRAGONBOX_TO_CHARS

#include "dragonbox/dragonbox.h"

namespace jkj::dragonbox {
    namespace to_chars_detail {
        template <class Float, class FloatTraits>
        extern char* to_chars(typename FloatTraits::carrier_uint significand, int exponent,
                              char* buffer) noexcept;

        // Avoid needless ABI overhead incurred by tag dispatch.
        template <class PolicyHolder, class Float, class FloatTraits>
        char* to_chars_n_impl(float_bits<Float, FloatTraits> br, char* buffer) noexcept {
            auto const exponent_bits = br.extract_exponent_bits();
            auto const s = br.remove_exponent_bits(exponent_bits);

            if (br.is_finite(exponent_bits)) {
                if (s.is_negative()) {
                    *buffer = '-';
                    ++buffer;
                }
                if (br.is_nonzero()) {
                    auto result = to_decimal<Float, FloatTraits>(
                        s, exponent_bits, policy::sign::ignore, policy::trailing_zero::ignore,
                        typename PolicyHolder::decimal_to_binary_rounding_policy{},
                        typename PolicyHolder::binary_to_decimal_rounding_policy{},
                        typename PolicyHolder::cache_policy{});
                    return to_chars_detail::to_chars<Float, FloatTraits>(result.significand,
                                                                         result.exponent, buffer);
                }
                else {
                    std::memcpy(buffer, "0E0", 3);
                    return buffer + 3;
                }
            }
            else {
                if (s.has_all_zero_significand_bits()) {
                    if (s.is_negative()) {
                        *buffer = '-';
                        ++buffer;
                    }
                    std::memcpy(buffer, "Infinity", 8);
                    return buffer + 8;
                }
                else {
                    std::memcpy(buffer, "NaN", 3);
                    return buffer + 3;
                }
            }
        }
    }

    // Returns the next-to-end position
    template <class Float, class FloatTraits = default_float_traits<Float>, class... Policies>
    char* to_chars_n(Float x, char* buffer, Policies... policies) noexcept {
        using namespace jkj::dragonbox::detail::policy_impl;
        using policy_holder = decltype(make_policy_holder(
            base_default_pair_list<base_default_pair<decimal_to_binary_rounding::base,
                                                     decimal_to_binary_rounding::nearest_to_even>,
                                   base_default_pair<binary_to_decimal_rounding::base,
                                                     binary_to_decimal_rounding::to_even>,
                                   base_default_pair<cache::base, cache::full>>{},
            policies...));

        return to_chars_detail::to_chars_n_impl<policy_holder>(float_bits<Float, FloatTraits>(x),
                                                               buffer);
    }

    // Null-terminate and bypass the return value of fp_to_chars_n
    template <class Float, class FloatTraits = default_float_traits<Float>, class... Policies>
    char* to_chars(Float x, char* buffer, Policies... policies) noexcept {
        auto ptr = to_chars_n<Float, FloatTraits>(x, buffer, policies...);
        *ptr = '\0';
        return ptr;
    }

    // Maximum required buffer size (excluding null-terminator)
    template <class FloatFormat>
    inline constexpr std::size_t max_output_string_length =
        std::is_same_v<FloatFormat, ieee754_binary32>
            ?
            // sign(1) + significand(9) + decimal_point(1) + exp_marker(1) + exp_sign(1) + exp(2)
            (1 + 9 + 1 + 1 + 1 + 2)
            :
            // format == ieee754_format::binary64
            // sign(1) + significand(17) + decimal_point(1) + exp_marker(1) + exp_sign(1) + exp(3)
            (1 + 17 + 1 + 1 + 1 + 3);
}

#endif
