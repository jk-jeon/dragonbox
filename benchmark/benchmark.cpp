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

#include "benchmark.h"
#include "../dragonbox_to_chars.h"
#include <cstring>
#include <fstream>
#include <iomanip>

#define RUN_MATLAB
#ifdef RUN_MATLAB
#include <cstdlib>

void run_matlab() {
	struct launcher {
		~launcher() {
			std::system("matlab -nosplash -r \"cd('benchmark_results'); plot_benchmarks\"");
		}
	};
	static launcher l;
}
#endif

template <class Float>
static void benchmark_test(std::string_view float_name,
	std::size_t number_of_uniform_samples, std::size_t number_of_digits_samples_per_digits,
	std::size_t number_of_iterations)
{
	auto& inst = benchmark_holder<Float>::get_instance();
	std::cout << "Generating random samples...\n";
	inst.prepare_samples(number_of_uniform_samples, number_of_digits_samples_per_digits);
	auto out = inst.run(number_of_iterations, float_name);

	std::cout << "Benchmarking done.\n" << "Now writing to files...\n";

	// Write uniform benchmark results
	auto filename = std::string("benchmark_results/uniform_benchmark_");
	filename += float_name;
	filename += ".csv";
	std::ofstream out_file{ filename };
	out_file << "number_of_samples," << number_of_uniform_samples << std::endl;;
	out_file << "name,sample,bit_representation,time\n";

	char buffer[64];
	typename jkj::dragonbox::ieee754_traits<Float>::carrier_uint br;
	for (auto& name_result_pair : out) {
		for (auto const& data_time_pair : name_result_pair.second[0]) {
			std::memcpy(&br, &data_time_pair.first, sizeof(Float));
			jkj::dragonbox::to_chars(data_time_pair.first, buffer);
			out_file << "\"" << name_result_pair.first << "\"," << buffer << "," <<
				"0x" << std::hex << std::setfill('0');
			if constexpr (sizeof(Float) == 4)
				out_file << std::setw(8);
			else
				out_file << std::setw(16);
			out_file << br << std::dec << "," << data_time_pair.second << "\n";
		}
	}
	out_file.close();

	// Write digits benchmark results
	filename = std::string("benchmark_results/digits_benchmark_");
	filename += float_name;
	filename += ".csv";
	out_file.open(filename);
	out_file << "number_of_samples_per_digits," << number_of_digits_samples_per_digits << std::endl;;
	out_file << "name,digits,sample,time\n";

	for (auto& name_result_pair : out) {
		for (unsigned int digits = 1; digits <= benchmark_holder<Float>::max_digits; ++digits) {
			for (auto const& data_time_pair : name_result_pair.second[digits]) {
				jkj::dragonbox::to_chars(data_time_pair.first, buffer);
				out_file << "\"" << name_result_pair.first << "\"," << digits << "," <<
					buffer << "," << data_time_pair.second << "\n";
			}
		}
		
	}
	out_file.close();

#ifdef RUN_MATLAB
	run_matlab();
#endif
}

void benchmark_test_float(std::size_t number_of_uniform_samples,
	std::size_t number_of_digits_samples_per_digits, std::size_t number_of_iterations)
{
	std::cout << "[Running benchmark for binary32...]\n";
	benchmark_test<float>("binary32", number_of_uniform_samples,
		number_of_digits_samples_per_digits, number_of_iterations);
	std::cout << "Done.\n\n\n";
}

void benchmark_test_double(std::size_t number_of_uniform_samples,
	std::size_t number_of_digits_samples_per_digits, std::size_t number_of_iterations)
{
	std::cout << "[Running benchmark for binary64...]\n";
	benchmark_test<double>("binary64", number_of_uniform_samples,
		number_of_digits_samples_per_digits, number_of_iterations);
	std::cout << "Done.\n\n\n";
}