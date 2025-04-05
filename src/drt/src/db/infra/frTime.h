// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <chrono>
#include <ctime>
#include <iostream>

#include "frBaseTypes.h"

extern size_t getPeakRSS();
extern size_t getCurrentRSS();

namespace drt {
class frTime
{
 public:
  frTime() : t0_(std::chrono::high_resolution_clock::now()), t_(clock()) {}
  std::chrono::high_resolution_clock::time_point getT0() const { return t0_; }
  void print(Logger* logger);
  bool isExceed(double in)
  {
    auto t1 = std::chrono::high_resolution_clock::now();
    auto time_span
        = std::chrono::duration_cast<std::chrono::duration<double>>(t1 - t0_);
    return (time_span.count() > in);
  }

 private:
  std::chrono::high_resolution_clock::time_point t0_;
  clock_t t_;
};

std::ostream& operator<<(std::ostream& os, const frTime& t);
}  // namespace drt
