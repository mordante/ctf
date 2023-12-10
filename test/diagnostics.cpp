//===----------------------------------------------------------------------===//
//
// Part of the CTF project, under the Apache License v2.0 with LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// Tests the clang diagnostics.
//
// Validates the hand-crafted static_assert messages.

#include "ctf/format.hpp"

enum class not_formattable {};

int test() {

  // expected-error-re@*:* {{static assertion failed due to requirement '!"parse error"':{{.*}}\
unexpected end of the format string{{.*}}\
{{{.*}}\
~^{{.*}}\
 {     -> an escaped {{{.*}}\
 }     -> the end of the replacement-field{{.*}}\
 :     -> start of the format-specifier{{.*}}\
 [0-9] -> an arg-id{{.*}}\
}}
  // expected-note@+1 {{in instantiation of function template specialization}}
  (void)ctf::format<"{">();

  // expected-error-re@*:* {{static assertion failed due to requirement '!"parse error"':{{.*}}\
unexpected end of the format string{{.*}}\
test tilde location {{{.*}}\
                    ~^{{.*}}\
                     {     -> an escaped {{{.*}}\
                     }     -> the end of the replacement-field{{.*}}\
                     :     -> start of the format-specifier{{.*}}\
                     [0-9] -> an arg-id{{.*}}\
}}
  // expected-note@+1 {{in instantiation of function template specialization}}
  (void)ctf::format<"test tilde location {">();

  // expected-error-re@*:* {{static assertion failed due to requirement '!"parse error"':{{.*}}\
unexpected character in the format string{{.*}}\
invalid character {a}{{.*}}\
                  ~^{{.*}}\
                   {     -> an escaped {{{.*}}\
                   }     -> the end of the replacement-field{{.*}}\
                   :     -> start of the format-specifier{{.*}}\
                   [0-9] -> an arg-id{{.*}}\
}}
  // expected-note@+1 {{in instantiation of function template specialization}}
  (void)ctf::format<"invalid character {a}">();

  // TODO Fix this case, the parser dies
  // (void)ctf::format<"{:">();

  // TODO Several other messages are wrong and need fixing

  // expected-error-re@*:* {{static assertion failed due to requirement '!"parse error"':{{.*}}\
unexpected end of the format string{{.*}}\
unexpected end in arg-id {0{{.*}}\
                         ~~^{{.*}}\
                           }     -> the end of the replacement-field{{.*}}\
                           :     -> start of the format-specifier{{.*}}\
                           [0-9] -> continuation of the arg-id{{.*}}\
}}
  // expected-note@+1 {{in instantiation of function template specialization}}
  (void)ctf::format<"unexpected end in arg-id {0">();

  // TODO
  // Pedantically this message is wrong; the arg-id has reached its maximum
  // value. Neither does the suggestion take the number of arguments supplied
  // into account.

  // expected-error-re@*:* {{static assertion failed due to requirement '!"parse error"':{{.*}}\
unexpected end of the format string{{.*}}\
unexpected end in arg-id {2147483647{{.*}}\
                         ~~~~~~~~~~~^{{.*}}\
                                    }     -> the end of the replacement-field{{.*}}\
                                    :     -> start of the format-specifier{{.*}}\
                                    [0-9] -> continuation of the arg-id{{.*}}\
}}
  // expected-note@+1 {{in instantiation of function template specialization}}
  (void)ctf::format<"unexpected end in arg-id {2147483647">();

  // expected-error-re@*:* {{static assertion failed due to requirement '!"parse error"':{{.*}}\
unexpected character in the format string{{.*}}\
unexpected end in arg-id {0a{{.*}}\
                         ~~^{{.*}}\
                           }     -> the end of the replacement-field{{.*}}\
                           :     -> start of the format-specifier{{.*}}\
                           [0-9] -> continuation of the arg-id{{.*}}\
}}
  // expected-note@+1 {{in instantiation of function template specialization}}
  (void)ctf::format<"unexpected end in arg-id {0a">();

  // expected-error-re@*:* {{static assertion failed due to requirement '!"parse error"':{{.*}}\
expected '}' in escape sequence{{.*}}\
}{{.*}}\
^{{.*}}\
}}
  // expected-note@+1 {{in instantiation of function template specialization}}
  (void)ctf::format<"}">();

  // expected-error-re@*:* {{static assertion failed due to requirement '!"parse error"':{{.*}}\
using the first argument while zero arguments are available{{.*}}\
{}{{.*}}\
~^{{.*}}\
}}
  // expected-note@+1 {{in instantiation of function template specialization}}
  (void)ctf::format<"{}">();

  // expected-error-re@*:* {{static assertion failed due to requirement '!"parse error"':{{.*}}\
using the first argument while zero arguments are available{{.*}}\
{:foo}{{.*}}\
~^{{.*}}\
}}
  // expected-note@+1 {{in instantiation of function template specialization}}
  (void)ctf::format<"{:foo}">();

  // expected-error-re@*:* {{static assertion failed due to requirement '!"parse error"':{{.*}}\
using the second argument while one argument is available{{.*}}\
{} {}{{.*}}\
   ~^{{.*}}\
}}
  // expected-note@+1 {{in instantiation of function template specialization}}
  (void)ctf::format<"{} {}">(42);

  // expected-error-re@*:* {{static assertion failed due to requirement '!"parse error"':{{.*}}\
using the second argument while one argument is available{{.*}}\
{1}{{.*}}\
 ~^{{.*}}\
}}
  // expected-note@+1 {{in instantiation of function template specialization}}
  (void)ctf::format<"{1}">(42);

  // expected-error-re@*:* {{static assertion failed due to requirement '!"parse error"':{{.*}}\
using the 11-th argument while two arguments are available{{.*}}\
{10:bar}{{.*}}\
 ~~^{{.*}}\
}}
  // expected-note@+1 {{in instantiation of function template specialization}}
  (void)ctf::format<"{10:bar} {1}">(42, 99);

  // expected-error-re@*:* {{static assertion failed due to requirement '!"parse error"':{{.*}}\
using a manual argument id while in automatic index mode{{.*}}\
{} {0}{{.*}}\
    ~^{{.*}}\
}}
  // expected-note@+1 {{in instantiation of function template specialization}}
  (void)ctf::format<"{} {0}">(42);

  // expected-error-re@*:* {{static assertion failed due to requirement '!"parse error"':{{.*}}\
using a manual argument id while in automatic index mode{{.*}}\
{} {42:foo}{{.*}}\
    ~~^{{.*}}\
}}
  // expected-note@+1 {{in instantiation of function template specialization}}
  (void)ctf::format<"{} {42:foo}">(42);

  // expected-error-re@*:* {{static assertion failed due to requirement '!"parse error"':{{.*}}\
using a automatic argument id while in manual index mode{{.*}}\
{0} {}{{.*}}\
    ~^{{.*}}\
}}
  // expected-note@+1 {{in instantiation of function template specialization}}
  (void)ctf::format<"{0} {}">(42);

  // expected-error-re@*:* {{static assertion failed due to requirement '!"parse error"':{{.*}}\
using a automatic argument id while in manual index mode{{.*}}\
{0} {:foo}{{.*}}\
    ~^{{.*}}\
}}
  // expected-note@+1 {{in instantiation of function template specialization}}
  (void)ctf::format<"{0} {:foo}">(42);

  // expected-error-re@*:* {{static assertion failed due to requirement '!"parse error"':{{.*}}\
the supplied type for the argument is not formattable{{.*}}\
{}{{.*}}\
~^{{.*}}\
}}
  // expected-note@+1 {{in instantiation of function template specialization}}
  (void)ctf::format<"{}">(not_formattable{});

  // expected-error-re@*:* {{static assertion failed due to requirement '!"parse error"':{{.*}}\
the supplied type for the argument is not formattable{{.*}}\
{:not parsed at all}{{.*}}\
~^{{.*}}\
}}
  // expected-note@+1 {{in instantiation of function template specialization}}
  (void)ctf::format<"{:not parsed at all}">(not_formattable{});

  // expected-error-re@*:* {{static assertion failed due to requirement '!"parse error"':{{.*}}\
the supplied type for the argument is not formattable{{.*}}\
{0}{{.*}}\
~~^{{.*}}\
}}
  // expected-note@+1 {{in instantiation of function template specialization}}
  (void)ctf::format<"{0}">(not_formattable{});

  // expected-error-re@*:* {{static assertion failed due to requirement '!"parse error"':{{.*}}\
the supplied type for the argument is not formattable{{.*}}\
{0:not parsed at all}{{.*}}\
~~^{{.*}}\
}}
  // expected-note@+1 {{in instantiation of function template specialization}}
  (void)ctf::format<"{0:not parsed at all}">(not_formattable{});

  // Note the output is modified by Clang so our selection position is wrong.
  // TODO Escape the output

  // expected-error-re@*:* {{static assertion failed due to requirement '!"parse error"':{{.*}}\
the UTF-8 code point starts with a continuation code unit{{.*}}\
UTF-8 continuation at start {:<80>}{{.*}}\
                              ^{{.*}}\
}}
  // expected-note@+1 {{in instantiation of function template specialization}}
  (void)ctf::format<"UTF-8 continuation at start {:\x80}">(std::string_view{});

  // expected-error-re@*:* {{static assertion failed due to requirement '!"parse error"':{{.*}}\
the UTF-8 code point starts with an invalid code unit{{.*}}\
invalid UTF-8 code unit {:<FD>}{{.*}}\
                          ^{{.*}}\
}}
  // expected-note@+1 {{in instantiation of function template specialization}}
  (void)ctf::format<"invalid UTF-8 code unit {:\xFD}">(std::string_view{});

  // expected-error-re@*:* {{static assertion failed due to requirement '!"parse error"':{{.*}}\
expected UTF-8 continuation code unit{{.*}}\
UTF-8 2 code unit code point {:<C0>#}{{.*}}\
                               ~^{{.*}}\
}}
  // expected-note@+1 {{in instantiation of function template specialization}}
  (void)ctf::format<"UTF-8 2 code unit code point {:\xC0#}">(
      std::string_view{});

  // expected-error-re@*:* {{static assertion failed due to requirement '!"parse error"':{{.*}}\
expected UTF-8 continuation code unit{{.*}}\
UTF-8 3 code unit code point {:<E0><80>#}{{.*}}\
                               ~~^{{.*}}\
}}
  // expected-note@+1 {{in instantiation of function template specialization}}
  (void)ctf::format<"UTF-8 3 code unit code point {:\xE0\x80#}">(
      std::string_view{});

  // expected-error-re@*:* {{static assertion failed due to requirement '!"parse error"':{{.*}}\
expected UTF-8 continuation code unit{{.*}}\
UTF-8 4 code unit code point {:<F0><80><80>#}{{.*}}\
                               ~~~^{{.*}}\
}}
  // expected-note@+1 {{in instantiation of function template specialization}}
  (void)ctf::format<"UTF-8 4 code unit code point {:\xF0\x80\x80#}">(
      std::string_view{});

  // expected-error-re@*:* {{static assertion failed due to requirement '!"parse error"':{{.*}}\
the format specification does not allow the sign option{{.*}}\
sign not allowed {: }{{.*}}\
                   ^{{.*}}\
}}
  // expected-note@+1 {{in instantiation of function template specialization}}
  (void)ctf::format<"sign not allowed {: }">(std::string_view{});

  // expected-error-re@*:* {{static assertion failed due to requirement '!"parse error"':{{.*}}\
the format specification does not allow the alternate form option{{.*}}\
alternate form not allowed {:#}{{.*}}\
                             ^{{.*}}\
}}
  // expected-note@+1 {{in instantiation of function template specialization}}
  (void)ctf::format<"alternate form not allowed {:#}">(std::string_view{});

  // expected-error-re@*:* {{static assertion failed due to requirement '!"parse error"':{{.*}}\
the format specification does not allow the zero-padding option{{.*}}\
zero-padding not allowed {:0}{{.*}}\
                           ^{{.*}}\
}}
  // expected-note@+1 {{in instantiation of function template specialization}}
  (void)ctf::format<"zero-padding not allowed {:0}">(std::string_view{});

  // expected-error-re@*:* {{static assertion failed due to requirement '!"parse error"':{{.*}}\
the display type is not valid for a string argument{{.*}}\
invalid display type {:A}{{.*}}\
                       ^{{.*}}\
}}
  // expected-note@+1 {{in instantiation of function template specialization}}
  (void)ctf::format<"invalid display type {:A}">(std::string_view{});

  // expected-error-re@*:* {{static assertion failed due to requirement '!"parse error"':{{.*}}\
the argument index may not be a negative value{{.*}}\
egative arg-id {0:{-1}{{.*}}\
                  ~^{{.*}}\
}}
  // expected-note@+1 {{in instantiation of function template specialization}}
  (void)ctf::format<"negative arg-id {0:{-1}}">(std::string_view{});

  // expected-error-re@*:* {{static assertion failed due to requirement '!"parse error"':{{.*}}\
the argument index has a leading zero{{.*}}\
negative arg-id {0:{0001}}{{.*}}\
                    ~^~~{{.*}}\
}}
  // expected-note@+1 {{in instantiation of function template specialization}}
  (void)ctf::format<"negative arg-id {0:{0001}}">(std::string_view{}, 42);

  // expected-error-re@*:* {{static assertion failed due to requirement '!"parse error"':{{.*}}\
the value of the argument index is larger than the implementation supports (2147483647){{.*}}\
arg-id value too large {0:{2147483648}}{{.*}}\
                           ~~~~~~~~~^{{.*}}\
}}
  // expected-note@+1 {{in instantiation of function template specialization}}
  (void)ctf::format<"arg-id value too large {0:{2147483648}}">(
      std::string_view{});

  // expected-error-re@*:* {{static assertion failed due to requirement '!"parse error"':{{.*}}\
the value of the argument index is larger than the implementation supports (2147483647){{.*}}\
arg-id value too large {0:{2147483647000}}{{.*}}\
                           ~~~~~~~~~~^{{.*}}\
}}
  // expected-note@+1 {{in instantiation of function template specialization}}
  (void)ctf::format<"arg-id value too large {0:{2147483647000}}">(
      std::string_view{});

  // expected-error-re@*:* {{static assertion failed due to requirement '!"parse error"':{{.*}}\
using the second argument while one argument is available{{.*}}\
arg-id out of bounds {0:{1}}{{.*}}\
                         ^{{.*}}\
}}
  // expected-note@+1 {{in instantiation of function template specialization}}
  (void)ctf::format<"arg-id out of bounds {0:{1}}">(std::string_view{});

  // expected-error-re@*:* {{static assertion failed due to requirement '!"parse error"':{{.*}}\
the type of the arg-id is not a standard signed or unsigned integer type{{.*}}\
invalid type for width {0:{0}}{{.*}}\
                          ~~~^{{.*}}\
}}
  // expected-note@+1 {{in instantiation of function template specialization}}
  (void)ctf::format<"invalid type for width {0:{0}}">(std::string_view{});

  // expected-error-re@*:* {{static assertion failed due to requirement '!"parse error"':{{.*}}\
the type of the arg-id is not a standard signed or unsigned integer type{{.*}}\
invalid type for width {:{}}{{.*}}\
                         ~^{{.*}}\
}}
  // expected-note@+1 {{in instantiation of function template specialization}}
  (void)ctf::format<"invalid type for width {:{}}">(std::string_view{}, '*');

  // expected-error-re@*:* {{static assertion failed due to requirement '!"parse error"':{{.*}}\
unexpected character in the arg-id{{.*}}\
arg-id not closed {0:{1s}}{{.*}}\
                      ~^{{.*}}\
                       }     -> the end of the replacement-field{{.*}}\
                       [0-9] -> an arg-id{{.*}}\
}}
  // expected-note@+1 {{in instantiation of function template specialization}}
  (void)ctf::format<"arg-id not closed {0:{1s}}">(std::string_view{}, 42);
}
