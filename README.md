# backport::expected

C++17 workalike for C++23 std::expected.

## Use

The file `include/backport/expected.h` contains the `std::expected` workalike
implementation in the namespace `backport`.

It provides templated classes `expected`, `unexpected`, the exception class
`bad_expected_access`, the class `unexpect_t` and inline constexpr value
`unexpect`.

Implementation is complete, but not completely tested.

## Building and running the unit tests

The provided `Makefile` is designed to be used for an out-of-tree build.

The make target `unit` will build the unit test, while the phony target
`test` will build and run the test. An example build and test run:

```
% mkdir build
% cd build
% ln -s  ../Makefile
% make -j unit
[...]
% ./unit
```

`make clean` will remove generated object files and coverage data;
`make realclean` will also remove the `unit` executable and any `.d` dependency files.

By default, the unit tests will be built for C++17. The Makefile looks for
the make or environment variable CXXSTD; this can be overridden at build time:

```
% make realclean
% CXXSTD=c++20 make -j unit
```

## Producing test coverage report

If the make variable `coverage` is defined, the unit test will be built with
coverage enabled and the `test` target will attempt to use `gcovr` to produce
an annotated `expected.h` in the file `coverage.expected.h.html`.

```
% make coverage=true test -j
[...]
Generating gcovr detailed report: coverage.expected.h.html
[...]
% xdg-open coverage.expected.h.html
```

