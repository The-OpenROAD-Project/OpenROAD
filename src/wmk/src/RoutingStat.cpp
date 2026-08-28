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
// reported and the decision uses whichever is smaller, which is sound because
// the closed form is an upper bound on the same tail.

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <numeric>
#include <random>
#include <ranges>
#include <string>
#include <utility>
#include <vector>

#include "HmacSha256.h"
#include "Wirelength.h"
#include "odb/db.h"
#include "odb/dbTypes.h"
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
                           double t_obs,
                           int trials)
{
  const int e = static_cast<int>(q.size());
  const int m = e - k;
  if (k <= 0 || m <= 0 || trials <= 0) {
    return 1.0;
  }
  const double total = std::accumulate(q.begin(), q.end(), 0.0);
  const double a = 1.0 / k + 1.0 / m;
  const double offset = total / m;

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
    if (drawn_sum * a - offset <= t_obs + 1e-15) {
      ++at_least_as_clean;
    }
  }
  return static_cast<double>(1 + at_least_as_clean) / (trials + 1);
}

}  // namespace

RoutingStat Watermark::verifyRouting(const std::array<std::uint8_t, 32>& key,
                                     double fraction,
                                     int permutations)
{
  RoutingStat stat;
  dbBlock* block = db_->getChip() ? db_->getChip()->getBlock() : nullptr;
  if (block == nullptr) {
    logger_->error(utl::WMK, 80, "No block loaded; read a design first.");
    return stat;
  }
  if (fraction <= 0.0 || fraction > 1.0) {
    logger_->error(
        utl::WMK, 81, "fraction must be in (0, 1]; got {:.4f}.", fraction);
    return stat;
  }

  // The marked set is recovered from the key alone.  Nothing recorded at embed
  // time is needed, which is what makes this stage checkable by anyone holding
  // the key and nothing else.
  const std::uint64_t threshold
      = static_cast<std::uint64_t>(std::llround(fraction * 4294967296.0));

  std::vector<double> q_marked;
  std::vector<double> q_rest;
  int diagonal_segments = 0;
  int zero_ww = 0;

  for (dbNet* net : block->getNets()) {
    if (!isRoutableSignalNet(net)) {
      continue;
    }
    std::int64_t l_ww = 0;
    std::int64_t l_tot = 0;
    canonicalWirelength(net, l_ww, l_tot, diagonal_segments);
    if (l_tot <= 0) {
      // Unrouted, or via-only: q_R is undefined, so the net is not eligible.
      continue;
    }
    const double q = static_cast<double>(l_ww) / static_cast<double>(l_tot);
    if (l_ww == 0) {
      ++zero_ww;
    }

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
      q_marked.push_back(q);
    } else {
      q_rest.push_back(q);
    }
  }

  stat.eligible = static_cast<int>(q_marked.size() + q_rest.size());
  stat.marked = static_cast<int>(q_marked.size());
  if (stat.marked == 0 || q_rest.empty()) {
    logger_->warn(utl::WMK,
                  82,
                  "Not enough routed signal nets to test: {} marked, {} "
                  "eligible.",
                  stat.marked,
                  stat.eligible);
    return stat;
  }
  if (diagonal_segments > 0) {
    logger_->warn(utl::WMK,
                  83,
                  "{} diagonal wire segments were ignored; wirelength assumes "
                  "Manhattan routing.",
                  diagonal_segments);
  }

  const double sum_marked
      = std::accumulate(q_marked.begin(), q_marked.end(), 0.0);
  const double sum_rest = std::accumulate(q_rest.begin(), q_rest.end(), 0.0);
  stat.q_marked = sum_marked / q_marked.size();
  stat.q_rest = sum_rest / q_rest.size();
  stat.t_r = stat.q_marked - stat.q_rest;

  // Closed-form tail.  Every marked net having q_R <= the observed marked total
  // implies the whole marked set is drawn from the nets at or below it, so the
  // probability a uniform draw of the same size lands there is a ratio of
  // binomial coefficients.  Computed in log10 because it underflows a double
  // long before it stops being meaningful.  The bound is only tight when the
  // marked total is near zero; otherwise nearly every net qualifies and the
  // bound collapses to 1, which is why the sampled p-value below is the one
  // that decides an ordinary case.
  const double observed_sum = sum_marked;
  int n_le = 0;
  for (double q : q_marked) {
    if (q <= observed_sum + 1e-15) {
      ++n_le;
    }
  }
  for (double q : q_rest) {
    if (q <= observed_sum + 1e-15) {
      ++n_le;
    }
  }
  const int k = stat.marked;
  if (n_le >= k && k < stat.eligible) {
    double log10p = 0.0;
    for (int i = 0; i < k; ++i) {
      log10p += std::log10(static_cast<double>(n_le - i)
                           / static_cast<double>(stat.eligible - i));
    }
    stat.log10_tail = log10p;
  }
  stat.zero_wrongway_nets = zero_ww;

  // A technology whose router never wires against the preferred direction
  // offers nothing for this statistic to measure.  Saying so is not the same
  // as saying the watermark is missing, and a user on such a platform should
  // not be left reading a failure into it.
  if (zero_ww == stat.eligible) {
    stat.carrier_absent = true;
    logger_->warn(
        utl::WMK,
        86,
        "No routed signal net uses wrong-way metal, so the routing "
        "carrier does not exist in this technology and the stage "
        "cannot be tested.  Ownership must rest on the other stages.");
    return stat;
  }

  // The null is over which nets are marked, so the same q_R values are simply
  // relabelled and only the multiset matters.  Sorting it fixes the draws to
  // depend on the design alone: two keys that select the same number of nets
  // then face the same null, and the p-value does not shift with the order the
  // groups happened to be collected in.
  std::vector<double> all;
  all.reserve(q_marked.size() + q_rest.size());
  all.insert(all.end(), q_marked.begin(), q_marked.end());
  all.insert(all.end(), q_rest.begin(), q_rest.end());
  std::ranges::sort(all);
  const std::array<std::uint8_t, 32> seed_bytes = sha256(block->getName());
  std::uint64_t seed = 0;
  for (int i = 0; i < 8; ++i) {
    seed = (seed << 8) | seed_bytes[i];
  }
  stat.p_r = randomizationPvalue(all, seed, k, stat.t_r, permutations);

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
