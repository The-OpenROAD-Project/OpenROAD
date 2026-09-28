// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors
//
// Routing watermark verification.
//
// The routing mark is not a per-object bit, so it is not checked object by
// object.  It is a population effect: the keyed nets were routed under an
// inflated cost for wiring against a layer's preferred direction, so they
// should carry less of it than everything else.
//
// The statistic is the per-net wrong-way fraction
//
//     q_R(n) = l_ww(n) / l_tot(n)
//
// measured on canonicalized geometry.  Canonical matters: a router is free to
// split one straight wire into several records, or to overlap them, and a
// naive sum would then depend on how the route happened to be written rather
// than on where the metal is.  Segments are therefore bucketed by layer,
// orientation and the line they sit on, and each bucket's union is measured
// once.  Vias have no direction and are excluded.
//
// Evidence is the difference in mean q_R between the marked set and the rest,
//
//     T_R = mean q_R over the marked nets - mean q_R over the rest.
//
// The sign of T_R is not evidence on its own: on a design carrying no
// watermark it is a coin flip, so half of all wrong keys would "pass" a sign
// test.  What matters is how unusual the value is, which is measured against
// the null of drawing the marked set uniformly at random from the eligible
// nets.  That null is sampled directly: draw many marked sets of the observed
// size and count how often they look at least this clean.  The draws come from
// a stream seeded by the design name, so the number is reproducible by anyone
// and depends on nothing secret.
//
// Sampling floors the p-value at 1/(B+1), which is nowhere near small enough
// when every marked net is entirely free of wrong-way metal -- the case a
// working watermark actually produces.  For that case the tail is also
// available in closed form: the chance that a uniformly drawn subset of the
// same size would be that clean is a ratio of binomial coefficients.  Both are
// reported. The decision doubles their minimum (capped at one), allocating
// half the false-positive budget to each test. The sampled p-value is not an
// upper bound on the exact tail, so the unadjusted minimum is not a valid test.

#include "RoutingStat.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <numeric>
#include <random>
#include <ranges>
#include <string>
#include <utility>
#include <vector>

#include "HmacSha256.h"
#include "Wirelength.h"
#include "odb/db.h"
#include "utl/Logger.h"
#include "wmk/Watermark.h"

namespace wmk {

using odb::dbBlock;
using odb::dbNet;

namespace {

// Fraction of uniformly drawn marked sets of size k whose T_R is at least as
// negative as the observed one.
//
// Only the sum of q_R over the drawn set varies between trials -- the total is
// fixed -- so each trial is a partial Fisher-Yates shuffle of k entries and a
// running sum, and the swaps are undone afterwards so the index array is
// reused rather than rebuilt.  The generator is a standard one seeded from the
// design name, so the same design gives the same p-value on any machine.
double randomizationPvalue(const std::vector<double>& q,
                           std::uint64_t seed,
                           int k,
                           double sum_limit,
                           int trials)
{
  const int e = static_cast<int>(q.size());
  const int m = e - k;
  if (k <= 0 || m <= 0 || trials <= 0) {
    return 1.0;
  }

  std::mt19937_64 rng(seed);
  std::vector<int> idx(e);
  std::iota(idx.begin(), idx.end(), 0);
  std::vector<std::pair<int, int>> swaps;
  swaps.reserve(k);

  int at_least_as_clean = 0;
  for (int b = 0; b < trials; ++b) {
    double drawn_sum = 0.0;
    swaps.clear();
    for (int i = 0; i < k; ++i) {
      const int j
          = i + static_cast<int>(rng() % static_cast<std::uint64_t>(e - i));
      std::swap(idx[i], idx[j]);
      swaps.emplace_back(i, j);
      drawn_sum += q[idx[i]];
    }
    for (const auto& [first, second] : std::ranges::reverse_view(swaps)) {
      std::swap(idx[first], idx[second]);
    }
    if (drawn_sum <= sum_limit) {
      ++at_least_as_clean;
    }
  }
  return (1.0 + at_least_as_clean) / (1.0 + trials);
}

// For a fixed population and subset size, T_R increases with the subset sum.
// Compare those sums directly, avoiding cancellation against the rest of the
// population. For nonnegative terms, an ordinary k-term sum has relative error
// at most gamma_k = k*epsilon/(1-k*epsilon). Allowing 4*k*epsilon on the
// observed sum covers rounding in BOTH sums: (1+gamma_k)/(1-gamma_k) <
// 1+4*k*epsilon for int-sized k. nextafter also covers rounding when forming
// the limit. This can only add ties, making the test conservative. Zero sums
// are exact. The allowance scales with the terms and their magnitude, not the
// number of trials; a fixed absolute epsilon is insufficient for large tied
// populations.
double sumComparisonLimit(double observed_sum, int k)
{
  if (observed_sum == 0.0) {
    return 0.0;
  }
  const double allowance = 4.0 * k * std::numeric_limits<double>::epsilon();
  return std::nextafter(observed_sum * (1.0 + allowance),
                        std::numeric_limits<double>::infinity());
}

struct RoutingPopulation
{
  std::vector<double> marked;
  std::vector<double> rest;
  int diagonal_segments = 0;
};

RoutingPopulation collectRoutingPopulation(
    dbBlock* block,
    const std::array<std::uint8_t, 32>& key,
    double fraction)
{
  // The marked set is recovered from the key alone.  Nothing recorded at embed
  // time is needed, which is what makes this stage checkable by anyone holding
  // the key and nothing else.
  const std::uint64_t threshold
      = static_cast<std::uint64_t>(std::llround(fraction * 4294967296.0));

  RoutingPopulation population;

  for (dbNet* net : block->getNets()) {
    if (!isRoutableSignalNet(net)) {
      continue;
    }
    std::int64_t l_ww = 0;
    std::int64_t l_tot = 0;
    canonicalWirelength(net, l_ww, l_tot, population.diagonal_segments);
    if (l_tot <= 0) {
      // Unrouted, or via-only: q_R is undefined, so the net is not eligible.
      continue;
    }
    const double q = static_cast<double>(l_ww) / static_cast<double>(l_tot);

    const std::string name = net->getName();
    std::vector<std::uint8_t> msg;
    msg.reserve(4 + name.size());
    msg.push_back('n');
    msg.push_back('e');
    msg.push_back('t');
    msg.push_back('\0');
    msg.insert(msg.end(), name.begin(), name.end());
    std::uint8_t mac[32];
    hmac_sha256(key.data(), key.size(), msg.data(), msg.size(), mac);
    const std::uint64_t u32 = static_cast<std::uint64_t>(mac[0])
                              | (static_cast<std::uint64_t>(mac[1]) << 8)
                              | (static_cast<std::uint64_t>(mac[2]) << 16)
                              | (static_cast<std::uint64_t>(mac[3]) << 24);
    if (u32 < threshold) {
      population.marked.push_back(q);
    } else {
      population.rest.push_back(q);
    }
  }

  return population;
}

// If a nonnegative subset sum is at most the observed sum, each of its terms
// is too. The probability of drawing only qualifying terms is therefore an
// upper bound on the tail, exact when the observed sum is zero.
double log10TailBound(const std::vector<double>& q, int k, double sum_limit)
{
  const int qualifying = std::ranges::count_if(
      q, [sum_limit](double value) { return value <= sum_limit; });
  double bound = 0.0;
  for (int i = 0; i < k; ++i) {
    bound += std::log10(static_cast<double>(qualifying - i) / (q.size() - i));
  }
  return bound;
}

}  // namespace

RoutingStat routingStatistics(std::vector<double> marked,
                              std::vector<double> rest,
                              std::uint64_t seed,
                              int trials)
{
  RoutingStat stat;
  stat.eligible = static_cast<int>(marked.size() + rest.size());
  stat.marked = static_cast<int>(marked.size());
  if (marked.empty() || rest.empty()) {
    return stat;
  }

  // Fix the accumulation order as well as the null population order. Otherwise
  // even the observed sum can change when OpenDB enumerates nets differently.
  std::ranges::sort(marked);
  std::ranges::sort(rest);
  const double sum_marked = std::accumulate(marked.begin(), marked.end(), 0.0);
  const double sum_rest = std::accumulate(rest.begin(), rest.end(), 0.0);
  stat.q_marked = sum_marked / marked.size();
  stat.q_rest = sum_rest / rest.size();
  stat.t_r = stat.q_marked - stat.q_rest;
  const double sum_limit = sumComparisonLimit(sum_marked, stat.marked);

  // The null is over net labels: only the multiset of fractions matters. Two
  // keys selecting the same number of nets face the same null draws.
  std::vector<double> all = std::move(marked);
  all.insert(all.end(), rest.begin(), rest.end());
  std::ranges::sort(all);
  stat.log10_tail = log10TailBound(all, stat.marked, sum_limit);
  stat.zero_wrongway_nets = std::ranges::count(all, 0.0);
  stat.carrier_absent = stat.zero_wrongway_nets == stat.eligible;
  if (!stat.carrier_absent) {
    stat.p_r = randomizationPvalue(all, seed, stat.marked, sum_limit, trials);
  }
  return stat;
}

RoutingStat Watermark::verifyRouting(const std::array<std::uint8_t, 32>& key,
                                     double fraction,
                                     int permutations)
{
  dbBlock* block = db_->getChip() ? db_->getChip()->getBlock() : nullptr;
  if (block == nullptr) {
    logger_->error(utl::WMK, 80, "No block loaded; read a design first.");
  }
  if (!std::isfinite(fraction) || fraction <= 0.0 || fraction > 1.0) {
    logger_->error(
        utl::WMK, 81, "fraction must be in (0, 1]; got {:.4f}.", fraction);
  }
  if (permutations < 1) {
    logger_->error(
        utl::WMK, 114, "permutations must be positive; got {}.", permutations);
  }

  RoutingPopulation population = collectRoutingPopulation(block, key, fraction);
  const std::array<std::uint8_t, 32> seed_bytes = sha256(block->getName());
  std::uint64_t seed = 0;
  for (int i = 0; i < 8; ++i) {
    seed = (seed << 8) | seed_bytes[i];
  }
  const RoutingStat stat = routingStatistics(std::move(population.marked),
                                             std::move(population.rest),
                                             seed,
                                             permutations);
  if (stat.marked == 0 || stat.marked == stat.eligible) {
    logger_->warn(utl::WMK,
                  82,
                  "Not enough routed signal nets to test: {} marked, {} "
                  "eligible.",
                  stat.marked,
                  stat.eligible);
    return stat;
  }
  if (population.diagonal_segments > 0) {
    logger_->warn(utl::WMK,
                  83,
                  "{} diagonal wire segments were ignored; wirelength assumes "
                  "Manhattan routing.",
                  population.diagonal_segments);
  }
  if (stat.carrier_absent) {
    logger_->warn(
        utl::WMK,
        86,
        "No routed signal net uses wrong-way metal, so the routing "
        "carrier does not exist in this technology and the stage "
        "cannot be tested.  Ownership must rest on the other stages.");
    return stat;
  }

  logger_->info(utl::WMK,
                84,
                "Routing watermark: {} marked of {} routed signal nets, "
                "mean q_R {:.6f} vs {:.6f}, T_R = {:.6f}.",
                stat.marked,
                stat.eligible,
                stat.q_marked,
                stat.q_rest,
                stat.t_r);
  logger_->info(utl::WMK,
                85,
                "Routing watermark: p = {:.2e} over {} draws, closed-form tail "
                "<= 1e{:.1f} ({} nets carry no wrong-way metal).",
                stat.p_r,
                permutations,
                stat.log10_tail,
                stat.zero_wrongway_nets);
  return stat;
}

}  // namespace wmk
