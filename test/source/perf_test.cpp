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

#include "../dragonbox_to_chars.h"
#include "random_float.h"
#include <iostream>

template <class Float>
static void uniform_random_perf_test(std::size_t number_of_tests)
{
	char buffer[41];
	auto rg = generate_correctly_seeded_mt19937_64();
	for (std::size_t test_idx = 0; test_idx < number_of_tests; ++test_idx) {
		auto x = uniformly_randomly_generate_general_float<Float>(rg);
		jkj::dragonbox::to_chars(x, buffer);
	}
}

void uniform_random_perf_test_float(std::size_t number_of_tests) {
	std::cout << "[Running the algorithm with uniformly randomly generated float inputs...]\n";
	uniform_random_perf_test<float>(number_of_tests);
	std::cout << "Done.\n\n\n";
}
void uniform_random_perf_test_double(std::size_t number_of_tests) {
	std::cout << "[Running the algorithm with uniformly randomly generated double inputs...]\n";
	uniform_random_perf_test<double>(number_of_tests);
	std::cout << "Done.\n\n\n";
}

template <class Float>
static void digit_perf_test(unsigned int digits, std::size_t number_of_tests)
{
	char buffer[41];
	auto rg = generate_correctly_seeded_mt19937_64();
	for (std::size_t test_idx = 0; test_idx < number_of_tests; ++test_idx) {
		auto x = randomly_generate_float_with_given_digits<Float>(digits, rg);
		jkj::dragonbox::to_chars(x, buffer);
	}
}

void digit_perf_test_float(unsigned int digits, std::size_t number_of_tests) {
	std::cout << "[Running the algorithm with float inputs of digits = " << digits << "...]\n";
	digit_perf_test<float>(digits, number_of_tests);
	std::cout << "Done.\n\n\n";
}
void digit_perf_test_double(unsigned int digits, std::size_t number_of_tests) {
	std::cout << "[Running the algorithm with double inputs of digits = " << digits << "...]\n";
	digit_perf_test<double>(digits, number_of_tests);
	std::cout << "Done.\n\n\n";
}