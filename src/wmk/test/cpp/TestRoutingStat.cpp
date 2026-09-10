// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <algorithm>
#include <bit>
#include <cmath>
#include <vector>

#include "gtest/gtest.h"
#include "wmk/Watermark.h"

namespace wmk {
namespace {

// Enumerate every observed subset and every sequence of null draws. Each
// outcome is equally likely conditional on the number of marked nets.
std::vector<double> nullPvalues(const std::vector<int>& q, int k, int trials)
{
  std::vector<int> sums;
  for (unsigned mask = 0; mask < (1u << q.size()); ++mask) {
    if (std::popcount(mask) != k) {
      continue;
    }
    int sum = 0;
    for (size_t i = 0; i < q.size(); ++i) {
      if (mask & (1u << i)) {
        sum += q[i];
      }
    }
    sums.push_back(sum);
  }
  size_t sequences = 1;
  for (int trial = 0; trial < trials; ++trial) {
    sequences *= sums.size();
  }
  std::vector<double> result;
  for (int observed : sums) {
    const int n_le = std::ranges::count_if(
        q, [observed](int value) { return value <= observed; });
    double log10_bound = 0.0;
    for (int i = 0; i < k; ++i) {
      log10_bound += std::log10(static_cast<double>(n_le - i) / (q.size() - i));
    }
    for (size_t sequence = 0; sequence < sequences; ++sequence) {
      int clean = 0;
      size_t draws = sequence;
      for (int trial = 0; trial < trials; ++trial) {
        clean += sums[draws % sums.size()] <= observed;
        draws /= sums.size();
      }
      RoutingStat stat;
      stat.p_r = (clean + 1.0) / (trials + 1.0);
      stat.log10_tail = log10_bound;
      result.push_back(stat.pValue());
    }
  }
  return result;
}

TEST(RoutingStat, ControlsNullRejections)
{
  for (const auto& q :
       {std::vector<int>{0, 1, 2, 3}, std::vector<int>{0, 0, 1, 1}}) {
    for (int k : {1, 2, 3}) {
      for (int trials : {1, 2, 3}) {
        SCOPED_TRACE(::testing::Message() << "k=" << k << " trials=" << trials);
        auto pvalues = nullPvalues(q, k, trials);
        std::ranges::sort(pvalues);
        // A valid p-value is super-uniform: at any alpha, no more than alpha
        // of the null outcomes may reject. Checking each rank includes ties.
        for (size_t i = 0; i < pvalues.size(); ++i) {
          EXPECT_GE(pvalues[i] + 1e-12,
                    static_cast<double>(i + 1) / pvalues.size());
          EXPECT_LE(pvalues[i], 1.0);
        }
      }
    }
  }
}

TEST(RoutingStat, KeepsTinyAnalyticalTail)
{
  RoutingStat stat;
  stat.p_r = 1.0 / 100001;
  stat.log10_tail = -100.0;
  EXPECT_NEAR(stat.pValue(), 2e-100, 1e-112);
}

TEST(RoutingStat, DefaultHasNoEvidence)
{
  EXPECT_DOUBLE_EQ(RoutingStat{}.pValue(), 1.0);
}

}  // namespace
}  // namespace wmk
