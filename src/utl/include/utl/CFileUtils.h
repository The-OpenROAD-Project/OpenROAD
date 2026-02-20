// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2025, The OpenROAD Authors

#pragma once

#include <cstdint>
#include <cstdio>
#include <span>
#include <string>

#include "utl/Logger.h"

namespace utl {

// Reads all of the contents of "file" to a C++ string.
//
// NOTE: C-style `FILE*` constructs are not recommended for use in new code,
// prefer C++ iostreams for any new code.
//
// This seeks to the beginning of "file" and then attempts to read it to EOF.
//
// Side effects: The "file", by side effect, is positioned at EOF when this
// function returns successfully.
//
// Errors: If we cannot retrieve the contents of the file successfully
// logger->error is used to throw.
std::string GetContents(FILE* file, Logger* logger);

// Writes all the contents of "data" to the current position of "file".
//
// NOTE: C-style `FILE*` constructs are not recommended for use in new code,
// prefer C++ iostreams for any new code.
//
// "logger" is used to flag any errors in the writing process. Returns when all
// of "data" has been written successfully.
void WriteAll(FILE* file, std::span<const uint8_t> data, Logger* logger);

}  // namespace utl
