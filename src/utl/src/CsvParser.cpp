// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "utl/CsvParser.h"

#include <boost/algorithm/string/trim.hpp>
#include <boost/tokenizer.hpp>
#include <fstream>
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
    logger->error(utl::UTL, 204, "Unable to open {}", file_path);
    return {};
  }

  // Handles quoted fields and backslash-escaped delimiters per the
  // escaped-list grammar (e.g. `foo,"a,b",bar` → 3 cells).
  const boost::escaped_list_separator<char> sep(
      '\\', delimiter, '"');

  std::vector<std::vector<std::string>> rows;
  std::string line;
  int line_no = 0;
  while (std::getline(in, line)) {
    ++line_no;
    if (line.empty()) {
      continue;
    }
    std::vector<std::string> cells;
    try {
      boost::tokenizer<boost::escaped_list_separator<char>> tok(line, sep);
      for (std::string cell : tok) {
        boost::algorithm::trim(cell);
        cells.push_back(std::move(cell));
      }
    } catch (const boost::escaped_list_error& e) {
      logger->error(utl::UTL,
                    205,
                    "Malformed CSV at {}:{} ({})",
                    file_path,
                    line_no,
                    e.what());
    }
    rows.push_back(std::move(cells));
  }
  return rows;
}

}  // namespace utl
