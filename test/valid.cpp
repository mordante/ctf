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
