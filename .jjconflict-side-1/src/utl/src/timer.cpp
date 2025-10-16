// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2025, The OpenROAD Authors

#include "utl/timer.h"

#include <chrono>
#include <ostream>
#include <string>

#include "utl/Logger.h"

namespace utl {

void Timer::reset()
{
  start_ = Clock::now();
}

double Timer::elapsed() const
{
  return std::chrono::duration<double>{Clock::now() - start_}.count();
}

std::ostream& operator<<(std::ostream& os, const Timer& t)
{
  auto elapsed_sec = t.elapsed();
  if (elapsed_sec < 1e-6) {
    os << elapsed_sec * 1e9 << " nsec";
  } else if (elapsed_sec < 1e-3) {
    os << elapsed_sec * 1e6 << " usec";
  } else if (elapsed_sec < 1) {
    os << elapsed_sec * 1e3 << " msec";
  } else {
    os << elapsed_sec << " sec";
  }
  return os;
}

//////////////////////////

DebugScopedTimer::DebugScopedTimer(utl::Logger* logger,
                                   ToolId tool,
                                   const char* group,
                                   int level,
                                   const std::string& msg)
    : Timer(),
      logger_(logger),
      msg_(msg),
      tool_(tool),
      group_(group),
      level_(level)
{
}

DebugScopedTimer::DebugScopedTimer(double& aggregate,
                                   utl::Logger* logger,
                                   ToolId tool,
                                   const char* group,
                                   int level,
                                   const std::string& msg)
    : Timer(),
      logger_(logger),
      msg_(msg),
      tool_(tool),
      group_(group),
      level_(level),
      aggregate_(&aggregate)
{
}

DebugScopedTimer::DebugScopedTimer(double& aggregate)
    : Timer(), logger_(nullptr), aggregate_(&aggregate)
{
}

DebugScopedTimer::~DebugScopedTimer()
{
  if (logger_) {
    debugPrint(logger_, tool_, group_, level_, msg_, *this);
  }
  if (aggregate_) {
    *aggregate_ += elapsed();
  }
}

}  // namespace utl
