// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <algorithm>
#include <cstdint>
#include <map>

namespace grt {

struct OverflowStatistics
{
  int64_t usage = 0;
  int64_t capacity = 0;
  int64_t overflow = 0;
  int64_t congested_edges = 0;
  int max_overflow = 0;

  bool operator==(const OverflowStatistics&) const = default;
};

// Only positive overflow values need histogram entries. Removing the last edge
// at the maximum immediately exposes the next maximum, including after rip-up.
class OverflowAccumulator
{
 public:
  void add(int usage, int capacity) { update(usage, capacity, 1); }
  void remove(int usage, int capacity) { update(usage, capacity, -1); }

  OverflowStatistics statistics() const
  {
    auto result = totals_;
    result.max_overflow = histogram_.empty() ? 0 : histogram_.rbegin()->first;
    return result;
  }

 private:
  void update(int usage, int capacity, int sign)
  {
    const int overflow = std::max(0, usage - capacity);
    totals_.usage += int64_t{sign} * usage;
    totals_.capacity += int64_t{sign} * capacity;
    totals_.overflow += int64_t{sign} * overflow;
    if (overflow > 0) {
      totals_.congested_edges += sign;
      if (sign > 0) {
        ++histogram_[overflow];
      } else {
        const auto it = histogram_.find(overflow);
        if (--it->second == 0) {
          histogram_.erase(it);
        }
      }
    }
  }

  OverflowStatistics totals_;
  std::map<int, int64_t> histogram_;
};

}  // namespace grt
