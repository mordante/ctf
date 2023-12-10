//===----------------------------------------------------------------------===//
//
// Part of the CTF project, under the Apache License v2.0 with LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "ctf/format.hpp"

#include <nanobench.h>

#include <chrono>
#include <fstream>

namespace {

void generate_output(const std::string &extension, char const *output_template,
                     const ankerl::nanobench::Bench &bench) {
  std::ofstream output("compile-time.render." + extension);
  ankerl::nanobench::render(output_template, bench, output);
}
} // namespace
int main() {
  ankerl::nanobench::Bench bench;
  bench.title("Compile-time");
  bench.minEpochIterations(25'000);

  bench.run("basic", [&] {
    std::string s = ctf::format<"{{hello world}}">();
    ankerl::nanobench::doNotOptimizeAway(s);
  });
  bench.run("answer", [&] {
    std::string s = ctf::format<"answer{}">(42);
    ankerl::nanobench::doNotOptimizeAway(s);
  });
  bench.run("triple", [&] {
    std::string s = ctf::format<"{:x} {:s} {:p}">(42, true, nullptr);
    ankerl::nanobench::doNotOptimizeAway(s);
  });

  bench.run("longer text", [&] {
    std::string s =
        ctf::format<"Checked out {} items for a total price of {}.">(
            42, std::numeric_limits<double>::infinity());
    ankerl::nanobench::doNotOptimizeAway(s);
  });

  {
    using namespace std::literals::chrono_literals;
    std::chrono::sys_seconds time{2'000'000'000s};

    // The interesting part about this formatter is that it also has a parsing
    // phase during formatting. At least on libc++ it does.
    bench.run("date time", [&] {
      std::string s =
          ctf::format<"The current time is {:%Y.%B.%m %H:%M:%S}">(time);
      ankerl::nanobench::doNotOptimizeAway(s);
    });
    bench.run("date time 2", [&] {
      std::string s =
          ctf::format<"{:%tThe current time is %Y.%B.%m %H:%M:%S}">(time);
      ankerl::nanobench::doNotOptimizeAway(s);
    });
  }

  generate_output("html", ankerl::nanobench::templates::htmlBoxplot(), bench);
  generate_output("json", ankerl::nanobench::templates::json(), bench);
}
