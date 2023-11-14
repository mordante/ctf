//===----------------------------------------------------------------------===//
//
// Part of the CTF project, under the Apache License v2.0 with LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "ctf/format.hpp"

#include <boost/ut.hpp>

#include <array>
#include <cassert>
#include <chrono>
#include <map>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>

// A user-defined type used to test the handle formatter.
enum class status : std::uint16_t {
  foo = 0xAAAA,
  bar = 0x5555,
  foobar = 0xAA55
};

// The formatter for a user-defined type used to test the handle formatter.
template <class CharT> struct std::formatter<status, CharT> {
  // During the 2023 Issaquah meeting LEWG made it clear a formatter is
  // required to call its parse function. LWG3892 Adds the wording for that
  // requirement. Therefore this formatter is initialized in an invalid state.
  // A call to parse sets it in a valid state and a call to format validates
  // the state.
  int type = -1;

  constexpr auto parse(basic_format_parse_context<CharT> &parse_ctx)
      -> decltype(parse_ctx.begin()) {
    auto begin = parse_ctx.begin();
    auto end = parse_ctx.end();
    type = 0;
    if (begin == end)
      return begin;

    switch (*begin) {
    case CharT('x'):
      break;
    case CharT('X'):
      type = 1;
      break;
    case CharT('s'):
      type = 2;
      break;
    case CharT('}'):
      return begin;
    default:
      throw_format_error("The type option contains an invalid value for a "
                         "status formatting argument");
    }

    ++begin;
    if (begin != end && *begin != CharT('}'))
      throw_format_error(
          "The format specifier should consume the input or end with a '}'");

    return begin;
  }

  template <class Out>
  auto format(status s, basic_format_context<Out, CharT> &ctx) const
      -> decltype(ctx.out()) {
    const char *names[] = {"foo", "bar", "foobar"};
    char buffer[7];
    const char *begin = names[0];
    const char *end = names[0];
    switch (type) {
    case -1:
      throw_format_error("The formatter's parse function has not been called.");

    case 0:
      begin = buffer;
      buffer[0] = '0';
      buffer[1] = 'x';
      end = std::to_chars(&buffer[2], std::end(buffer),
                          static_cast<std::uint16_t>(s), 16)
                .ptr;
      buffer[6] = '\0';
      break;

    case 1:
      begin = buffer;
      buffer[0] = '0';
      buffer[1] = 'X';
      end = std::to_chars(&buffer[2], std::end(buffer),
                          static_cast<std::uint16_t>(s), 16)
                .ptr;
      std::transform(static_cast<const char *>(&buffer[2]), end, &buffer[2],
                     [](char c) { return static_cast<char>(std::toupper(c)); });
      buffer[6] = '\0';
      break;

    case 2:
      switch (s) {
      case status::foo:
        begin = names[0];
        break;
      case status::bar:
        begin = names[1];
        break;
      case status::foobar:
        begin = names[2];
        break;
      }
      end = begin + strlen(begin);
      break;
    }

    return std::copy(begin, end, ctx.out());
  }

private:
  [[noreturn]] void throw_format_error([[maybe_unused]] const char *s) const {
#ifndef TEST_HAS_NO_EXCEPTIONS
    throw std::format_error(s);
#else
    std::abort();
#endif
  }
};

namespace {
template <class... Args> struct list {};
using integer_list = list<signed char, short, int, long, long long, __int128_t,
                          unsigned char, unsigned short, unsigned int,
                          unsigned long, unsigned long long, __uint128_t>;
using floating_point_list = list<float, double, long double>;

// charT[N] is omitted in this list.
// For basic_string and basic_string_view only std::char_traits<char> is tested.
// For basic_string only std::allocator<char> is tested.
template <class CharT>
using basic_string_list = list<CharT *, const CharT *, std::basic_string<CharT>,
                               std::basic_string_view<CharT>>;
using string_list = basic_string_list<char>;
// using wstring_list = basic_string_list<wchar_t>;

using pointer_list = list<std::nullptr_t, void *, const void *>;

// Runs a set of tests on a type.
// The typical usage
/*
templated_test(
    []<class T> {
      expect(eq(ctf::format<"{}">(T(...)), "..."sv));
      ...
    },
    xxx_list{});
*/
auto templated_test = []<class L, class T, class... Args>(
                          this auto self, L test, list<T, Args...>) {
  test.template operator()<T>();
  if constexpr (sizeof...(Args))
    self(test, list<Args...>{});
};

boost::ut::suite<"format no replacement fields"> format_no_replacement_fields =
    [] {
      auto test = [] {
        assert(ctf::format<"">() == "");
        assert(ctf::format<"{{">() == "{");
        assert(ctf::format<"}}">() == "}");
        assert(ctf::format<"{{}}">() == "{}");
        assert(ctf::format<"hello world">() == "hello world");

        assert(ctf::format<"{{}}">('a') == "{}");

        return true;
      };
      test();
      static_assert(test());
    };

boost::ut::suite<"format replacement field basics">
    format_replacement_field_basics = [] {
      assert(ctf::format<"{}">(42) == "42");
      assert(ctf::format<"{:} {:}">(42, 99) == "42 99");

      assert(ctf::format<"{0} {0}">(42) == "42 42");
      assert(ctf::format<"{0:} {0:}">(42) == "42 42");
      assert(ctf::format<"{1} {0:} {1}">(42, 99) == "99 42 99");
    };

boost::ut::suite<"format char"> format_char = [] {
  using namespace boost::ut;
  using namespace std::literals::string_view_literals;

  "default"_test = [] {
    expect(eq(ctf::format<"{}">('a'), "a"sv));
    expect(eq(ctf::format<"{:<<5}">('a'), "a<<<<"sv));
    expect(eq(ctf::format<"{:>>5}">('a'), ">>>>a"sv));
    expect(eq(ctf::format<"{:^^5}">('a'), "^^a^^"sv));
  };

  "char"_test = [] {
    expect(eq(ctf::format<"{:c}">('a'), "a"sv));
    expect(eq(ctf::format<"{:<<5c}">('a'), "a<<<<"sv));
    expect(eq(ctf::format<"{:>>5c}">('a'), ">>>>a"sv));
    expect(eq(ctf::format<"{:^^5c}">('a'), "^^a^^"sv));
  };

  "debug"_test = [] {
    expect(eq(ctf::format<"{:?}">('a'), "'a'"sv));
    expect(eq(ctf::format<"{:<<5?}">('a'), "'a'<<"sv));
    expect(eq(ctf::format<"{:>>5?}">('a'), ">>'a'"sv));
    expect(eq(ctf::format<"{:^^5?}">('a'), "^'a'^"sv));
  };

  "integral"_test = [] {
    expect(eq(ctf::format<"{:b}">('*'), "101010"sv));
    expect(eq(ctf::format<"{:+b}">('*'), "+101010"sv));
    expect(eq(ctf::format<"{:+#b}">('*'), "+0b101010"sv));
    expect(eq(ctf::format<"{:+#011b}">('*'), "+0b00101010"sv));

    expect(eq(ctf::format<"{:B}">('*'), "101010"sv));
    expect(eq(ctf::format<"{:+B}">('*'), "+101010"sv));
    expect(eq(ctf::format<"{:+#B}">('*'), "+0B101010"sv));
    expect(eq(ctf::format<"{:+#011B}">('*'), "+0B00101010"sv));

    expect(eq(ctf::format<"{:o}">('*'), "52"sv));
    expect(eq(ctf::format<"{:+o}">('*'), "+52"sv));
    expect(eq(ctf::format<"{:+#o}">('*'), "+052"sv));
    expect(eq(ctf::format<"{:+#06o}">('*'), "+00052"sv));

    expect(eq(ctf::format<"{:d}">('*'), "42"sv));
    expect(eq(ctf::format<"{:+d}">('*'), "+42"sv));
    expect(eq(ctf::format<"{:+#d}">('*'), "+42"sv));
    expect(eq(ctf::format<"{:+#05d}">('*'), "+0042"sv));

    expect(eq(ctf::format<"{:x}">('*'), "2a"sv));
    expect(eq(ctf::format<"{:+x}">('*'), "+2a"sv));
    expect(eq(ctf::format<"{:+#x}">('*'), "+0x2a"sv));
    expect(eq(ctf::format<"{:+#07x}">('*'), "+0x002a"sv));

    expect(eq(ctf::format<"{:X}">('*'), "2A"sv));
    expect(eq(ctf::format<"{:+X}">('*'), "+2A"sv));
    expect(eq(ctf::format<"{:+#X}">('*'), "+0X2A"sv));
    expect(eq(ctf::format<"{:+#07X}">('*'), "+0X002A"sv));
  };
};

boost::ut::suite<"format bool"> format_bool = [] {
  using namespace boost::ut;
  using namespace std::literals::string_view_literals;

  "default"_test = [] {
    expect(eq(ctf::format<"{}">(true), "true"sv));
    expect(eq(ctf::format<"{}">(false), "false"sv));
  };

  "string"_test = [] {
    expect(eq(ctf::format<"{:s}">(true), "true"sv));
    expect(eq(ctf::format<"{:s}">(false), "false"sv));
  };

  "integral"_test = [] {
    expect(eq(ctf::format<"{:b}">(true), "1"sv));
    expect(eq(ctf::format<"{:#b}">(false), "0b0"sv));

    expect(eq(ctf::format<"{:B}">(true), "1"sv));
    expect(eq(ctf::format<"{:#B}">(false), "0B0"sv));

    expect(eq(ctf::format<"{:o}">(true), "1"sv));
    expect(eq(ctf::format<"{:#o}">(true), "01"sv));
    expect(eq(ctf::format<"{:#o}">(false), "0"sv));

    expect(eq(ctf::format<"{:d}">(true), "1"sv));
    expect(eq(ctf::format<"{:#d}">(false), "0"sv));

    expect(eq(ctf::format<"{:x}">(true), "1"sv));
    expect(eq(ctf::format<"{:#x}">(false), "0x0"sv));

    expect(eq(ctf::format<"{:X}">(true), "1"sv));
    expect(eq(ctf::format<"{:#X}">(false), "0X0"sv));
  };
};

boost::ut::suite<"format integral"> format_integral = [] {
  using namespace boost::ut;
  using namespace std::literals::string_view_literals;

  "default"_test = [] {
    templated_test(
        []<class T> {
          expect(eq(ctf::format<"{}">(T(0)), "0"sv));
          expect(eq(ctf::format<"{}">(T(42)), "42"sv));
          if constexpr (std::signed_integral<T>)
            expect(eq(ctf::format<"{}">(T(-42)), "-42"sv));
        },
        integer_list{});
  };

  "char"_test = [] {
    templated_test(
        []<class T> {
          expect(eq(ctf::format<"{:c}">(T(0)), "\x00"sv));
          expect(eq(ctf::format<"{:c}">(T(42)), "*"sv));
          if constexpr (std::signed_integral<T>)
            expect(eq(ctf::format<"{:c}">(T(-1)), "\xFF"sv));
        },
        integer_list{});
  };

  "integral"_test = [] {
    templated_test(
        []<class T> {
          expect(eq(ctf::format<"{:b}">(T(42)), "101010"sv));
          expect(eq(ctf::format<"{:+b}">(T(42)), "+101010"sv));
          expect(eq(ctf::format<"{:+#b}">(T(42)), "+0b101010"sv));
          expect(eq(ctf::format<"{:+#011b}">(T(42)), "+0b00101010"sv));

          expect(eq(ctf::format<"{:B}">(T(42)), "101010"sv));
          expect(eq(ctf::format<"{:+B}">(T(42)), "+101010"sv));
          expect(eq(ctf::format<"{:+#B}">(T(42)), "+0B101010"sv));
          expect(eq(ctf::format<"{:+#011B}">(T(42)), "+0B00101010"sv));

          expect(eq(ctf::format<"{:o}">(T(42)), "52"sv));
          expect(eq(ctf::format<"{:+o}">(T(42)), "+52"sv));
          expect(eq(ctf::format<"{:+#o}">(T(42)), "+052"sv));
          expect(eq(ctf::format<"{:+#06o}">(T(42)), "+00052"sv));

          expect(eq(ctf::format<"{:d}">(T(42)), "42"sv));
          expect(eq(ctf::format<"{:+d}">(T(42)), "+42"sv));
          expect(eq(ctf::format<"{:+#d}">(T(42)), "+42"sv));
          expect(eq(ctf::format<"{:+#05d}">(T(42)), "+0042"sv));

          expect(eq(ctf::format<"{:x}">(T(42)), "2a"sv));
          expect(eq(ctf::format<"{:+x}">(T(42)), "+2a"sv));
          expect(eq(ctf::format<"{:+#x}">(T(42)), "+0x2a"sv));
          expect(eq(ctf::format<"{:+#07x}">(T(42)), "+0x002a"sv));

          expect(eq(ctf::format<"{:X}">(T(42)), "2A"sv));
          expect(eq(ctf::format<"{:+X}">(T(42)), "+2A"sv));
          expect(eq(ctf::format<"{:+#X}">(T(42)), "+0X2A"sv));
          expect(eq(ctf::format<"{:+#07X}">(T(42)), "+0X002A"sv));
        },
        integer_list{});
  };
};

boost::ut::suite<"format floating-point"> format_floating_point = [] {
  using namespace boost::ut;
  using namespace std::literals::string_view_literals;

  "default"_test = [] {
    templated_test(
        []<class T> {
          expect(eq(ctf::format<"{}">(T(-2.5)), "-2.5"sv));
          expect(eq(ctf::format<"{}">(T(0)), "0"sv));
          expect(eq(ctf::format<"{}">(T(0.015625)), "0.015625"sv));

          expect(eq(ctf::format<"{}">(std::numeric_limits<T>::infinity()),
                    "inf"sv));
          expect(eq(ctf::format<"{}">(std::numeric_limits<T>::quiet_NaN()),
                    "nan"sv));
        },
        floating_point_list{});
  };

  "floating_point"_test = [] {
    templated_test(
        []<class T> {
          expect(eq(ctf::format<"{:a}">(T(0x1.abcp+0)), "1.abcp+0"sv));
          expect(eq(ctf::format<"{:A}">(T(-0x1.abcp+0)), "-1.ABCP+0"sv));

          expect(eq(ctf::format<"{:e}">(T(0.25)), "2.500000e-01"sv));
          expect(eq(ctf::format<"{:E}">(T(25)), "2.500000E+01"sv));

          expect(eq(ctf::format<"{:f}">(T(0.25)), "0.250000"sv));
          expect(eq(ctf::format<"{:F}">(T(25)), "25.000000"sv));

          expect(eq(ctf::format<"{:g}">(T(2'500'000)), "2.5e+06"sv));
          expect(eq(ctf::format<"{:G}">(T(-2'500'000)), "-2.5E+06"sv));
        },
        floating_point_list{});
  };
};

boost::ut::suite<"format string"> format_string = [] {
  using namespace boost::ut;
  using namespace std::literals::string_view_literals;

  "default"_test = [] {
    templated_test(
        []<class T> {
          expect(eq(ctf::format<"{}">(T("hello")), "hello"sv));
          expect(eq(ctf::format<"{:^^7}">(T("hello")), "^hello^"sv));
          expect(eq(ctf::format<"{:^^7}">(T("hellö")), "^hellö^"sv));
          expect(
              eq(ctf::format<"{:^^7}">(T("hell\u0308o")), "^hell\u0308o^"sv));

          // 2 columns
          expect(eq(ctf::format<"{:^^4}">(T("\u1100")), "^\u1100^"sv));
        },
        string_list{});
  };

  "string"_test = [] {
    templated_test(
        []<class T> {
          expect(eq(ctf::format<"{:s}">(T("hello")), "hello"sv));
          expect(eq(ctf::format<"{:^^7s}">(T("hello")), "^hello^"sv));
          expect(eq(ctf::format<"{:^^7s}">(T("hellö")), "^hellö^"sv));
          expect(
              eq(ctf::format<"{:^^7s}">(T("hell\u0308o")), "^hell\u0308o^"sv));

          // 2 columns
          expect(eq(ctf::format<"{:^^4s}">(T("\u1100")), "^\u1100^"sv));
        },
        string_list{});
  };

  "debug"_test = [] {
    templated_test(
        []<class T> {
          expect(eq(ctf::format<"{:?}">(T("hello")), "\"hello\""sv));
          expect(eq(ctf::format<"{:^^9?}">(T("hello")), "^\"hello\"^"sv));
          expect(eq(ctf::format<"{:^^9?}">(T("hellö")), "^\"hellö\"^"sv));
        },
        string_list{});
  };
  /*
   * TODO IMPLEMENT
   * template<size_t N> struct formatter<charT[N], charT>;
   */
};

boost::ut::suite<"format pointer"> format_pointer = [] {
  using namespace boost::ut;
  using namespace std::literals::string_view_literals;

  templated_test(
      []<class T> {
        expect(eq(ctf::format<"{}">(T(nullptr)), "0x0"sv));
        expect(eq(ctf::format<"{:p}">(T(nullptr)), "0x0"sv));
        expect(eq(ctf::format<"{:P}">(T(nullptr)), "0X0"sv));
      },
      pointer_list{});
};

boost::ut::suite<"format handle"> format_handle = [] {
  using namespace boost::ut;
  using namespace std::literals::string_view_literals;

  expect(eq(ctf::format<"{}">(status::foo), "0xaaaa"sv));
  expect(eq(ctf::format<"{:x}">(status::foo), "0xaaaa"sv));
  expect(eq(ctf::format<"{:X}">(status::foo), "0XAAAA"sv));
  expect(eq(ctf::format<"{:s}">(status::foo), "foo"sv));

  expect(eq(ctf::format<"{}">(status::bar), "0x5555"sv));
  expect(eq(ctf::format<"{:x}">(status::bar), "0x5555"sv));
  expect(eq(ctf::format<"{:X}">(status::bar), "0X5555"sv));
  expect(eq(ctf::format<"{:s}">(status::bar), "bar"sv));

  expect(eq(ctf::format<"{}">(status::foobar), "0xaa55"sv));
  expect(eq(ctf::format<"{:x}">(status::foobar), "0xaa55"sv));
  expect(eq(ctf::format<"{:X}">(status::foobar), "0XAA55"sv));
  expect(eq(ctf::format<"{:s}">(status::foobar), "foobar"sv));
};

boost::ut::suite<"format chrono"> format_chrono = [] {
  using namespace boost::ut;
  using namespace std::literals::string_view_literals;
  using namespace std::literals::chrono_literals;

  // 03:33:20 UTC on Wednesday, 18 May 2033
  {
    auto time = std::chrono::sys_seconds{2'000'000'000s};
    expect(eq(std::format("{}", time), "2033-05-18 03:33:20"sv));
    // TODO fix this failure
    // expect(eq(ctf::format<"{} ">(time), "a"sv));
  }

#define time                                                                   \
  std::chrono::sys_seconds { 2'000'000'000s }
  expect(eq(ctf::format<"{}">(time), "2033-05-18 03:33:20"sv));
  expect(eq(ctf::format<"The current time is {:%Y.%m.%d %H:%M:%S}">(time),
            "The current time is 2033.05.18 03:33:20"sv));
  expect(eq(ctf::format<"{:%tThe current time is %Y.%m.%d %H:%M:%S}">(time),
            "\tThe current time is 2033.05.18 03:33:20"sv));
#undef time
};

boost::ut::suite<"format ranges"> format_ranges = [] {
  using namespace boost::ut;
  using namespace std::literals::string_view_literals;

  expect(eq(ctf::format<"{}">(std::vector{1, 2, 3}), "[1, 2, 3]"sv));
  expect(eq(ctf::format<"{}">(std::array{1.1, 2.2, 3.3}), "[1.1, 2.2, 3.3]"sv));
#if 0
  // TODO investigate
  expect(
      eq(ctf::format<"{}">(std::make_pair("key", "value")), "{key: value}"sv));
  expect(
      eq(ctf::format<"{}">(std::make_tuple("key", "value")), "{key: value}"sv));
#endif
  expect(eq(
      ctf::format<"{}">(std::map<const char *, const char *>{{"key", "value"}}),
      "{\"key\": \"value\"}"sv));
};

} // namespace
