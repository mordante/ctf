//===----------------------------------------------------------------------===//
//
// Part of the CTF project, under the Apache License v2.0 with LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "ctf/format.hpp"

static_assert(ctf::valid<"">);

static_assert(!ctf::valid<"{">);
static_assert(ctf::valid<"{{">);

static_assert(!ctf::valid<"}">);
static_assert(ctf::valid<"}}">);

static_assert(!ctf::valid<"{}">);
static_assert(ctf::valid<"{}", int>);
static_assert(!ctf::valid<"{33:}", int>);

static_assert(ctf::valid<"{}", std::string_view>);
static_assert(!ctf::valid<"{:+}", std::string_view>);
static_assert(!ctf::valid<"{:-}", std::string_view>);
static_assert(!ctf::valid<"{: }", std::string_view>);
static_assert(!ctf::valid<"{:#}", std::string_view>);
static_assert(!ctf::valid<"{:0}", std::string_view>);
static_assert(ctf::valid<"{:1}", std::string_view>);
static_assert(ctf::valid<"{:10}", std::string_view>);
static_assert(ctf::valid<"{:2147483646}", std::string_view>);
static_assert(ctf::valid<"{:2147483647}", std::string_view>); // maximum value
static_assert(!ctf::valid<"{:2147483648}", std::string_view>);
static_assert(!ctf::valid<"{:2147483647000}", std::string_view>);

static_assert(!ctf::valid<"{0:{-1}}", std::string_view>);
static_assert(!ctf::valid<"{0:{2147483648}}", std::string_view>);
static_assert(!ctf::valid<"{0:{2147483647000}}", std::string_view>);

static_assert(!ctf::valid<"{0:{0}}", std::string_view>);
static_assert(!ctf::valid<"{:{}}", std::string_view, char>);
static_assert(!ctf::valid<"{:{}}", std::string_view, bool>);
static_assert(ctf::valid<"{:{}}", std::string_view, signed char>);
static_assert(ctf::valid<"{:{}}", std::string_view, short>);
static_assert(ctf::valid<"{:{}}", std::string_view, int>);
static_assert(ctf::valid<"{:{}}", std::string_view, long>);
static_assert(ctf::valid<"{:{}}", std::string_view, long long>);
static_assert(ctf::valid<"{:{}}", std::string_view, unsigned char>);
static_assert(ctf::valid<"{:{}}", std::string_view, unsigned short>);
static_assert(ctf::valid<"{:{}}", std::string_view, unsigned>);
static_assert(ctf::valid<"{:{}}", std::string_view, unsigned long>);
static_assert(ctf::valid<"{:{}}", std::string_view, unsigned long long>);
static_assert(!ctf::valid<"{:{}}", std::string_view, float>);
static_assert(!ctf::valid<"{:{}}", std::string_view, double>);
static_assert(!ctf::valid<"{:{}}", std::string_view, long double>);
static_assert(!ctf::valid<"{:{}}", std::string_view, char *>);
static_assert(!ctf::valid<"{:{}}", std::string_view, const char *>);
static_assert(!ctf::valid<"{:{}}", std::string_view, std::string>);
static_assert(!ctf::valid<"{:{}}", std::string_view, std::string_view>);
static_assert(!ctf::valid<"{:{}}", std::string_view, nullptr_t>);
static_assert(!ctf::valid<"{:{}}", std::string_view, void *>);
static_assert(!ctf::valid<"{:{}}", std::string_view, const void *>);

static_assert(ctf::valid<"{:s}", std::string_view>);
static_assert(ctf::valid<"{:?}", std::string_view>);
static_assert(!ctf::valid<"{:A}", std::string_view>);
