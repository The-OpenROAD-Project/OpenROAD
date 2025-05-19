// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024-2025, The OpenROAD Authors

#pragma once

#include <string>

namespace rmp {
class UniqueName
{
 public:
  std::string GetUniqueName(const std::string& prefix = "rmp_")
  {
    int64_t id = counter++;
    return prefix + std::to_string(id);
  }

 private:
  int64_t counter = 0;
};
}  // namespace rmp
