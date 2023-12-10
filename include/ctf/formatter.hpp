//===----------------------------------------------------------------------===//
//
// Part of the CTF project, under the Apache License v2.0 with LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef CTF_FORMATTER_HPP
#define CTF_FORMATTER_HPP

#include "format_error.hpp"
#include "parse.hpp"
#include "utility.hpp"

#include <cstdint>
#include <format>

namespace ctf {
template <std::size_t o, arg_id_status i, class F> struct formatter_result {
  // The offset at the end.
  static constexpr std::size_t offset = o;
  // The index of the next arg_id to be parsed.
  //
  // This is only used in automatic indexing mode. This allows the usage of an
  // arg-id for the width and precision.
  static constexpr arg_id_status arg_id = i;

  // The formatter with its parse() function called.
  // This call is required to be done before calling the format member function.
  F formatter;
};

// This class is intended as a custumization point for the type T.
//
// This is a stub parser that skips over the format-spec, assuming {} pairs are
// balanced. When the format-spec uses arg-ids in automatic mode the index
// won't be updated.
template <class T, fixed_string fmt, std::size_t begin, arg_id_status arg_id,
          class... Args>
struct formatter {

  static consteval auto create() {
    constexpr std::size_t end =
        [&]<std::size_t offset, std::size_t level>(this auto self) {
          if constexpr (offset == fmt.size())
            return offset;
          else {
            constexpr auto c = fmt[offset];
            if constexpr (c == '}') {
              if constexpr (level == 0)
                return offset;
              else
                return self.template operator()<offset + 1, level - 1>();
            } else if (c == '{')
              return self.template operator()<offset + 1, level + 1>();
            else
              return self.template operator()<offset + 1, level>();
          }
        }.template operator()<begin, 0>();

    if constexpr (fmt[end] != '}')
      return ctf::create_format_error(
          "unable to find the end of the format-spec", fmt, begin, end, end);
    else {
      using CharT = decltype(fmt)::char_type;
      using F = std::formatter<T, CharT>;
      F f;

      // &fmt[fmt::size()] is valid; it points to the NUL terminator.
      std::format_parse_context parse_ctx{
          std::string_view{&fmt[begin], &fmt[fmt.size()]}, sizeof...(Args)};
      auto it = f.parse(parse_ctx);

      if (it != &fmt[end])
        throw std::format_error(
            "format specifiers with unbalanced {} pairs are not supported");

      return formatter_result<end, arg_id, F>{f};
    }
  }
};

} // namespace ctf

#endif // CTF_FORMATTER_HPP
