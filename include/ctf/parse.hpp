//===----------------------------------------------------------------------===//
//
// Part of the CTF project, under the Apache License v2.0 with LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef CTF_PARSE_HPP
#define CTF_PARSE_HPP

#include "format_error.hpp"
#include "utility.hpp"

#include <cstdint>

namespace ctf {

// The numbers used in the library are always non-negative.
//
// The Standard does not have clear limit for the value. However this limit
// seems more than likely to be used. Using an 8-bit char this can output
// 2GB of data. Similar for arg-id this allows for more template arguments
// than compilers support.
template <std::size_t O, std::int32_t V> struct parse_number_result {
  static constexpr std::size_t offset = O;
  // -1 means overflow
  static constexpr std::int32_t value = V;
};

// Parses a number from the input.
//
// This function expects the caller to have parsed the first digit. The width
// is a positive integer, the other use-cases a non-negative integer. By
// letting the caller process the first digit it's easy to handle both cases.
// (Besides the caller needs to detect there is a digit in the first place
// before deciding to call this function.)
//
// Note the input is NUL terminated so reading out of bounds in not an issue.
//
// The function is called with
// result = parse_number_result
// - O offset with the next character to parse.
// - V the initial value of the first digit.
//
// Returns
// parse_number_result
// - O offset beyond the last parsed character.
// - V the parsed value, -1 upon overflow.
//
// The function does not return format_error, since it does not know the
// context it is called in. The caller can create a better error message based
// on the context.
template <fixed_string fmt, class result> auto consteval parse_number() {
  static_assert(result::value >= 0, "negative values are flags");

  constexpr auto c = fmt.text[result::offset];
  using CharT = decltype(c);

  if constexpr (c >= CharT('0') && c <= CharT('9')) {
    constexpr std::int32_t v = c - CharT('0');
    // Validate the result can be stored in an int32_t.
    if constexpr (result::value < 214748364 ||
                  (result::value == 214748364 && v <= 7))
      return parse_number<fmt, parse_number_result<result::offset + 1,
                                                   result::value * 10 + v>>();
    else
      return parse_number_result<result::offset, -1>();
  } else
    return result{};
}

// The indexing mode for the argument indices.
//
// unknown means no replacement field found yet.
enum class index_mode { unknown, manual, automatic };

template <index_mode m, std::size_t i, std::size_t c> struct arg_id_status {
  static constexpr index_mode mode = m;

  // index of the next arg_id in manual mode
  static constexpr std::size_t index = i;

  // The number of arguments.
  static constexpr std::size_t count = c;
};

template <std::size_t o, std::size_t i, arg_id_status s>
struct parse_arg_id_result {
  static constexpr std::size_t offset = o;

  // -1 means the arg-id was not terminated with a : or }.
  // The caller should give the proper diagnostic.
  static constexpr std::size_t index = i;

  static constexpr arg_id_status status = s;
};

namespace detail {

consteval std::string index_to_string(std::size_t index) {
  switch (index) {
  case 0:
    return "first";
  case 1:
    return "second";
  case 2:
    return "third";
  case 3:
    return "fourth";
  case 4:
    return "fifth";
  case 5:
    return "sixth";
  case 6:
    return "seventh";
  case 7:
    return "eight";
  case 8:
    return "ninth";
  case 9:
    return "tenth";
  default:
    return ctf::to_string(index + 1) + "-th";
  }
}

consteval std::string count_to_string(std::size_t count) {
  switch (count) {
  case 0:
    return "zero";
  case 1:
    return "one";
  case 2:
    return "two";
  case 3:
    return "three";
  case 4:
    return "four";
  case 5:
    return "five";
  case 6:
    return "six";
  case 7:
    return "seven";
  case 8:
    return "eight";
  case 9:
    return "nine";
  default:
    return ctf::to_string(count);
  }
}
consteval format_error
create_format_error_arg_id_out_of_bounds(std::string &&fmt, std::size_t arg_id,
                                         std::size_t count, std::size_t begin,
                                         std::size_t end) {

  return ctf::create_format_error(
      "using the " + ctf::detail::index_to_string(arg_id) + " argument while " +
          ctf::detail::count_to_string(count) + " argument" +
          (count != 1 ? "s are" : " is") + " available",
      std::move(fmt), begin, end, end);
}

template <fixed_string fmt, std::size_t offset, arg_id_status status>
auto consteval parse_arg_id_manual() {
  // The number is parsed to mark the entire value when in the wrong mode.
  auto number = ctf::parse_number<
      fmt, parse_number_result<offset + 1, fmt.text[offset] - '0'>>();

  if constexpr (status.mode == index_mode::automatic)
    return ctf::create_format_error(
        "using a manual argument id while in automatic index mode",
        std::string(fmt.text), offset, number.offset, number.offset);
  else {
    if constexpr (number.value == -1)
      return ctf::create_format_error(
          "the value of the argument index is larger than the implementation "
          "supports (2147483647)",
          std::string(fmt.text), offset, number.offset, number.offset);
    else {
      constexpr auto c = fmt.text[number.offset];
      if constexpr (c != ':' && c != '}')
        return parse_arg_id_result<
            number.offset, std::size_t(-1),
            arg_id_status<index_mode::manual, status.index, status.count>{}>{};

      else if constexpr (fmt.text[offset] == '0' && number.offset != offset + 1)
        return ctf::create_format_error(
            "the argument index has a leading zero ", std::string(fmt.text),
            offset, offset + 1, number.offset - 1);

      else if constexpr (number.value >= status.count)
        return create_format_error_arg_id_out_of_bounds(
            std::string(fmt.text), number.value, status.count, offset,
            number.offset);
      else
        return parse_arg_id_result<
            number.offset, number.value,
            arg_id_status<index_mode::manual, status.index, status.count>{}>{};
    }
  }
}

template <fixed_string fmt, std::size_t offset, arg_id_status status>
auto consteval parse_arg_id_automatic() {
  if constexpr (status.mode == index_mode::manual)
    return ctf::create_format_error(
        "using a automatic argument id while in manual index mode",
        std::string(fmt.text), offset - 1, offset, offset);
  else {
    constexpr auto c = fmt.text[offset];
    if constexpr (c != ':' && c != '}')
      return parse_arg_id_result<
          offset, std::size_t(-1),
          arg_id_status<index_mode::automatic, status.index, status.count>{}>{};

    else if constexpr (status.index >= status.count)
      return create_format_error_arg_id_out_of_bounds(
          std::string(fmt.text), status.index, status.count, offset - 1,
          offset);
    else
      return parse_arg_id_result<
          offset, status.index,
          arg_id_status<index_mode::automatic, status.index + 1,
                        status.count>{}>{};
  }
}

} // namespace detail

template <fixed_string fmt, std::size_t offset, arg_id_status status>
auto consteval parse_arg_id() {
  constexpr auto c = fmt.text[offset];
  using CharT = decltype(c);
  if constexpr (c >= CharT('0') && c <= CharT('9'))
    return detail::parse_arg_id_manual<fmt, offset, status>();
  else
    return detail::parse_arg_id_automatic<fmt, offset, status>();
}

} // namespace ctf

#endif // CTF_PARSE_HPP
