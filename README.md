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


Features
--------

### Improved execution speed

See the benchmarks at the end of the page.

### Improved diagnostics

Part of the parsing engine have been rewritten to allow better diagnostics. For example:

```
In file included from …/test/diagnostics.cpp:12:
…/include/ctf/format.hpp:553:19: error: static assertion failed due to requirement '!"parse error"':
using the second argument while one argument is available
{1}
 ~^

  553 |     static_assert(!"parse error", status);
      |                   ^~~~~~~~~~~~~~
…/test/diagnostics.cpp:65:14: note: in instantiation of function template specialization 'ctf::format<fixed_string<4UL>{"{1}"}, int>' requested here
   65 |   (void)ctf::format<"{1}">(42);
      |              ^
```

Instead of libc++'s error messages:

```
…/test/diagnostics.cpp:66:21: error: call to consteval function 'std::basic_format_string<char, int>::basic_format_string<char[4]>' is not a constant expression
   66 |   (void)std::format("{1}", 42);
      |                     ^
/usr/lib/llvm-18/bin/../include/c++/v1/__format/format_parse_context.h:91:7: note: non-constexpr function '__throw_format_error' cannot be used in a constant expression
   91 |       std::__throw_format_error("Argument index outside the valid range");
      |       ^
/usr/lib/llvm-18/bin/../include/c++/v1/__format/format_string.h:80:3: note: in call to '__parse_ctx.check_arg_id(1)'
   80 |   __parse_ctx.check_arg_id(__r.__value);
      |   ^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
/usr/lib/llvm-18/bin/../include/c++/v1/__format/format_string.h:159:10: note: in call to '__parse_manual<const char *, std::basic_format_parse_context<char>>(&"{1}"[1], &"{1}"[3], basic_format_parse_context<char>{this->__str_, sizeof...(_Args)})'
  159 |   return __detail::__parse_manual(__begin, __end, __parse_ctx);
      |          ^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
/usr/lib/llvm-18/bin/../include/c++/v1/__format/format_functions.h:246:41: note: in call to '__parse_arg_id<const char *, std::basic_format_parse_context<char>>(&"{1}"[1], &"{1}"[3], basic_format_parse_context<char>{this->__str_, sizeof...(_Args)})'
  246 |   __format::__parse_number_result __r = __format::__parse_arg_id(__begin, __end, __parse_ctx);
      |                                         ^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
/usr/lib/llvm-18/bin/../include/c++/v1/__format/format_functions.h:315:13: note: in call to '__handle_replacement_field<const char *, std::basic_format_parse_context<char>, std::__format::__compile_time_basic_format_context<char>>(&"{1}"[1], &"{1}"[3], basic_format_parse_context<char>{this->__str_, sizeof...(_Args)}, _Context{__types_.data(), __handles_.data(), sizeof...(_Args)})'
  315 |             __format::__handle_replacement_field(__begin, __end, __parse_ctx, __ctx);
      |             ^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
/usr/lib/llvm-18/bin/../include/c++/v1/__format/format_functions.h:370:5: note: in call to '__vformat_to<std::basic_format_parse_context<char>, std::__format::__compile_time_basic_format_context<char>>(basic_format_parse_context<char>{this->__str_, sizeof...(_Args)}, _Context{__types_.data(), __handles_.data(), sizeof...(_Args)})'
  370 |     __format::__vformat_to(basic_format_parse_context<_CharT>{__str_, sizeof...(_Args)},
      |     ^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  371 |                            _Context{__types_.data(), __handles_.data(), sizeof...(_Args)});
      |                            ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
…/test/diagnostics.cpp:66:21: note: in call to 'basic_format_string<char[4]>("{1}")'
   66 |   (void)std::format("{1}", 42);
      |                     ^~~~~
/usr/lib/llvm-18/bin/../include/c++/v1/__format/format_error.h:41:1: note: declared here
   41 | __throw_format_error(const char* __s) {

```

The parts used from the Standard library will provide the messages of that
library and probably look more like the latter message.


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
