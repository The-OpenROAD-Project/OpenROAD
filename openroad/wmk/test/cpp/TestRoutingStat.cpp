// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <algorithm>
#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

#include "RoutingStat.h"
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

TEST(RoutingStat, CountsEveryUniformDrawAsATie)
{
  for (int n : {20, 100, 1000, 5000}) {
    for (int k : {1, n / 20, n - 1}) {
      for (double q : {1.0 / 3.0, 0.9, 1e-18}) {
        SCOPED_TRACE(::testing::Message()
                     << "n=" << n << " k=" << k << " q=" << q);
        const auto stat = routingStatistics(
            std::vector<double>(k, q), std::vector<double>(n - k, q), 42, 512);
        EXPECT_EQ(stat.eligible, n);
        EXPECT_EQ(stat.marked, k);
        EXPECT_DOUBLE_EQ(stat.p_r, 1.0);
        EXPECT_DOUBLE_EQ(stat.log10_tail, 0.0);
        EXPECT_FALSE(stat.carrier_absent);
      }
    }
  }
}

TEST(RoutingStat, CountsNearlyUniformTies)
{
  for (int n : {100, 1000, 5000}) {
    const int k = n / 20;
    std::vector<double> marked(k, 1.0 / 3.0);
    std::vector<double> rest(n - k, 1.0 / 3.0);
    rest.back() = 2.0 / 3.0;
    const auto stat = routingStatistics(marked, rest, 42, 20000);
    // A draw ties iff it omits the exceptional net. This is an exact counting
    // argument, independent of the sampler's floating-point implementation.
    EXPECT_NEAR(stat.p_r, static_cast<double>(n - k) / n, 0.01);
    EXPECT_DOUBLE_EQ(stat.pValue(), 1.0);

    std::swap(marked.back(), rest.back());
    EXPECT_DOUBLE_EQ(routingStatistics(marked, rest, 42, 512).p_r, 1.0);
  }
}

TEST(RoutingStat, MatchesEnumeratedTailsWithMixedTies)
{
  // Integer units give exact tails, including distinct subsets with equal
  // totals. The production sampler receives their rounded decimal fractions.
  const std::vector<int> units{1, 1, 2, 2, 3, 3};
  std::vector<int> sums;
  for (unsigned mask = 0; mask < (1u << units.size()); ++mask) {
    if (std::popcount(mask) == 3) {
      int sum = 0;
      for (size_t i = 0; i < units.size(); ++i) {
        if (mask & (1u << i)) {
          sum += units[i];
        }
      }
      sums.push_back(sum);
    }
  }
  for (unsigned mask = 0; mask < (1u << units.size()); ++mask) {
    if (std::popcount(mask) != 3) {
      continue;
    }
    std::vector<double> marked;
    std::vector<double> rest;
    int observed = 0;
    for (size_t i = 0; i < units.size(); ++i) {
      const bool selected = mask & (1u << i);
      (selected ? marked : rest).push_back(units[i] / 10.0);
      observed += selected ? units[i] : 0;
    }
    const double exact
        = static_cast<double>(std::ranges::count_if(
              sums, [observed](int sum) { return sum <= observed; }))
          / sums.size();
    for (std::uint64_t seed : {0, 42}) {
      SCOPED_TRACE(::testing::Message() << "mask=" << mask << " seed=" << seed);
      const auto stat = routingStatistics(marked, rest, seed, 20000);
      EXPECT_NEAR(stat.p_r, exact, 0.02);
    }
  }
}

TEST(RoutingStat, CountsTiesAcrossDifferentSummationOrders)
{
  // Small terms can round down or up when added after 1.0. A draw is at least
  // as clean iff it contains at most one large term, irrespective of order.
  const double exact = 1.0 - 1001.0 * 1000.0 / (2002.0 * 2001.0);
  for (double small : {std::ldexp(1.0, -54), std::ldexp(5.0, -55)}) {
    std::vector<double> marked(1001, small);
    marked.back() = 1.0;
    const auto stat = routingStatistics(marked, marked, 42, 20000);
    EXPECT_NEAR(stat.p_r, exact, 0.02);
  }
}

TEST(RoutingStat, IndependentOfInputOrderAndScale)
{
  const std::vector<double> marked{0.1, 0.2, 0.3, 0.9};
  const std::vector<double> rest{0.9, 0.3, 0.2, 0.1, 0.1, 0.2, 0.3};
  const auto expected = routingStatistics(marked, rest, 42, 20000);
  auto reversed_marked = marked;
  auto reversed_rest = rest;
  std::ranges::reverse(reversed_marked);
  std::ranges::reverse(reversed_rest);
  const auto reordered
      = routingStatistics(reversed_marked, reversed_rest, 42, 20000);
  EXPECT_DOUBLE_EQ(reordered.p_r, expected.p_r);
  EXPECT_DOUBLE_EQ(reordered.log10_tail, expected.log10_tail);
  EXPECT_DOUBLE_EQ(reordered.q_marked, expected.q_marked);
  EXPECT_DOUBLE_EQ(reordered.q_rest, expected.q_rest);
  for (double scale : {1e-6, 1e-18}) {
    auto scaled_marked = marked;
    auto scaled_rest = rest;
    for (double& q : scaled_marked) {
      q *= scale;
    }
    for (double& q : scaled_rest) {
      q *= scale;
    }
    const auto scaled
        = routingStatistics(scaled_marked, scaled_rest, 42, 20000);
    EXPECT_DOUBLE_EQ(scaled.p_r, expected.p_r);
    EXPECT_DOUBLE_EQ(scaled.log10_tail, expected.log10_tail);
  }
}

TEST(RoutingStat, PreservesSeparatedEvidenceAtSmallScales)
{
  for (double scale : {1.0, 1e-18}) {
    const auto stat = routingStatistics(std::vector<double>(16, 0.1 * scale),
                                        std::vector<double>(240, 0.9 * scale),
                                        42,
                                        20000);
    EXPECT_DOUBLE_EQ(stat.p_r, 1.0 / 20001);
    EXPECT_LE(stat.pValue(), 1e-4);
    EXPECT_FALSE(stat.carrier_absent);
  }
}

TEST(RoutingStat, ZeroTailExcludesPositiveFractions)
{
  std::vector<double> rest(15, 0.0);
  rest.push_back(1e-18);
  const auto stat = routingStatistics({0.0, 0.0}, rest, 42, 20000);
  EXPECT_EQ(stat.zero_wrongway_nets, 17);
  EXPECT_FALSE(stat.carrier_absent);
  EXPECT_NEAR(
      std::pow(10.0, stat.log10_tail), 17.0 * 16.0 / (18.0 * 17.0), 1e-14);
}

TEST(RoutingStat, DegeneratePopulationsHaveNoEvidence)
{
  for (const auto& groups :
       {std::pair<std::vector<double>, std::vector<double>>{},
        {{0.5}, {}},
        {{}, {0.5}},
        {{0.0}, {0.0}}}) {
    const auto stat = routingStatistics(groups.first, groups.second, 42, 100);
    EXPECT_DOUBLE_EQ(stat.p_r, 1.0);
    EXPECT_DOUBLE_EQ(stat.log10_tail, 0.0);
    EXPECT_DOUBLE_EQ(stat.pValue(), 1.0);
  }
  EXPECT_TRUE(routingStatistics({0.0}, {0.0}, 42, 100).carrier_absent);
}

}  // namespace
}  // namespace wmk
