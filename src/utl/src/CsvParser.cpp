// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "utl/CsvParser.h"

#include <fstream>
#include <sstream>

#include "utl/Logger.h"

namespace utl {

namespace {

static std::string trim(const std::string& s)
{
  const auto start = s.find_first_not_of(" \t\r\n");
  const auto end = s.find_last_not_of(" \t\r\n");
  if (start == std::string::npos || end == std::string::npos) {
    return "";
  }
  return s.substr(start, end - start + 1);
}

}  // namespace

std::vector<std::vector<std::string>> readCsv(const std::string& file_path,
                                              Logger* logger,
                                              char delimiter)
{
  std::ifstream in(file_path);
  if (!in.is_open()) {
    logger->error(utl::UTL, 201, "Unable to open {}", file_path);
    return {};
  }

  std::vector<std::vector<std::string>> rows;
  std::string line;
  while (std::getline(in, line)) {
    if (line.empty()) {
      continue;
    }
    std::vector<std::string> cells;
    std::stringstream ss(line);
    std::string cell;
    while (std::getline(ss, cell, delimiter)) {
      cells.push_back(trim(cell));
    }
    rows.push_back(std::move(cells));
  }
  return rows;
}

}  // namespace utl
