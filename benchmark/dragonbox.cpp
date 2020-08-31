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

namespace {
	void dragonbox_float_to_chars(float x, char* buffer)
	{
		jkj::dragonbox::to_chars(x, buffer,
			jkj::dragonbox::rounding_modes::nearest_to_even{},
			jkj::dragonbox::correct_rounding::tie_to_even{});
	}
	void dragonbox_double_to_chars(double x, char* buffer)
	{
		jkj::dragonbox::to_chars(x, buffer,
			jkj::dragonbox::rounding_modes::nearest_to_even{},
			jkj::dragonbox::correct_rounding::tie_to_even{});
	}
	/*register_function_for_benchmark dummy("Dragonbox",
		dragonbox_float_to_chars,
		dragonbox_double_to_chars);*/

	void dragonbox_wo_tzremoval_float_to_chars(float x, char* buffer)
	{
		jkj::dragonbox::to_chars<true>(x, buffer,
			jkj::dragonbox::rounding_modes::nearest_to_even{},
			jkj::dragonbox::correct_rounding::tie_to_even{});
	}
	void dragonbox_wo_tzremoval_double_to_chars(double x, char* buffer)
	{
		jkj::dragonbox::to_chars<true>(x, buffer,
			jkj::dragonbox::rounding_modes::nearest_to_even{},
			jkj::dragonbox::correct_rounding::tie_to_even{});
	}
	register_function_for_benchmark dummy2("Dragonbox (w/o trailing zero removal)",
		dragonbox_wo_tzremoval_float_to_chars,
		dragonbox_wo_tzremoval_double_to_chars);
}