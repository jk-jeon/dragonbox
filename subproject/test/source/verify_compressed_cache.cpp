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

int main()
{
	using namespace jkj::dragonbox::detail;

	std::cout << "[Verifying recovery from compressed cache...]\n";

	bool success = true;
	for (int k = impl<double>::min_k; k <= impl<double>::max_k; ++k)
	{
		using jkj::dragonbox::policy::cache::normal;
		using jkj::dragonbox::policy::cache::compressed;

		auto real_cache = normal.get_cache<jkj::dragonbox::ieee754_format::binary64>(k);
		auto recovered_cache = compressed.get_cache<jkj::dragonbox::ieee754_format::binary64>(k);

		if (real_cache.high() != recovered_cache.high() ||
			real_cache.low() != recovered_cache.low())
		{
			std::cout << "Mismatch! (k = " << k << ")\n";
			success = false;
		}
	}

	if (success) {
		std::cout << "Recovered cache is the same as the original cache.\n";
	}
	else {
		std::cout << "Error detected.\n";
	}

	std::cout << "Done.\n\n\n";

	if (!success) {
		return -1;
	}
}
