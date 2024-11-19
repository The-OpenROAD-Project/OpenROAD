// Copyright 2024 Google LLC
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

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
