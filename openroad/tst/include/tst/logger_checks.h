// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <optional>
#include <stdexcept>  // IWYU pragma: keep (EXPECT_LOGGER_ERROR)
#include <string>
#include <string_view>

#include "gtest/gtest.h"
#include "utl/Logger.h"

namespace tst {

// Reads back the metrics a logger has recorded.
//
//   tst::MetricsCapture metrics(logger_);
//   checker.checkAntennas();
//   EXPECT_EQ(metrics.value("antenna__violating__nets"), "1");
class MetricsCapture
{
 public:
  explicit MetricsCapture(utl::Logger& logger);
  ~MetricsCapture();

  MetricsCapture(const MetricsCapture&) = delete;
  MetricsCapture& operator=(const MetricsCapture&) = delete;

  // The most recent value recorded for name, as written to a metrics file
  // (string values keep their quotes), or nullopt if none was recorded.
  std::optional<std::string> value(std::string_view name);

 private:
  utl::Logger& logger_;
  std::string path_;
};

}  // namespace tst

// Expects statement to raise the logger error with message_id, e.g.
// EXPECT_LOGGER_ERROR(checker.checkAntennas(net), "ANT-0014").
// utl::Logger::error throws a std::runtime_error whose what() is the id.
// Wrap a statement containing a top-level comma in parentheses.
#define EXPECT_LOGGER_ERROR(statement, message_id)                          \
  do {                                                                      \
    try {                                                                   \
      statement;                                                            \
      ADD_FAILURE() << "Expected logger error " << (message_id)             \
                    << " from: " #statement;                                \
    } catch (const std::runtime_error& logger_error) {                      \
      EXPECT_STREQ(logger_error.what(), message_id) << "from: " #statement; \
    } catch (...) {                                                         \
      ADD_FAILURE() << "Expected logger error " << (message_id)             \
                    << ", got another exception from: " #statement;         \
    }                                                                       \
  } while (0)
