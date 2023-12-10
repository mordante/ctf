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

// Compile-time c string wrapper class.
//
// Users may assume the NUL terminator present so
//   fixed_string f;
//   f[f.size()];
// is valid and returns the NUL terminator.
//
// This makes the code for the formatter a lot simpler, no need to do a bouds
// check on the size of the input. The NUL character is not used in the parser
// so it will never match.
template <class CharT, std::size_t Size> class fixed_string {
public:
  using char_type = CharT;

  consteval fixed_string(const CharT (&r)[Size]) {
    __builtin_memcpy(private_str_, r, Size);
  }

  // The size of the array shouldn't include the NUL character.
  consteval std::size_t size() const { return Size - 1; }

  consteval const CharT &operator[](std::size_t i) const {
    return private_str_[i];
  }

  consteval operator std::string() const { return std::string(private_str_); }

  // When the class has a private member it's no longer a structural type and
  // can't be used as an template argument.
  CharT private_str_[Size];
};

template <std::size_t Size>
fixed_string(const char (&)[Size]) -> fixed_string<char, Size>;

template <std::size_t Size>
fixed_string(const wchar_t (&)[Size]) -> fixed_string<wchar_t, Size>;

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
