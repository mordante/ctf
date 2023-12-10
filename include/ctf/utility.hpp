//===----------------------------------------------------------------------===//
//
// Part of the CTF project, under the Apache License v2.0 with LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef CTF_UTILITY_HPP
#define CTF_UTILITY_HPP

#include <algorithm>
#include <array>
#include <charconv>
#include <cstddef>
#include <string>

namespace ctf {

template <std::size_t Size> struct fixed_string {
  using char_type = char;
  constexpr fixed_string(const char (&r)[Size]) {
    __builtin_memcpy(text, r, Size);
  }
  char text[Size];

  // The size of the array shouldn't include the NUL character.
  static constexpr std::size_t size = Size - 1;
};

template <std::size_t Size>
fixed_string(const char (&)[Size]) -> fixed_string<Size>;

consteval std::string to_string(std::size_t v) {
  char buffer[20];
  return {buffer, std::to_chars(buffer, &buffer[20], v).ptr};
}

namespace detail {
template <class T> struct tuple_size;

template <std::size_t I, class T, class... Args> struct pack_type {
  using type = pack_type<I - 1, Args...>::type;
};

template <class T, class... Args> struct pack_type<0, T, Args...> {
  using type = T;
};
} // namespace detail

template <std::size_t I, class... Args>
using pack_type = detail::pack_type<I, Args...>::type;

} // namespace ctf

#endif // CTF_UTILITY_HPP
