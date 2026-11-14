// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "utl/StackTrace.h"

#include <array>
#include <string>

#include "absl/debugging/stacktrace.h"
#include "absl/debugging/symbolize.h"
#include "fmt/format.h"

namespace utl {

void initStackTrace(const char* argv0)
{
  absl::InitializeSymbolizer(argv0);
}

// noinline so the frame skipped below is always this function's own.
[[gnu::noinline]] std::string stackTrace(const int skip)
{
  constexpr int kMaxFrames = 64;
  std::array<void*, kMaxFrames> frames{};
  const int depth = absl::GetStackTrace(frames.data(), kMaxFrames, skip + 1);
  std::string trace;
  for (int i = 0; i < depth; ++i) {
    std::array<char, 1024> symbol{};
    const bool found = absl::Symbolize(frames[i], symbol.data(), symbol.size());
    trace += fmt::format(
        "{:2}# {} in {}\n", i, frames[i], found ? symbol.data() : "??");
  }
  return trace;
}

}  // namespace utl
