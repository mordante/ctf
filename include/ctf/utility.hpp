//===----------------------------------------------------------------------===//
//
// Part of the CTF project, under the Apache License v2.0 with LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef CTF_UTILITY_HPP
#define CTF_UTILITY_HPP

#include <cstddef>

namespace ctf {
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
