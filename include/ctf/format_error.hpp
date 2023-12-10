//===----------------------------------------------------------------------===//
//
// Part of the CTF project, under the Apache License v2.0 with LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef CTF_FORMAT_ERROR_HPP
#define CTF_FORMAT_ERROR_HPP

#include <algorithm>
#include <array>
#include <cstddef>
#include <string>

namespace ctf {

// The error message class to be used for errors in during parsing.
// Since the code is executed compile-time it is not possible to return a
// heap-allocated object. Instead use a static buffer, when the buffer is too
// small the string is truncated.
class format_error {
public:
  consteval format_error(std::string &&m)
      : size_(std::min(m.size(), data_.size())) {
    auto it = std::copy_n(m.data(), size_, data_.begin());
    if (size_ < data_.size())
      std::fill(it, data_.end(), '\0');
    else if (m.size() > data_.size())
      // Horizontal Ellipsis is 3 UTF-8 code units.
      std::ranges::copy(std::string_view{"\u2026"}, &data_[data_.size() - 4]);
  }

  consteval std::size_t size() const noexcept { return size_; }
  consteval const char *data() const noexcept { return data_.data(); }

private:
  std::array<char, 4096> data_;
  std::size_t size_;
};

template <class T> consteval bool is_format_error(const T &) {
  return std::same_as<format_error, std::remove_cvref_t<T>>;
}

template <class... Fixits>
consteval format_error create_format_error(
    std::string &&message, std::string &&fmt,
    int begin,       /* offset to start of selection area */
    int caret,       /* position of the caret */
    int end,         /* end of the selection area */
    Fixits... fixits /* fixits under the caret, each on its own line */
) {
  int pre = caret - begin;
  int post = end - caret;

  return {(

      ('\n' + message + '\n' +                                   //
       fmt + +"\n" +                                             //
       std::string(begin, ' ') +                                 //
       (pre > 0 ? std::string(pre, '~') : std::string{}) + '^' + //
       (post > 0 ? std::string(post, '~') : std::string{}) +     //
       '\n')                                                     //

      + ... + (std::string(caret, ' ') + fixits + '\n'))};
}

} // namespace ctf

#endif // CTF_FORMAT_ERROR_HPP
