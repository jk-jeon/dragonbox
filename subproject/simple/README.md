# Simple Dragonbox

This is a simplified implementation of the algorithm, that closely follows the
one in `include/dragonbox/dragonbox.h`, but aims to be shorter overall, use less
C++ template indirection, and offer less flexibility and performance for the
sake of simplicity.

Following simplifications over the implementation in `include/dragonbox/dragonbox.h` are made:

1. No support of policies (always uses default policies).
2. `int` is assumed to be of at least 32-bits.
3. `float` and `double` are assumed to be of [IEEE-754 binary32](https://en.wikipedia.org/wiki/Single-precision_floating-point_format) and [IEEE-754 binary64](https://en.wikipedia.org/wiki/Double-precision_floating-point_format) representations,
  respectively.
4. No use of compiler intrinsics for 128-bit arithmetics (always uses portable fallback implementations).
5. Straightforward digit character generation without any fancy tricks.
6. `if constexpr` is available (C++17).
7. No support of `constexpr`.
8. No `noexcept` annotations.
9. No forced inlining.
10. Modern compiler and standard library are assumed to be available.

The points 4 and 5 have critical impact on the performance, so any serious port of Dragonbox
that aims for excellent performance must re-address them.

# Usage Examples

Simple string generation from `float`/`double` (mirrors the interface and
behavior of `jkj::dragonbox::to_chars`; see the [primary README](/README.md)):

```cpp
#include "simple_dragonbox.h"

constexpr int buffer_length = 1 + // for '\0'
  jkj::simple_dragonbox::max_output_string_length<double>;
double x = 1.234;  // Also works for float
char buffer[buffer_length];

// Null-terminate the buffer and return the pointer to the null character.
// Hence, the length of the string is `end - buffer`.
// `buffer` is now { '1', '.', '2', '3', '4', 'E', '0', '\0', (garbage) }.
char* end = jkj::simple_dragonbox::to_chars(x, buffer);
```

Direct use of `jkj::simple_dragonbox::to_decimal` (mirrors the interface and
behavior of `jkj::dragonbox::to_decimal`; see the [primary README](/README.md)):

```cpp
#include "simple_dragonbox.h"
double x = 1.234;   // Also works for float

// `x` must be a nonzero finite number.
// `v` is a struct with three members:
// significand : decimal significand (1234 in this case);
//               it is of type std::uint64_t for double, std::uint32_t for float
//    exponent : decimal exponent (-3 in this case); it is of type int
// is_negative : as the name suggests; it is of type bool
auto v = jkj::simple_dragonbox::to_decimal(x);
```

Like `jkj::dragonbox::to_decimal`, `jkj::simple_dragonbox::to_decimal` only works
with finite nonzero inputs. Its behavior when given infinities, NaNs, and ±0 is
undefined. `jkj::simple_dragonbox::to_chars` works fine for any inputs.
