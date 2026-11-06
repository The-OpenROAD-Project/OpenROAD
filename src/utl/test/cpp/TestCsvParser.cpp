// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <filesystem>
#include <fstream>
#include <ios>
#include <string>
#include <vector>

#include "gtest/gtest.h"
#include "utl/CsvParser.h"
#include "utl/Logger.h"

namespace utl {

namespace {

using Rows = std::vector<std::vector<std::string>>;

std::string writeFile(const std::string& name, const std::string& contents)
{
  const std::filesystem::path path
      = std::filesystem::path(testing::TempDir()) / name;
  std::ofstream(path, std::ios::binary) << contents;
  return path.string();
}

}  // namespace

TEST(CsvParser, ReadsRowsAndTrimsCells)
{
  Logger logger;
  const std::string path
      = writeFile("csv_basic.csv", "chip, heat\n0,0, 1 ,1,\"2,5\"\n");
  const Rows expected{{"chip", "heat"}, {"0", "0", "1", "1", "2,5"}};
  EXPECT_EQ(readCsv(path, &logger), expected);
}

TEST(CsvParser, SkipsBlankLinesWithCrlfEndings)
{
  Logger logger;
  const std::string path
      = writeFile("csv_crlf.csv", "chip,heat\r\n\r\n0,0,1,1,5\r\n\r\n");
  const Rows expected{{"chip", "heat"}, {"0", "0", "1", "1", "5"}};
  EXPECT_EQ(readCsv(path, &logger), expected);
}

TEST(CsvParser, SkipsWhitespaceOnlyLines)
{
  Logger logger;
  const std::string path
      = writeFile("csv_spaces.csv", "chip,heat\n  \t\n0,0,1,1,5\n   \n");
  const Rows expected{{"chip", "heat"}, {"0", "0", "1", "1", "5"}};
  EXPECT_EQ(readCsv(path, &logger), expected);
}

}  // namespace utl
