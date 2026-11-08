// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "LayeredCubeGreens.h"

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
#include "BoxFace.h"
#include "CubePanels.h"
#include "LayerModes.h"
#include "Philox.h"
#include "fsv/Geometry.h"

namespace fsv {

namespace {

constexpr double kPi = std::numbers::pi;

}  // namespace

LayeredCubeGreens::LayeredCubeGreens(const int panels,
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

void LayeredCubeGreens::tabulateTopBottom(const double ratio)
{
  const auto ints = panelSineIntegrals(n_);
  for (auto* table :
       {&bottom_, &top_, &fz_bottom_, &fz_top_, &fx_bottom_, &fx_top_}) {
    table->assign(n_ * n_, 0.0);
  }
  for (int m = 1; m <= kMaxMode; m++) {
    const double sm = std::sin(m * kPi / 2);
    const double cm = m * kPi / 2 * std::cos(m * kPi / 2);  // d/dx at 0
    for (int q = 1; q <= kMaxMode; q += 2) {
      const double sq = std::sin(q * kPi / 2);
      const double k
          = kPi / 2
            * std::hypot(static_cast<double>(m), static_cast<double>(q));
      // The top face is the bottom face of the mirrored cube z -> -z, which
      // also flips the sign of the z flux.
      const ZProfile b = bottomCenter(k, t_, 1.0, ratio);
      const ZProfile mirrored = bottomCenter(k, -t_, ratio, 1.0);
      // Odd m: value and z flux; even m: x flux.
      const double pb = sm * sq * b.value;
      const double pt = sm * sq * mirrored.value;
      const double zb = sm * sq * b.flux;
      const double zt = -sm * sq * mirrored.flux;
      const double xb = cm * sq * b.value;
      const double xt = cm * sq * mirrored.value;
      // Each panel's basis is ints[m][i] * ints[q][j]; the coefficients are
      // folded into the i factor, and each parity fills only its tables.
      const std::vector<double>& basis_j = ints[q];
      for (int i = 0; i < n_; i++) {
        const int row = i * n_;
        if (m % 2 == 1) {
          const double bi = pb * ints[m][i];
          const double ti = pt * ints[m][i];
          const double zbi = zb * ints[m][i];
          const double zti = zt * ints[m][i];
          for (int j = 0; j < n_; j++) {
            bottom_[row + j] += bi * basis_j[j];
            top_[row + j] += ti * basis_j[j];
            fz_bottom_[row + j] += zbi * basis_j[j];
            fz_top_[row + j] += zti * basis_j[j];
          }
        } else {
          const double xbi = xb * ints[m][i];
          const double xti = xt * ints[m][i];
          for (int j = 0; j < n_; j++) {
            fx_bottom_[row + j] += xbi * basis_j[j];
            fx_top_[row + j] += xti * basis_j[j];
          }
        }
      }
    }
  }
}

void LayeredCubeGreens::tabulateSides(const double ratio)
{
  // Side face x = -1 with data g(y, z): the potential is a sum over modes
  //   b_mk sin(m pi (y + 1) / 2) Z_k(z) sinh(kappa (1 - x)) / sinh(2 kappa)
  // with kappa^2 = (m pi / 2)^2 + q_k^2 and b_mk from the eps-weighted
  // orthogonality of the Z_k. At the center the x factor is
  // 1 / (2 cosh(kappa)) and its x derivative -kappa / (2 sinh(kappa)).
  const double e_lo = 1.0;
  const double e_hi = ratio;
  const auto ints = panelSineIntegrals(n_);
  for (auto* table : {&side_, &fz_side_, &fx_normal_, &fx_tangential_}) {
    table->assign(n_ * n_, 0.0);
  }
  const int position = static_cast<int>(std::lround(t_ * kPositions));
  for (const ZMode& mode : zModes(position, e_lo, e_hi)) {
    const double q = mode.q;
    const ModeTerms terms = modeTerms(mode, t_, n_, e_lo, e_hi);
    const double norm = terms.norm;
    const double value = terms.value;
    const double z_flux = terms.flux;
    const std::vector<double>& z_ints = terms.panel;
    for (int m = 1; m <= kMaxMode; m++) {
      const double mu = m * kPi / 2;
      const double kappa = std::hypot(mu, q);
      const double center_x = 1 / (2 * std::cosh(kappa));
      // Each panel's basis is ints[m][i] * z_ints[j]. Odd m carries the
      // value, z flux and normal x flux; even m the tangential x flux.
      if (m % 2 == 1) {
        const double sm = std::sin(mu);
        const double p_coef = sm * value * center_x / norm;
        const double fz_coef = sm * z_flux * center_x / norm;
        const double fn_coef
            = sm * value * (-kappa / (2 * std::sinh(kappa))) / norm;
        for (int i = 0; i < n_; i++) {
          const int row = i * n_;
          const double vi = p_coef * ints[m][i];
          const double zi = fz_coef * ints[m][i];
          const double ni = fn_coef * ints[m][i];
          for (int j = 0; j < n_; j++) {
            side_[row + j] += vi * z_ints[j];
            fz_side_[row + j] += zi * z_ints[j];
            fx_normal_[row + j] += ni * z_ints[j];
          }
        }
      } else {
        const double ft_coef = mu * std::cos(mu) * value * center_x / norm;
        for (int i = 0; i < n_; i++) {
          const int row = i * n_;
          const double ti = ft_coef * ints[m][i];
          for (int j = 0; j < n_; j++) {
            fx_tangential_[row + j] += ti * z_ints[j];
          }
        }
      }
    }
  }
}

double LayeredCubeGreens::flux(const Axis axis,
                               const BoxFace face,
                               const int i,
                               const int j) const
{
  const int p = i * n_ + j;
  if (axis == Axis::kZ) {
    if (face.axis != Axis::kZ) {
      return fz_side_[p];
    }
    return face.high ? fz_top_[p] : fz_bottom_[p];
  }
  // Along x or y; the y tables are the x tables with x and y exchanged.
  if (face.axis == axis) {
    return face.high ? -fx_normal_[p] : fx_normal_[p];
  }
  if (face.axis != Axis::kZ) {
    return fx_tangential_[p];
  }
  const int pt = axis == Axis::kX ? p : j * n_ + i;
  return face.high ? fx_top_[pt] : fx_bottom_[pt];
}

void LayeredCubeGreens::buildSamplers()
{
  const int panels_per_face = n_ * n_;
  std::vector<double> weights(BoxFace::kCount * panels_per_face);
  for (int f = 0; f < BoxFace::kCount; f++) {
    const BoxFace face = BoxFace::fromIndex(f);
    const std::vector<double>& table
        = face.axis != Axis::kZ ? side_ : (face.high ? top_ : bottom_);
    for (int p = 0; p < panels_per_face; p++) {
      weights[f * panels_per_face + p] = std::max(0.0, table[p]);
    }
  }
  alias_ = std::make_unique<AliasTable>(weights);

  for (const Axis axis : kAxes) {
    const int d = index(axis);
    hop_sign_[d].resize(weights.size());
    flux_norm_[d] = 0;
    for (int f = 0; f < BoxFace::kCount; f++) {
      const BoxFace face = BoxFace::fromIndex(f);
      for (int i = 0; i < n_; i++) {
        for (int j = 0; j < n_; j++) {
          const double g = flux(axis, face, i, j);
          const int idx = f * panels_per_face + i * n_ + j;
          weights[idx] = std::abs(g);
          hop_sign_[d][idx] = g < 0 ? -1.0 : 1.0;
          flux_norm_[d] += std::abs(g);
        }
      }
    }
    hop_alias_[d] = std::make_unique<AliasTable>(weights);
  }
}

std::shared_ptr<const LayeredCubeGreens> LayeredCubeGreens::shared(
    const int panels,
    const int position,
    const double ratio)
{
  static std::mutex mutex;
  static std::map<std::tuple<int, int, double>,
                  std::weak_ptr<const LayeredCubeGreens>>
      cache;
  const std::lock_guard lock(mutex);
  auto& slot = cache[{panels, position, ratio}];
  std::shared_ptr<const LayeredCubeGreens> tables = slot.lock();
  if (!tables) {
    tables = std::make_shared<const LayeredCubeGreens>(panels, position, ratio);
    slot = tables;
  }
  return tables;
}

Vec3 LayeredCubeGreens::sampleTransition(Philox& rng) const
{
  const int idx = alias_->sample(rng.uniform());
  const BoxFace face = BoxFace::fromIndex(idx / (n_ * n_));
  const int panel = idx % (n_ * n_);
  const double u = rng.uniform();
  const double v = rng.uniform();
  return cubeFacePoint(face, n_, panel / n_, panel % n_, u, v);
}

FirstHop LayeredCubeGreens::sampleFirstHop(const Axis axis, Philox& rng) const
{
  const int d = index(axis);
  const int idx = hop_alias_[d]->sample(rng.uniform());
  const BoxFace face = BoxFace::fromIndex(idx / (n_ * n_));
  const int panel = idx % (n_ * n_);
  const double u = rng.uniform();
  const double v = rng.uniform();
  return {cubeFacePoint(face, n_, panel / n_, panel % n_, u, v),
          hop_sign_[d][idx]};
}

}  // namespace fsv
