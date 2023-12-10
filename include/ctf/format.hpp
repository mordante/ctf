//===----------------------------------------------------------------------===//
//
// Part of the CTF project, under the Apache License v2.0 with LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef CTF_FORMAT_HPP
#define CTF_FORMAT_HPP

#include "tuple.hpp"
#include "utility.hpp"

#include <cstddef>
#include <format>
#include <tuple>

namespace ctf {

template <std::size_t Size> struct fixed_string {
  constexpr fixed_string(const char (&r)[Size]) {
    __builtin_memcpy(text, r, Size);
  }
  char text[Size];

  // The size of the array shouldn't include the NUL character.
  static constexpr std::size_t size = Size - 1;
};

template <std::size_t Size>
fixed_string(const char (&)[Size]) -> fixed_string<Size>;

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

// unknown means no replacement field found yet.
enum class Mode { unknown, manual, automatic };

template <std::size_t Offset = 0, std::size_t Index = 0 /* of arg-id */,
          Mode M = Mode::unknown, class Tuple = tuple<>>
struct ttoken_list {
  static constexpr std::size_t offset = Offset;
  static constexpr std::size_t index = Index;
  static constexpr Mode mode = M;

  Tuple data;
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

  if constexpr (Fmt.text[R::offset] == '}') {
    if constexpr (R::level == 0)
      return R{};
    else
      return parse_replacement_field<
          replacement_field<R::offset + 1, R::colon, R::id, R::level - 1>,
          Fmt>();
  } else if constexpr (R::colon == -1) {
    if constexpr (Fmt.text[R::offset] == ':')
      return parse_replacement_field<
          replacement_field<R::offset + 1, R::offset, R::id, R::level>, Fmt>();
    else if constexpr (Fmt.text[R::offset] >= '0' &&
                       Fmt.text[R::offset] <= '9') {
      constexpr std::size_t d = Fmt.text[R::offset] - '0';
      if constexpr (R::id == -1)
        return parse_replacement_field<
            replacement_field<R::offset + 1, R::colon, d, R::level>, Fmt>();
      else
        return parse_replacement_field<
            replacement_field<R::offset + 1, R::colon, R::id * 10 + d, R::level>,
            Fmt>();
    } else
      static_assert(false, "The argument index should end with a ':' or a '}'");
  } else if constexpr (Fmt.text[R::offset] == '{')
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

template <fixed_string Fmt, class... Args>
consteval auto parse(auto token_list) {
  using TL = decltype(token_list);
  if constexpr (TL::offset >= Fmt.size) {
    return token_list;
  } else if constexpr (Fmt.text[TL::offset] == '{') {
    // handle {{
    if constexpr (Fmt.text[TL::offset + 1] == '{') {
      auto t = append_char<TL::offset>(token_list.data);
      using T = decltype(t);
      return parse<Fmt, Args...>(
          ttoken_list<TL::offset + 2, TL::index, TL::mode, T>{t});
    } else {
      // handle replacement field
      using replacement =
          decltype(parse_replacement_field<replacement_field<TL::offset + 1>,
                                           Fmt>());
      static_assert(Fmt.text[replacement::offset] == '}',
                    "The replacement field misses a terminating '}'");

      using A = std::remove_reference_t<pack_type<TL::index, Args...>>;

      if constexpr (!std::formattable<A, char>) {
        constexpr std::string_view sv{type_name<A>()};
        static_assert(false, "The supplied argument type is not formattable");
      } else {
        using F = std::formatter<A, char>;
        F f;
        if constexpr (replacement::colon == -1) {
          // empty string
          std::format_parse_context parse_ctx{std::string_view{},
                                              sizeof...(Args)};
          f.parse(parse_ctx);
        } else {
          std::format_parse_context parse_ctx{
              std::string_view{&Fmt.text[replacement::colon + 1],
                               &Fmt.text[replacement::offset + 1]},
              sizeof...(Args)};
          f.parse(parse_ctx);
        }

        if constexpr (TL::mode == Mode::unknown) {
          if constexpr (replacement::id == -1) {
            static_assert(0 < sizeof...(Args),
                          "The argument index value is too large for the "
                          "number of arguments supplied");

            auto t = ctf::tuple_append(
                token_list.data, output_replacement_field<TL::offset, 0, F>{f});
            using T = decltype(t);
            return parse<Fmt, Args...>(
                ttoken_list<replacement::offset + 1, 1, Mode::automatic, T>{t});

          } else {
            static_assert(replacement::id < sizeof...(Args),
                          "The argument index value is too large for the "
                          "number of arguments supplied");

            auto t = ctf::tuple_append(
                token_list.data,
                output_replacement_field<TL::offset, TL::index, F>{f});

            using T = decltype(t);
            return parse<Fmt, Args...>(
                ttoken_list<replacement::offset + 1, 0, Mode::manual, T>{t});
          }
        } else if constexpr (TL::mode == Mode::manual) {
          static_assert(replacement::id != -1,
                        "can't use automatic mode after selecting manual mode");
          static_assert(replacement::id < sizeof...(Args),
                        "The argument index value is too large for the number "
                        "of arguments supplied");

          auto t = ctf::tuple_append(
              token_list.data,
              output_replacement_field<TL::offset, replacement::id, F>{f});

          using T = decltype(t);
          return parse<Fmt, Args...>(
              ttoken_list<replacement::offset + 1, 0, Mode::manual, T>{t});

        } else /* automatic */ {
          static_assert(replacement::id == -1,
                        "can't use manual mode after selecting automatic mode");
          static_assert(TL::index < sizeof...(Args),
                        "The argument index value is too large for the number "
                        "of arguments supplied");

          auto t = ctf::tuple_append(
              token_list.data,
              output_replacement_field<TL::offset, TL::index, F>{f});

          using T = decltype(t);
          return parse<Fmt, Args...>(
              ttoken_list<replacement::offset + 1, TL::index + 1,
                          Mode::automatic, T>{t});
        }
      }
    }

  } else if constexpr (Fmt.text[TL::offset] == '}') {
    // handle }}
    static_assert(Fmt.text[TL::offset + 1] == '}',
                  "The format string contains an invalid escape sequence");
    auto t = append_char<TL::offset>(token_list.data);
    using T = decltype(t);
    return parse<Fmt, Args...>(
        ttoken_list<TL::offset + 2, TL::index, TL::mode, T>{t});

  } else {
    auto t = append_char<TL::offset>(token_list.data);
    using T = decltype(t);
    return parse<Fmt, Args...>(
        ttoken_list<TL::offset + 1, TL::index, TL::mode, T>{t});
  }
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
          result.push_back(Fmt.text[T::offset]);
        else if constexpr (std::same_as<typename T::tag, output_text_tag>)
          result.append(&Fmt.text[T::offset], &Fmt.text[T::offset + T::size]);
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
constexpr std::string format(Args &&...args) {
  constexpr auto token_list =
      parse<fmt, typename format_arg<char, std::remove_cvref_t<Args>>::type...>(
          ttoken_list<>{});

  return format_tokens<fmt>(token_list.data, args...);
}

} // namespace ctf

#endif // CTF_FORMAT_HPP
