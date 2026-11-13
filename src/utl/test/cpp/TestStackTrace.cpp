// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <string>

#include "gtest/gtest.h"
#include "utl/StackTrace.h"

namespace {

[[gnu::noinline]] std::string captureStackTrace()
{
  return utl::stackTrace();
}

TEST(StackTrace, CapturesCaller)
{
  const std::string trace = captureStackTrace();
  EXPECT_FALSE(trace.empty());
  EXPECT_NE(trace.find("0#"), std::string::npos);
  EXPECT_NE(trace.find(" in "), std::string::npos);
}

}  // namespace
