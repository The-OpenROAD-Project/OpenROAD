// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <string>

namespace utl {

// Call early in main() with argv[0]. Optional on Linux, where the running
// binary is found through /proc, but it lets symbolization work in a
// chroot or after setuid.
void initStackTrace(const char* argv0);

// Returns the calling thread's stack, one "N# address in function" line per
// frame, with function names read from the binary's own symbol table (no
// debug info needed). skip drops that many frames above the caller.
std::string stackTrace(int skip = 0);

}  // namespace utl
