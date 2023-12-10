//===----------------------------------------------------------------------===//
//
// Part of the CTF project, under the Apache License v2.0 with LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef CTF_FORMAT_HPP
#define CTF_FORMAT_HPP

#include "format_error.hpp"
#include "formatter.hpp"
#include "formatter_string.hpp"
#include "parse.hpp"
#include "tuple.hpp"
#include "utility.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <format>
#include <tuple>

namespace ctf {

template <fixed_string fmt, class... Args>
constexpr std::string format(Args &&...args);

struct output_char_tag {};
struct output_text_tag {};
struct output_replacement_field_tag {};

template <std::size_t N> struct output_char {
  using tag = output_char_tag;

  static constexpr std::size_t offset = N;
};

template <std::size_t O, std::size_t S> struct output_text {
  using tag = output_text_tag;
  static constexpr std::size_t offset = O;
  static constexpr std::size_t size = S;
};

template <std::size_t O, std::size_t I, class F>
struct output_replacement_field {
  using tag = output_replacement_field_tag;
  static constexpr std::size_t offset = O;
  static constexpr std::size_t index = I;
  F formatter;
};

template <std::size_t o, arg_id_status i, class Tokens> struct parser_status {
  static constexpr std::size_t offset = o;
  static constexpr arg_id_status arg_id = i;

  Tokens tokens;
};

template <std::size_t O, std::size_t C = std::size_t(-1),
          std::size_t Id = std::size_t(-1), std::size_t L = 0>
struct replacement_field {
  static constexpr std::size_t offset = O;
  static constexpr std::size_t colon = C;
  static constexpr std::size_t id = Id;
  static constexpr std::size_t level = L;
};

// This is a simple replacement field parser.
//
// It assumes the format specifier does not contain unbalanced {}-pairs. This
// is true for the Standard formatters.
template <class R, fixed_string Fmt> consteval auto parse_replacement_field() {
  // } allowed before colon
  // { allowed after colon

  constexpr auto c = Fmt[R::offset];
  if constexpr (c == '}') {
    if constexpr (R::level == 0)
      return R{};
    else
      return parse_replacement_field<
          replacement_field<R::offset + 1, R::colon, R::id, R::level - 1>,
          Fmt>();
  } else if constexpr (R::colon == -1) {
    if constexpr (Fmt[R::offset] == ':')
      return parse_replacement_field<
          replacement_field<R::offset + 1, R::offset, R::id, R::level>, Fmt>();
    else if constexpr (c >= '0' && c <= '9') {
      auto number =
          ctf::parse_number<Fmt, parse_number_result<R::offset + 1, c - '0'>>();

      if constexpr (number.value == -1)
        return ctf::create_format_error(
            "the value of the argument index is larger than the implementation "
            "supports (2147483647)",
            Fmt, R::offset, number.offset, number.offset);

      else
        return parse_replacement_field<
            replacement_field<number.offset, R::colon, number.value, R::level>,
            Fmt>();

    } else
      return ctf::create_format_error(
          (R::offset == Fmt.size ? "unexpected end of the format string"
                                 : "unexpected character in the format string"),
          Fmt, R::offset - 1, R::offset, R::offset,
          "{     -> an escaped {",                     //
          "}     -> the end of the replacement-field", //
          ":     -> start of the format-specifier",    //
          "[0-9] -> an arg-id");

  } else if constexpr (c == '{')
    return parse_replacement_field<
        replacement_field<R::offset + 1, R::colon, R::id, R::level + 1>, Fmt>();
  else
    return parse_replacement_field<
        replacement_field<R::offset + 1, R::colon, R::id, R::level>, Fmt>();
};

// Adds a new char to the result.
// The function tries a few optimizations
// char + char -> text
// text + char -> text
template <std::size_t O, class Tuple> consteval auto append_char(Tuple data) {
  if constexpr (std::same_as<Tuple, tuple<>>) {
    return ctf::tuple_append(data, output_char<O>{});
  } else {
    using T = ctf::tuple_type<ctf::tuple_size<Tuple> - 1, Tuple>;

    if constexpr (std::same_as<typename T::tag, output_char_tag>) {
      if constexpr (T::offset + 1 == O)
        return ctf::tuple_replace_back(data, output_text<T::offset, 2>{});
      else
        return ctf::tuple_append(data, output_char<O>{});

    } else if constexpr (std::same_as<typename T::tag, output_text_tag>) {
      if constexpr (T::offset + T::size == O)
        return ctf::tuple_replace_back(data,
                                       output_text<T::offset, T::size + 1>{});
      else
        return ctf::tuple_append(data, output_char<O>{});
    } else
      return ctf::tuple_append(data, output_char<O>{});
  }
}

// The format_arg's constructor type conversion rules.

template <class CharT, class T> struct format_arg {
  using type = T &;
};

template <class CharT> struct format_arg<CharT, bool> {
  using type = bool;
};

template <> struct format_arg<char, char> {
  using type = char;
};

template <> struct format_arg<wchar_t, char> {
  using type = wchar_t;
};

template <> struct format_arg<wchar_t, wchar_t> {
  using type = wchar_t;
};

template <class CharT> struct format_arg<CharT, signed char> {
  using type = int;
};

template <class CharT> struct format_arg<CharT, short> {
  using type = int;
};

template <class CharT> struct format_arg<CharT, int> {
  using type = int;
};

template <class CharT> struct format_arg<CharT, long> {
  using type = std::conditional_t<sizeof(long) == sizeof(int), int, long long>;
};

template <class CharT> struct format_arg<CharT, long long> {
  using type = long long;
};

template <class CharT> struct format_arg<CharT, __int128_t> {
  using type = __int128_t;
};

template <class CharT> struct format_arg<CharT, unsigned char> {
  using type = unsigned;
};

template <class CharT> struct format_arg<CharT, unsigned short> {
  using type = unsigned;
};

template <class CharT> struct format_arg<CharT, unsigned> {
  using type = unsigned;
};

template <class CharT> struct format_arg<CharT, unsigned long> {
  using type = std::conditional_t<sizeof(unsigned long) == sizeof(unsigned),
                                  unsigned, unsigned long long>;
};

template <class CharT> struct format_arg<CharT, unsigned long long> {
  using type = unsigned long long;
};

template <class CharT> struct format_arg<CharT, __uint128_t> {
  using type = __uint128_t;
};

template <class CharT> struct format_arg<CharT, float> {
  using type = float;
};

template <class CharT> struct format_arg<CharT, double> {
  using type = double;
};

template <class CharT> struct format_arg<CharT, long double> {
  using type = long double;
};

template <class CharT> struct format_arg<CharT, CharT *> {
  static_assert(std::same_as<CharT, char> || std::same_as<CharT, wchar_t>);
  using type = std::basic_string_view<CharT>;
};

template <class CharT> struct format_arg<CharT, const CharT *> {
  static_assert(std::same_as<CharT, char> || std::same_as<CharT, wchar_t>);
  using type = std::basic_string_view<CharT>;
};

template <class CharT, std::size_t N> struct format_arg<CharT, CharT[N]> {
  static_assert(std::same_as<CharT, char> || std::same_as<CharT, wchar_t>);
  using type = std::basic_string_view<CharT>;
};

template <class CharT, class Traits>
struct format_arg<CharT, std::basic_string_view<CharT, Traits>> {
  static_assert(std::same_as<CharT, char> || std::same_as<CharT, wchar_t>);
  using type = std::basic_string_view<CharT>;
};

template <class CharT, class Traits, class Allocator>
struct format_arg<CharT, std::basic_string<CharT, Traits, Allocator>> {
  static_assert(std::same_as<CharT, char> || std::same_as<CharT, wchar_t>);
  using type = std::basic_string_view<CharT>;
};

template <class CharT> struct format_arg<CharT, void *> {
  using type = const void *;
};

template <class CharT> struct format_arg<CharT, const void *> {
  using type = const void *;
};

template <class CharT> struct format_arg<CharT, nullptr_t> {
  using type = const void *;
};

template <fixed_string fmt, class... Args>
consteval auto handle_replacement_field3(auto status) {
  using P = decltype(status);

  auto arg_id = parse_arg_id<fmt, P::offset + 1, status.arg_id>();

  if constexpr (ctf::is_format_error(arg_id))
    return arg_id;
  else {

    constexpr auto c = fmt[arg_id.offset];
    if constexpr (c != ':' && c != '}') {
      // TODO both branches have some duplicates.
      if constexpr (arg_id.offset == P::offset + 1)
        // Nothing parsed so it's unknown what the user intended the { to be
        // part of.
        return ctf::create_format_error(
            (arg_id.offset == fmt.size()
                 ? "unexpected end of the format string"
                 : "unexpected character in the format string"),
            fmt, P::offset, arg_id.offset, arg_id.offset,
            "{     -> an escaped {",                     //
            "}     -> the end of the replacement-field", //
            ":     -> start of the format-specifier",    //
            "[0-9] -> an arg-id");
      else
        // An arg-id was found, to the user intended it to be a
        // replacement-field.
        return ctf::create_format_error(
            (arg_id.offset == fmt.size()
                 ? "unexpected end of the format string"
                 : "unexpected character in the format string"),
            fmt, P::offset, arg_id.offset, arg_id.offset,
            "}     -> the end of the replacement-field", //
            ":     -> start of the format-specifier",    //
            "[0-9] -> continuation of the arg-id");
    } else {

      using T = std::remove_reference_t<pack_type<arg_id.index, Args...>>;

      if constexpr (!std::formattable<T, char>)
        return ctf::create_format_error(
            "the supplied type for the argument is not formattable", fmt,
            P::offset, arg_id.offset, arg_id.offset);

      else {
        auto result = ctf::formatter<
            T, fmt, arg_id.offset + std::size_t(fmt[arg_id.offset] == ':')

                        ,

            arg_id.status, Args...>::create();

        if constexpr (ctf::is_format_error(result))
          return result;
        else {

          auto tokens = ctf::tuple_append(
              status.tokens,
              output_replacement_field<P::offset, arg_id.index,
                                       decltype(result.formatter)>{
                  result.formatter});

          return parse<fmt, Args...>(
              parser_status<result.offset + 1, result.arg_id, decltype(tokens)>{
                  tokens}

          );
        }
      }
    }
  }
}

template <fixed_string Fmt, class... Args> consteval auto parse(auto status) {
  using P = decltype(status);
  if constexpr (P::offset >= Fmt.size()) {
    return status;
  } else if constexpr (Fmt[P::offset] == '{') {
    // handle {{
    if constexpr (Fmt[P::offset + 1] == '{') {

      auto tokens = append_char<P::offset>(status.tokens);
      return parse<Fmt, Args...>(
          parser_status<P::offset + 2, P::arg_id, decltype(tokens)>{tokens});

    } else
      return handle_replacement_field3<Fmt, Args...>(status);

  } else if constexpr (Fmt[P::offset] == '}') {
    // handle }}
    if constexpr (Fmt[P::offset + 1] == '}') {

      auto tokens = append_char<P::offset>(status.tokens);
      return parse<Fmt, Args...>(
          parser_status<P::offset + 2, P::arg_id, decltype(tokens)>{tokens});

    } else
      return create_format_error("expected '}' in escape sequence", Fmt, 0,
                                 P::offset, P::offset, "}");
  } else {
    auto tokens = append_char<P::offset>(status.tokens);
    return parse<Fmt, Args...>(
        parser_status<P::offset + 1, P::arg_id, decltype(tokens)>{tokens});
  }
}

template <fixed_string fmt, class... Args> consteval auto parse() {
  return parse<fmt,
               typename format_arg<char, std::remove_cvref_t<Args>>::type...>(
      parser_status<0, arg_id_status<index_mode::unknown, 0, sizeof...(Args)>{},
                    tuple<>>{});
}

template <fixed_string Fmt, class... Args>
constexpr std::string format_tokens(auto tokens, Args &...args) {
  std::string result;
  std::tuple t{
      typename format_arg<char, std::remove_cvref_t<Args>>::type{args}...};

  std::__for_each_index_sequence(
      std::make_index_sequence<ctf::tuple_size<decltype(tokens)>>(),
      [&]<std::size_t I> {
        using T = ctf::tuple_type<I, decltype(tokens)>;
        const auto &token = tokens.template get<T>();

        if constexpr (std::same_as<typename T::tag, output_char_tag>)
          result.push_back(Fmt[T::offset]);
        else if constexpr (std::same_as<typename T::tag, output_text_tag>)
          result.append(&Fmt[T::offset], &Fmt[T::offset + T::size]);
        else if constexpr (std::same_as<typename T::tag,
                                        output_replacement_field_tag>) {

          const auto &v = std::get<T::index>(t);

          using OutIt = std::back_insert_iterator<std::string>;
          using Context = std::basic_format_context<OutIt, char>;

          auto format_args = std::make_format_args<Context>(args...);
          auto context = std::__format_context_create<OutIt, char>(
              std::back_inserter(result), format_args);

          token.formatter.format(v, context);
        } else
          static_assert(false, "type not supported");
      });
  return result;
}

template <fixed_string fmt, class... Args>
concept valid = !ctf::is_format_error(parse<fmt, Args...>());

template <fixed_string fmt, class... Args>
constexpr std::string format(Args &&...args) {
  constexpr auto status = parse<fmt, Args...>();

  if constexpr (ctf::is_format_error(status))
    static_assert(!"parse error", status);
  else
    return format_tokens<fmt>(status.tokens, args...);
}

} // namespace ctf

#endif // CTF_FORMAT_HPP
