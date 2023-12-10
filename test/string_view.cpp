//===----------------------------------------------------------------------===//
//
// Part of the CTF project, under the Apache License v2.0 with LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "ctf/format.hpp"

#include <boost/ut.hpp>

#include <string_view>

#if 0
The code is from libcxx/test/std/utilities/format/format.functions/format_tests.h
The function format_test_string

convered checks with the regex
s/check(SV(\(".*"\)), SV(\(".*"\)),*\(.*\);/expect(eq(ctf::format<\2>(\3,\1sv));


convered check_exceptionss with the regex
s/check_exception(.*SV(\(".*"\)),*\(.*\));/validate<\1>(\2);

The latter uses the validate helper function to deduce the format argument types.
Note that some exception are run-time exceptions, for example the dynamic
argument index is invalid. This needs manual removal of said test.
#endif

template <ctf::fixed_string fmt, class... Args> void validate(Args...) {
  static_assert(!ctf::valid<fmt, Args...>);
}

boost::ut::suite<"format string_view"> format_view_string = [] {
  using namespace boost::ut;
  using namespace std::literals::string_view_literals;

  std::string world = "world";
  std::string universe = "universe";

  // Unicode fill characters
  expect(eq(ctf::format<"{:\u3000^7}">(world), "\u3000world\u3000"sv));

  /* format_test_string */

  // *** Valid input tests ***
  // Unused argument is ignored. TODO FMT what does the Standard mandate?
  expect(eq(ctf::format<"hello {}">(world, universe), "hello world"sv));
  expect(eq(ctf::format<"hello {} and {}">(world, universe),
            "hello world and universe"sv));
  expect(eq(ctf::format<"hello {0}">(world, universe), "hello world"sv));
  expect(eq(ctf::format<"hello {1}">(world, universe), "hello universe"sv));
  expect(eq(ctf::format<"hello {1} and {0}">(world, universe),
            "hello universe and world"sv));

  expect(eq(ctf::format<"hello {:_>}">(world), "hello world"sv));
  expect(eq(ctf::format<"hello {:8}">(world), "hello world   "sv));
  expect(eq(ctf::format<"hello {:>8}">(world), "hello    world"sv));
  expect(eq(ctf::format<"hello {:_>8}">(world), "hello ___world"sv));
  expect(eq(ctf::format<"hello {:_^8}">(world), "hello _world__"sv));
  expect(eq(ctf::format<"hello {:_<8}">(world), "hello world___"sv));

  // The fill character ':' is allowed here (P0645) but not in ranges (P2286).
  expect(eq(ctf::format<"hello {::>8}">(world), "hello :::world"sv));
  expect(eq(ctf::format<"hello {:<>8}">(world), "hello <<<world"sv));
  expect(eq(ctf::format<"hello {:^>8}">(world), "hello ^^^world"sv));

  expect(eq(ctf::format<"hello {:$>{}}">(world, 6), "hello $world"sv));
  expect(eq(ctf::format<"hello {0:$>{1}}">(world, 6), "hello $world"sv));
  expect(eq(ctf::format<"hello {1:$>{0}}">(6, world), "hello $world"sv));

  expect(eq(ctf::format<"hello {:.5}">(world), "hello world"sv));
  expect(eq(ctf::format<"hello {:.5}">(universe), "hello unive"sv));

  expect(eq(ctf::format<"hello {:.{}}">(universe, 6), "hello univer"sv));
  expect(eq(ctf::format<"hello {0:.{1}}">(universe, 6), "hello univer"sv));
  expect(eq(ctf::format<"hello {1:.{0}}">(6, universe), "hello univer"sv));

  expect(eq(ctf::format<"hello {:%^7.7}">(world), "hello %world%"sv));
  expect(eq(ctf::format<"hello {:%^7.7}">(universe), "hello univers"sv));
  expect(eq(ctf::format<"hello {:%^{}.{}}">(world, 7, 7), "hello %world%"sv));
  expect(
      eq(ctf::format<"hello {0:%^{1}.{2}}">(world, 7, 7), "hello %world%"sv));
  expect(
      eq(ctf::format<"hello {0:%^{2}.{1}}">(world, 7, 7), "hello %world%"sv));
  expect(
      eq(ctf::format<"hello {1:%^{0}.{2}}">(7, world, 7), "hello %world%"sv));

  expect(eq(ctf::format<"hello {:_>s}">(world), "hello world"sv));
  expect(eq(ctf::format<"hello {:$>{}s}">(world, 6), "hello $world"sv));
  expect(eq(ctf::format<"hello {:.5s}">(world), "hello world"sv));
  expect(eq(ctf::format<"hello {:.{}s}">(universe, 6), "hello univer"sv));
  expect(eq(ctf::format<"hello {:%^7.7s}">(world), "hello %world%"sv));

  expect(eq(ctf::format<"hello {:#>8.3s}">(universe), "hello #####uni"sv));
  expect(eq(ctf::format<"hello {:#^8.3s}">(universe), "hello ##uni###"sv));
  expect(eq(ctf::format<"hello {:#<8.3s}">(universe), "hello uni#####"sv));

  // *** sign ***
  validate<"hello {:-}">(world);

  // *** alternate form ***
  validate<"hello {:#}">(world);

  // *** zero-padding ***
  validate<"hello {:0}">(world);

  // *** width ***
  // Width 0 allowed, but not useful for string arguments.
  expect(eq(ctf::format<"hello {:{}}">(world, 0), "hello world"sv));

  validate<"hello {:{}}">(world);
  validate<"hello {:{}}">(world, universe);
  validate<"hello {:{0}}">(world, 1);
  validate<"hello {0:{}}">(world, 1);
  // Arg-id may not have leading zeros.
  validate<"hello {0:{01}}">(world, 1);

  // *** precision ***

  // Precision 0 allowed, but not useful for string arguments.
  expect(eq(ctf::format<"hello {:.{}}">(world, 0), "hello "sv));
  // Precision may have leading zeros. Secondly tests the value is still
  // base 10.
  expect(eq(ctf::format<"hello {:.000010}">(std::string("0123456789abcdef")),
            "hello 0123456789"sv));
  validate<"hello {:.{}}">(world);
  validate<"hello {:.{}}">(world, universe);
  validate<"hello {:.{0}}">(world, 1);
  validate<"hello {0:.{}}">(world, 1);
  // Arg-id may not have leading zeros.
  validate<"hello {0:.{01}}">(world, 1);

  // *** locale-specific form ***
  validate<"hello {:L}">(world);
};
