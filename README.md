Compile time formatting
=======================

An experimental C++26 compile-time formatting library based on ``std::format``.
The current version avoids run-time parsing of the _format string_. This gives
a significant speedup when formatting the output run-time. In several initial
tests the speedup was around a factor 2 per call.


Motivation
----------

``std::format`` is a great library but has one major drawback; it validates the
_format string_ at compile-time, forgets all about it and parses again at
run-time. At least for the Standard library formatters the compile-time
information could be stored and reused during formatting. Not calling the
parsing code run-time stores less instantiations in the final binary.

Another part of the motivation is to see how far compile-time formatting can be
taken and hopefully find some additional improvements for libc++'s
``std::format`` implementation.


Limitations
-----------

- Only ``std::format`` for ``char``s is supported. So no support for ``wchar_t``,
  ``std::vformat``, ``std::format_to``, ``std::print``, no locale overloads, etc..
- The library has an additional pre-condition on ``std::formatter``
  specializations for custom types. These specializations should only used
  balanced ``{}`` pairs.
- The project requires C++26. It should be possible to rewrite most of it to
  use C++20, but that is not a goal. The main reason to require C++26 feature
  is being able to improve the diagnostics by using
  [User-generated static_assert messages](https://wg21.link/P2741R3).
- No compile-time formatting is possible yet.
- No installation support yet.
- No C++ module support yet.


Notes
-----

The current implementation has some "non-standard" behaviour internally so this
can't be used in a Standard library implementation other than libc++.

The initial version used a stateless type list, this doesn't work for stateful
parsers. For that reason the implementation uses ``std::tuple``. This change
has a bad impact on the compile-time of the code.


Licence
-------

The library copies parts of libc++'s ``std::format`` implementation and their
tests. The project is available using the same license as libc++; the
Apache-2.0 license with the LLVM-exception. See LICENSE.TXT for the complete
license.


Dependencies
------------

 * CMake 3.28
 * The code is developed using the development versions of Clang and libc++
 * The code is expected to work with version 17.0 of Clang and libc++


Benchmarks
----------

These numbers compare the compile-time and run-time benchmarks. Both benchmarks
do the same. They are in different files to easy the comparison of the
binaries and their contents.

Since the tests are quite small it's expected the build times are similar. The
build time of the unit test is probably more interesting; however there is no
comparable ``std::format`` based test.

### CTF

#### Output

|               ns/op |                op/s |    err% |     total | Compile-time
|--------------------:|--------------------:|--------:|----------:|:-------------
|               15.09 |       66,247,243.48 |    0.0% |      0.01 | `basic`
|               30.12 |       33,205,737.08 |    0.3% |      0.01 | `answer`
|               93.96 |       10,643,094.38 |    0.1% |      0.03 | `triple`
|               96.17 |       10,398,007.86 |    1.1% |      0.03 | `longer text`


#### Build time
```
real 0m5.315s
user 0m9.642s
sys  0m0.290s
```

#### size
```
   text	   data	    bss	    dec	    hex	filename
 169348	   1736	    112	 171196	  29cbc	benchmark/compile-time
```


### ``std::format``

#### Output

|               ns/op |                op/s |    err% |     total | Run-time
|--------------------:|--------------------:|--------:|----------:|:---------
|               35.21 |       28,398,502.43 |    0.6% |      0.01 | `basic`
|               56.11 |       17,821,852.48 |    0.2% |      0.02 | `answer`
|              177.60 |        5,630,732.62 |    0.4% |      0.05 | `triple`
|              171.61 |        5,827,019.35 |    0.6% |      0.05 | `longer text`


#### Build time
```
real 0m4.552s
user 0m7.693s
sys	 0m0.222s
```

#### Size
```
   text	   data	    bss	    dec	    hex	filename
 214654	   1800	    112	 216566	  34df6	benchmark/run-time
```
