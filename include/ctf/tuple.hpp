//===----------------------------------------------------------------------===//
//
// Part of the CTF project, under the Apache License v2.0 with LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef CTF_TUPLE_HPP
#define CTF_TUPLE_HPP

/**
 * @file A simple tuple class.
 *
 * This class is used to store the tokens of the parser. Since every token
 * stores its offset all tokens have a unique type. This avoids the need to
 * store duplicated types, simplifying the design.
 *
 * Since most types store no data and otherwise are trivially copyable there is
 * no need to support rvalue references.
 */

#include "utility.hpp"

#include <cstddef>
#include <type_traits>
#include <utility>

namespace ctf {

template <class T> struct tuple_element {
  constexpr explicit tuple_element(const T &v) : value{v} {}

  [[no_unique_address]] T value;
};

template <class... Args> struct tuple_data : public tuple_element<Args>... {
  constexpr explicit tuple_data(const Args &...args)
      : tuple_element<Args>{args}... {}
};

template <class... Args> class tuple {
public:
  constexpr explicit tuple(const Args &...args) : data_{args...} {}

  template <class T> constexpr const T &get() const noexcept {
    return static_cast<const tuple_element<T> &>(data_).value;
  }

private:
  [[no_unique_address]] tuple_data<Args...> data_;
};

template <> class tuple<> {};

namespace detail {
template <class T> struct tuple_size;

template <class... Args> struct tuple_size<tuple<Args...>> {
  static constexpr std::size_t size = sizeof...(Args);
};

template <std::size_t, class T> struct tuple_type;

template <std::size_t I, class... Args> struct tuple_type<I, tuple<Args...>> {
  using type = pack_type<I, Args...>::type;
};

} // namespace detail
template <class T>
inline constexpr std::size_t tuple_size = detail::tuple_size<T>::size;

template <std::size_t I, class T>
using tuple_type = detail::tuple_type<I, T>::type;

template <class... Args> constexpr auto make_tuple(const Args &...args) {
  return tuple<std::remove_cvref_t<Args>...>{args...};
}

template <std::size_t I, class T>
constexpr const auto &get(const T &tuple) noexcept {
  return tuple.template get<ctf::tuple_type<I, T>>();
}

// Adds a new element at the end of the tuple.
// This acts like std::tuple_cat for two elements withouth the need to wrap the
// new element in a tuple before concatenating.
template <class T, class... Args>
constexpr tuple<Args..., T> tuple_append(const tuple<Args...> &t, const T &v) {
  if constexpr (sizeof...(Args) == 0)
    return tuple<std::remove_cvref_t<T>>{v};
  else
    return [&]<std::size_t... I>(std::index_sequence<I...>) {
      return ctf::make_tuple(ctf::get<I>(t)..., v);
    }(std::make_index_sequence<sizeof...(Args)>());
}

// Replaces the last element of the tuple with a different type.
// This is something used during parsing the format string.
// When parsing a char and the last parsed element is a char or a text a new
// text element replaces the last element.
template <class T, class... Args>
  requires(sizeof...(Args) != 0)
constexpr auto tuple_replace_back(const tuple<Args...> &t, const T &v) {
  return [&]<std::size_t... I>(std::index_sequence<I...>) {
    return ctf::make_tuple(ctf::get<I>(t)..., v);
  }(std::make_index_sequence<sizeof...(Args) - 1>());
}

} // namespace ctf

#endif // CTF_TUPLE_HPP
