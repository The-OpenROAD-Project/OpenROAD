// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024-2025, The OpenROAD Authors

#pragma once

#include <cstdint>
#include <string>

namespace utl {
class UniqueName
{
 public:
  std::string GetUniqueName(const std::string& prefix)
  {
    int64_t id = counter_++;
    return prefix + std::to_string(id);
  }

 private:
  int64_t counter_ = 0;
};
}  // namespace utl
