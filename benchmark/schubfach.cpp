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
#include "schubfach/schubfach_32.h"
#include "schubfach/schubfach_64.h"

namespace {
	void schubfach_32(float x, char* buf) {
		schubfach::Ftoa(buf, x);
	}
	void schubfach_64(double x, char* buf) {
		schubfach::Dtoa(buf, x);
	}
	register_function_for_benchmark dummy("Schubfach", schubfach_32, schubfach_64);
}