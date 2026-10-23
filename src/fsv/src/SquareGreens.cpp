// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "SquareGreens.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <memory>
#include <mutex>
#include <numbers>
#include <utility>
#include <vector>

#include "AliasTable.h"
#include "CubePanels.h"
#include "Philox.h"
#include "fsv/Geometry.h"

namespace fsv {

namespace {

constexpr double kPi = std::numbers::pi;

// The tables for n panels per edge. With boundary data g on the edge
// z = -1, the potential is
//   u = sum_m a_m phi_m(x) sinh(k (1 - z)) / sinh(2 k),
// phi_m(x) = sin(m pi (x + 1) / 2), k = m pi / 2 and a_m = integral of
// g phi_m. At the center the z factor is 1 / (2 cosh(k)) and its z
// derivative -k / (2 sinh(k)). Data on the edge x = -1 gives d/dz through
// phi_q'(0) instead.
SquareGreens::Tables tabulate(const int n)
{
  const auto ints = panelSineIntegrals(n);
  SquareGreens::Tables t;
  t.prob.assign(n, 0.0);
  t.normal_edge.assign(n, 0.0);
  t.parallel_edge.assign(n, 0.0);
  for (int m = 1; m <= kMaxMode; m++) {
    const double k = m * kPi / 2;
    if (m % 2 == 1) {
      // P and the normal gradient need odd m.
      const double sm = std::sin(k);
      const double c_prob = sm / (2 * std::cosh(k));
      const double c_normal = -sm * k / (2 * std::sinh(k));
      for (int i = 0; i < n; i++) {
        t.prob[i] += c_prob * ints[m][i];
        t.normal_edge[i] += c_normal * ints[m][i];
      }
    } else {
      // The parallel gradient needs even m.
      const double c_parallel = k * std::cos(k) / (2 * std::cosh(k));
      for (int i = 0; i < n; i++) {
        t.parallel_edge[i] += c_parallel * ints[m][i];
      }
    }
  }
  return t;
}

}  // namespace

SquareGreens::SquareGreens(const int panels)
    : SquareGreens(panels, tabulate(panels))
{
}

SquareGreens::SquareGreens(const int panels, Tables tables)
    : n_(panels),
      prob_(std::move(tables.prob)),
      normal_edge_(std::move(tables.normal_edge)),
      parallel_edge_(std::move(tables.parallel_edge)),
      prob_alias_(prob_)
{
  // First-hop distributions: for each derivative axis d (x, z), the
  // derivative of P along d on every panel of every edge.
  for (int d = 0; d < 2; d++) {
    std::vector<double>& signs = hop_sign_[d];
    std::vector<double> weights(kSquareEdges * n_);
    signs.resize(weights.size());
    for (int e = 0; e < kSquareEdges; e++) {
      const SquareEdge edge = squareEdge(e);
      for (int i = 0; i < n_; i++) {
        double g;
        if (edge.axis == d) {
          // On the high edge the normal-edge gradient changes sign.
          g = edge.high ? -normalEdgeGradient(i) : normalEdgeGradient(i);
        } else {
          g = parallelEdgeGradient(i);
        }
        const int idx = e * n_ + i;
        weights[idx] = std::abs(g);
        signs[idx] = g < 0 ? -1.0 : 1.0;
      }
    }
    if (d == 1) {
      for (const double w : weights) {
        gradient_norm_ += w;
      }
    }
    hop_alias_.emplace_back(weights);
  }
}

std::shared_ptr<const SquareGreens> SquareGreens::shared(const int panels)
{
  static std::mutex mutex;
  static std::map<int, std::weak_ptr<const SquareGreens>> cache;
  const std::lock_guard lock(mutex);
  std::shared_ptr<const SquareGreens> tables = cache[panels].lock();
  if (!tables) {
    tables = std::make_shared<const SquareGreens>(panels);
    cache[panels] = tables;
  }
  return tables;
}

Vec2 SquareGreens::sampleTransition(Philox& rng) const
{
  // All four edges carry probability 1/4.
  const int e = std::min(static_cast<int>(rng.uniform() * kSquareEdges),
                         kSquareEdges - 1);
  const int panel = prob_alias_.sample(rng.uniform());
  return squareEdgePoint(squareEdge(e), n_, panel, rng.uniform());
}

FirstHop2D SquareGreens::sampleFirstHop(const int derivative_axis,
                                        Philox& rng) const
{
  const int d = derivative_axis;
  const int idx = hop_alias_[d].sample(rng.uniform());
  return {squareEdgePoint(squareEdge(idx / n_), n_, idx % n_, rng.uniform()),
          hop_sign_[d][idx]};
}

}  // namespace fsv
