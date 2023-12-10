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
}
