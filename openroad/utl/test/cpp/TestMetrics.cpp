// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024, The OpenROAD Authors

#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>

#include "gtest/gtest.h"
#include "utl/Logger.h"

namespace utl {

TEST(Utl, WarningMetrics)
{
  // ARRANGE
  std::string metrics_filename = "test_metrics_warning_metrics.rpt";
  // The logger must be scoped to trigger the destructor which writes the
  // metrics
  {
    utl::Logger logger;
    logger.addMetricsSink(metrics_filename.c_str());

    // ACTION
    for (int i = 0; i < 10; i++) {
      logger.warn(utl::UTL, 20, "Test warning UTL-20.");
    }
    logger.warn(utl::UTL, 21, "Another test warning UTL-21.");
  }

  // ASSERT
  std::ifstream metrics_file(metrics_filename);
  ASSERT_TRUE(metrics_file.good());
  std::string content((std::istreambuf_iterator<char>(metrics_file)),
                      (std::istreambuf_iterator<char>()));

  // The output is a JSON string, which may be multi-line.
  // For simplicity, we'll just check for substrings.
  EXPECT_NE(content.find("\"flow__warnings__count\": 11"), std::string::npos);
  EXPECT_NE(content.find("\"flow__errors__count\": 0"), std::string::npos);
  EXPECT_NE(content.find("\"flow__warnings__count:UTL-0020\": 10"),
            std::string::npos);
  EXPECT_NE(content.find("\"flow__warnings__count:UTL-0021\": 1"),
            std::string::npos);
  EXPECT_NE(content.find("\"flow__warnings__type_count\": 2"),
            std::string::npos);

  // Clean up
  std::remove(metrics_filename.c_str());
}

}  // namespace utl
