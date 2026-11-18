// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "LayeredSquareGreens.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <map>
#include <memory>
#include <mutex>
#include <numbers>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>

#include "AliasTable.h"
#include "CubePanels.h"
#include "LayerModes.h"
#include "Philox.h"
#include "fsv/Geometry.h"

namespace fsv {

namespace {

constexpr double kPi = std::numbers::pi;

}  // namespace

LayeredSquareGreens::LayeredSquareGreens(const int panels,
                                         const int position,
                                         const double ratio)
    : n_(panels), t_(static_cast<double>(position) / kPositions)
{
  if (panels <= 0 || panels % (2 * kPositions) != 0) {
    throw std::invalid_argument(
        "fsv: layered Green's tables need panels to be a multiple of "
        + std::to_string(2 * kPositions));
  }
  if (std::abs(position) >= kPositions) {
    throw std::invalid_argument("fsv: interface position out of range");
  }
  if (!(ratio > 0) || !std::isfinite(ratio)) {
    throw std::invalid_argument("fsv: permittivity ratio must be positive");
  }
  tabulateTopBottom(ratio);
  tabulateSides(ratio);
  buildSamplers();
}

void LayeredSquareGreens::tabulateTopBottom(const double ratio)
{
  // Bottom edge data in mode m along x: the z profile of LayerModes with
  // k = m pi / 2. The top edge is the bottom edge of the square mirrored
  // z -> -z, which also flips the sign of the z flux.
  const auto ints = panelSineIntegrals(n_);
  for (auto* table :
       {&bottom_, &top_, &fz_bottom_, &fz_top_, &fx_bottom_, &fx_top_}) {
    table->assign(n_, 0.0);
  }
  for (int m = 1; m <= kMaxMode; m++) {
    const double k = m * kPi / 2;
    const ZProfile b = bottomCenter(k, t_, 1.0, ratio);
    const ZProfile mirrored = bottomCenter(k, -t_, ratio, 1.0);
    if (m % 2 == 1) {
      // Odd m: value and z flux.
      const double sm = std::sin(k);
      for (int i = 0; i < n_; i++) {
        bottom_[i] += sm * b.value * ints[m][i];
        top_[i] += sm * mirrored.value * ints[m][i];
        fz_bottom_[i] += sm * b.flux * ints[m][i];
        fz_top_[i] += -sm * mirrored.flux * ints[m][i];
      }
    } else {
      // Even m: x derivative, phi_m'(0) = k cos(k).
      const double cm = k * std::cos(k);
      for (int i = 0; i < n_; i++) {
        fx_bottom_[i] += cm * b.value * ints[m][i];
        fx_top_[i] += cm * mirrored.value * ints[m][i];
      }
    }
  }
}

void LayeredSquareGreens::tabulateSides(const double ratio)
{
  // Side edge x = -1 with data g(z): the potential is a sum over modes
  //   b_k Z_k(z) sinh(q_k (1 - x)) / sinh(2 q_k)
  // with b_k from the eps-weighted orthogonality of the Z_k. At the center
  // the x factor is 1 / (2 cosh(q)) and its x derivative -q / (2 sinh(q)).
  const double e_lo = 1.0;
  const double e_hi = ratio;
  for (auto* table : {&side_, &fz_side_, &fx_normal_}) {
    table->assign(n_, 0.0);
  }
  const int position = static_cast<int>(std::lround(t_ * kPositions));
  for (const ZMode& mode : zModes(position, e_lo, e_hi)) {
    const double q = mode.q;
    const ModeTerms terms = modeTerms(mode, t_, n_, e_lo, e_hi);
    const double center_x = 1 / (2 * std::cosh(q));
    const double p_coef = terms.value * center_x / terms.norm;
    const double fz_coef = terms.flux * center_x / terms.norm;
    const double fn_coef = terms.value * (-q / (2 * std::sinh(q))) / terms.norm;
    for (int j = 0; j < n_; j++) {
      side_[j] += p_coef * terms.panel[j];
      fz_side_[j] += fz_coef * terms.panel[j];
      fx_normal_[j] += fn_coef * terms.panel[j];
    }
  }
}

double LayeredSquareGreens::flux(const int axis,
                                 const SquareEdge edge,
                                 const int i) const
{
  if (axis == 1) {
    if (edge.axis == 0) {
      return fz_side_[i];
    }
    return edge.high ? fz_top_[i] : fz_bottom_[i];
  }
  if (edge.axis == 0) {
    return edge.high ? -fx_normal_[i] : fx_normal_[i];
  }
  return edge.high ? fx_top_[i] : fx_bottom_[i];
}

void LayeredSquareGreens::buildSamplers()
{
  std::vector<double> weights(kSquareEdges * n_);
  for (int e = 0; e < kSquareEdges; e++) {
    const SquareEdge edge = squareEdge(e);
    const std::vector<double>& table
        = edge.axis == 0 ? side_ : (edge.high ? top_ : bottom_);
    for (int i = 0; i < n_; i++) {
      weights[e * n_ + i] = std::max(0.0, table[i]);
    }
  }
  alias_ = std::make_unique<AliasTable>(weights);

  for (int d = 0; d < 2; d++) {
    hop_sign_[d].resize(weights.size());
    flux_norm_[d] = 0;
    for (int e = 0; e < kSquareEdges; e++) {
      const SquareEdge edge = squareEdge(e);
      for (int i = 0; i < n_; i++) {
        const double g = flux(d, edge, i);
        const int idx = e * n_ + i;
        weights[idx] = std::abs(g);
        hop_sign_[d][idx] = g < 0 ? -1.0 : 1.0;
        flux_norm_[d] += std::abs(g);
      }
    }
    hop_alias_[d] = std::make_unique<AliasTable>(weights);
  }
}

std::shared_ptr<const LayeredSquareGreens> LayeredSquareGreens::shared(
    const int panels,
    const int position,
    const double ratio)
{
  static std::mutex mutex;
  static std::map<std::tuple<int, int, double>,
                  std::weak_ptr<const LayeredSquareGreens>>
      cache;
  const std::lock_guard lock(mutex);
  auto& slot = cache[{panels, position, ratio}];
  std::shared_ptr<const LayeredSquareGreens> tables = slot.lock();
  if (!tables) {
    tables
        = std::make_shared<const LayeredSquareGreens>(panels, position, ratio);
    slot = tables;
  }
  return tables;
}

Vec2 LayeredSquareGreens::sampleTransition(Philox& rng) const
{
  const int idx = alias_->sample(rng.uniform());
  return squareEdgePoint(squareEdge(idx / n_), n_, idx % n_, rng.uniform());
}

FirstHop2D LayeredSquareGreens::sampleFirstHop(const int axis,
                                               Philox& rng) const
{
  const int d = axis;
  const int idx = hop_alias_[d]->sample(rng.uniform());
  return {squareEdgePoint(squareEdge(idx / n_), n_, idx % n_, rng.uniform()),
          hop_sign_[d][idx]};
}

}  // namespace fsv
