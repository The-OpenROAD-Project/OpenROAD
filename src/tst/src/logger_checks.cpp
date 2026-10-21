// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "tst/logger_checks.h"

#include <atomic>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <optional>
#include <string>
#include <string_view>

#include "gtest/gtest.h"
#include "utl/Logger.h"

namespace tst {

MetricsCapture::MetricsCapture(utl::Logger& logger) : logger_(logger)
{
  static std::atomic<int> next_id{0};
  path_ = ::testing::TempDir() + "tst_metrics_" + std::to_string(next_id++)
          + ".json";
  logger_.addMetricsSink(path_.c_str());
}

MetricsCapture::~MetricsCapture()
{
  logger_.removeMetricsSink(path_.c_str());
  std::remove(path_.c_str());
}

std::optional<std::string> MetricsCapture::value(std::string_view name)
{
  // Removing the sink flushes the recorded metrics to it.
  logger_.removeMetricsSink(path_.c_str());
  logger_.addMetricsSink(path_.c_str());

  std::ifstream file(path_);
  const std::string json((std::istreambuf_iterator<char>(file)),
                         std::istreambuf_iterator<char>());

  // Entries are written one per line as: "name": value
  const std::string key = "\"" + std::string(name) + "\": ";
  std::optional<std::string> result;
  for (size_t pos = json.find(key); pos != std::string::npos;
       pos = json.find(key, pos + key.size())) {
    const size_t start = pos + key.size();
    const size_t end = json.find_first_of(",\n", start);
    result = json.substr(start, end - start);
  }
  return result;
}

}  // namespace tst
