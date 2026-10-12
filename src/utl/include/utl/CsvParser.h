// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <string>
#include <vector>

namespace utl {

class Logger;

// Reads a delimiter-separated file and returns all non-empty rows.
// Each row is a vector of trimmed cell strings.
// Logs an error and returns an empty result if the file cannot be opened.
std::vector<std::vector<std::string>> readCsv(const std::string& file_path,
                                              Logger* logger,
                                              char delimiter = ',');

}  // namespace utl
