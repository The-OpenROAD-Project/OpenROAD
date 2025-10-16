// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2025, The OpenROAD Authors

#pragma once

#include <chrono>
#include <iostream>
#include <string>

#include "utl/Logger.h"

namespace utl {
class Logger;

// A basic timer class for measuring elapsed time.
class Timer
{
 public:
  virtual ~Timer() = default;

  double elapsed() const;  // in seconds
  void reset();

 private:
  using Clock = std::chrono::steady_clock;

  std::chrono::time_point<Clock> start_{Clock::now()};
};

class DebugScopedTimer : public Timer
{
 public:
  DebugScopedTimer(utl::Logger* logger,
                   ToolId tool,
                   const char* group,
                   int level,
                   const std::string& msg);
  DebugScopedTimer(double& aggregate,
                   utl::Logger* logger,
                   ToolId tool,
                   const char* group,
                   int level,
                   const std::string& msg);
  DebugScopedTimer(double& aggregate);
  ~DebugScopedTimer() override;

 private:
  utl::Logger* logger_;
  std::string msg_;
  ToolId tool_;
  const char* group_;
  int level_;
  double* aggregate_ = nullptr;
};

std::ostream& operator<<(std::ostream& os, const Timer& t);

}  // namespace utl
