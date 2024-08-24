# Simple Dragonbox

This is a simplified implementation of the algorithm, that closely follows the
one in `include/dragonbox/dragonbox.h`, but aims to be shorter overall, use less
C++ template indirection, and offer slightly less flexibility for the benefit of
simpliciy.

Primary sacrifices over the implementation in `include/dragonbox/dragonbox.h`:

- No support for `sign` policies (always uses `return_sign`)
- No support for `trailing_zeros` policies (always uses `remove`)
- No support for 128-bit compiler intrinsics (always uses portable
  implementation)
- No support for fast digit-generation policy (always uses `compact`)
- Assumes the existence of `if constexpr` (C++17)
- Assumes `float` and `double` types are available and use IEEE-754
- Generally assumes a modern standard compiler environment and C standard
  library

Note the `cache`, `binary_to_decimal_rounding`, and `decimal_to_binary_rounding`
policies are still supported.

## API

### Basic use

To convert to decimal (analog to `jkj::dragonbox::to_decimal`),

```cpp
float x;
auto d = simple_dragonbox::to_decimal(x);
(void) d.significand;
(void) d.exponent;
(void) d.sign;
```

To convert to string (analog to `jkj::dragonbox::to_chars`),

```cpp
float x;
char buf[simple_dragonbox::max_output_string_length<float> + 1];
simple_dragonbox::to_chars(x, buf);
std::cout << buf << std::endl;
```

In the examples above, `x` can be either a `float` or a `double`.

### Policies

`simple_dragonbox` supports the same style of policy interface as
`jkj::dragonbox`. For example,

```cpp
simple_dragonbox::to_decimal(3.14,
    simple_dragonbox::policy::cache::compact,
    simple_dragonbox::policy::binary_to_decimal_rounding::to_odd);
```

Supported policies include:

`simple_dragonbox::policy::decimal_to_binary_rounding`

- `::nearest_to_even`
- `::nearest_to_odd`
- `::nearest_toward_plus_infinity`
- `::nearest_toward_minus_infinity`
- `::nearest_toward_zero`
- `::nearest_away_from_zero`
- `::nearest_to_even_static_boundary`
- `::nearest_to_odd_static_boundary`
- `::nearest_toward_plus_infinity_static_boundary`
- `::nearest_toward_minus_infinity_static_boundary`
- `::toward_plus_infinity`
- `::toward_minus_infinity`
- `::toward_zero`
- `::away_from_zero`

`simple_dragonbox::policy::binary_to_decimal_rounding`

- `::to_even`
- `::to_odd`
- `::away_from_zero`
- `::toward_zero`
- `::do_not_care`

`simple_dragonbox::policy::cache`

- `::full`
- `::compact`

### Internal/direct API

Internally, `simple_dragonbox` uses a more explicit, direct interface to express
the various policies, via a class called `simple_dragonbox::impl`. The `impl`
class has four template parameters: `Float`, `BinaryRoundMode`,
`DecimalRoundMode`, and `CacheType`, which correspond to the floating point
type and three categories of policies above. These template parameters can be
specified explicitly to instantiate a particular variant of the algorithm. The
class has a single constructor that receives a value of type `Float` (e.g.
`float` or `double`), and has methods `to_decimal()` and `to_chars()` for
performing the desired conversions.

For example,

```cpp
using impl = simple_dragonbox::impl<double,
                                    simple_dragonbox::binary_round_mode::toward_zero,
                                    simple_dragonbox::decimal_round_mode::toward_zero,
                                    simple_dragonbox::cache_type::compact>;

int main() {
  char buf[32];
  auto x = impl(3.14);
  x.to_decimal();  // equivalent to `simple_dragonbox::to_decimal(3.14, policies...)`
  x.to_chars(buf);  // equivalent to `simple_dragonbox::to_chars(3.14, buf, policies...)`
}
```
