// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "utl/CsvParser.h"

#include <boost/algorithm/string/trim.hpp>
#include <fstream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "utl/Logger.h"

namespace utl {

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
      boost::algorithm::trim(cell);
      cells.push_back(cell);
    }
    rows.push_back(std::move(cells));
  }
  return rows;
}

}  // namespace utl
