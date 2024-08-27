// Copyright 2024 Junekey Jeon, Toby Bell
// TODO: License

#include "simple_dragonbox.h"
#include "random_float.h"
#include "ryu/ryu.h"

#include <iostream>
#include <string_view>
#include <utility>

static void reference_implementation(float x, char* buffer) { f2s_buffered(x, buffer); }
static void reference_implementation(double x, char* buffer) { d2s_buffered(x, buffer); }

template <class Float, class... Arg>
static bool test_simple_dragonbox(bool& success, Arg... arg) {
    static constexpr char const* type_name = sizeof(Float) == 4 ? "float" : "double";
    static constexpr char const* compact = sizeof...(arg) ? "yes" : "no";
    static constexpr unsigned test_count = 10000000;

    std::cout << "uniform random test, type=" << type_name << " test_count=" << test_count << 
    " compact_cache=" << compact << " ...\n";

    char buffer1[64];
    char buffer2[64];
    auto rg = generate_correctly_seeded_mt19937_64();
    for (std::size_t test_idx = 0; test_idx < test_count; ++test_idx) {
        auto x = uniformly_randomly_generate_general_float<Float>(rg);

        // Check if the output is identical to the reference implementation (Ryu).
        simple_dragonbox::to_chars(x, buffer1, arg...);
        reference_implementation(x, buffer2);

        std::string_view view1(buffer1);
        std::string_view view2(buffer2);

        if (view1 != view2) {
            std::cout << "error detected, reference=" << buffer2 << " dragonbox=" << buffer1 << "\n";
            success = false;
        }
    }

    std::cout << "done.\n";

    std::cout << "test all shorter interval cases, type=" << type_name << " compact_cache=" << compact << " ... \n";

    using format = simple_dragonbox::detail::float_format<Float>;
    using carrier_uint = typename format::carrier_uint;

    for (int e = format::min_exponent; e <= format::max_exponent; ++e) {
        // Compose a floating-point number
        carrier_uint br = carrier_uint(e - format::exponent_bias)
                          << format::significand_bits;

        Float x;
        static_assert(sizeof(br) == sizeof(x));
        ::memcpy(&x, &br, sizeof(br));

        simple_dragonbox::to_chars(x, buffer1, arg...);
        reference_implementation(x, buffer2);

        std::string_view view1(buffer1);
        std::string_view view2(buffer2);

        if (view1 != view2) {
            std::cout << "error detected, reference=" << buffer2 << " dragonbox=" << buffer1 << "\n";
            success = false;
        }
    }

    std::cout << "done.\n";

    return success;
}

int main() {
    bool success = true;
    test_simple_dragonbox<float>(success);
    test_simple_dragonbox<float>(success, simple_dragonbox::policy::cache::compact);
    test_simple_dragonbox<double>(success);
    test_simple_dragonbox<double>(success, simple_dragonbox::policy::cache::compact);
    if (!success)
        return -1;
}

