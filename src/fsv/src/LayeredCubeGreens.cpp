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
#include "Philox.h"
#include "fsv/Geometry.h"

namespace fsv {

namespace {

constexpr double kPi = std::numbers::pi;

// Largest side-face eigenvalue root q kept. A mode's weight decays like
// exp(-q), so roots past ~130 are below double precision.
constexpr double kMaxRoot = 130.0;

// Unit data on the bottom face z = -1 in one x-y Fourier mode with
// wavenumber k. Below the interface z = t the permittivity is e_lo, above it
// e_hi; the potential and e * dZ/dz are continuous at t. With a = 1 + t and
// b = 1 - t, the z profile is
//   lower: (sinh(k (a - (z + 1))) + c sinh(k (z + 1))) / sinh(k a)
//   upper: c sinh(k (1 - z)) / sinh(k b)
// where c, its value at the interface, follows from the flux condition.
struct ZProfile
{
  double value;  // Z(0)
  double flux;   // e(0) * Z'(0), continuous across the interface
};

ZProfile bottomCenter(const double k,
                      const double t,
                      const double e_lo,
                      const double e_hi)
{
  const double a = 1 + t;
  const double b = 1 - t;
  const double c = e_lo
                   / (std::sinh(k * a)
                      * (e_lo / std::tanh(k * a) + e_hi / std::tanh(k * b)));
  if (t < 0) {
    return {c * std::sinh(k) / std::sinh(k * b),
            -e_hi * c * k * std::cosh(k) / std::sinh(k * b)};
  }
  // The center is in (or, for t = 0, on the top of) the lower layer.
  return {(std::sinh(k * t) + c * std::sinh(k)) / std::sinh(k * a),
          e_lo * k * (c * std::cosh(k) - std::cosh(k * t)) / std::sinh(k * a)};
}

// One eigenfunction of -(e Z')' = lambda e Z on [-1, 1] with Z(+-1) = 0 and
// an interface at t: Z = A sin(q (z + 1)) below, B sin(q (1 - z)) above.
struct ZMode
{
  double q;
  double lower;  // A
  double upper;  // B
};

// The modes for an interface at t = position / kPositions, in increasing q.
// Z and e Z' are continuous at t when
//   e_lo cos(q a) sin(q b) + e_hi cos(q b) sin(q a) = 0.
// Dividing this by sin(q a) sin(q b) gives
//   g(q) = e_lo cot(q a) + e_hi cot(q b),
// which falls from +inf to -inf between consecutive zeros of
// sin(q a) sin(q b) (its poles), so each such interval holds exactly one
// root. The remaining roots are the double poles, where both sines vanish
// and both conditions hold. With a = (K + k) / K and b = (K - k) / K for
// K = kPositions, the poles are q = pi K m / (K + k) and pi K n / (K - k);
// they are ordered, and the double poles found, in exact integer arithmetic.
std::vector<ZMode> zModes(const int position,
                          const double e_lo,
                          const double e_hi)
{
  constexpr int kK = LayeredCubeGreens::kPositions;
  const int ka = kK + position;
  const int kb = kK - position;
  const double a = static_cast<double>(ka) / kK;
  const double b = static_cast<double>(kb) / kK;
  auto g = [&](const double q) {
    return e_lo / std::tan(q * a) + e_hi / std::tan(q * b);
  };
  auto addMode = [&](std::vector<ZMode>& modes, const double q) {
    // Two amplitude choices satisfy both conditions at a root; take the one
    // that is not degenerate (both sines, or both cosines, vanish).
    double lower = std::sin(q * b);
    double upper = std::sin(q * a);
    const double lower2 = e_hi * std::cos(q * b);
    const double upper2 = -e_lo * std::cos(q * a);
    if (std::hypot(lower2, upper2) > std::hypot(lower, upper)) {
      lower = lower2;
      upper = upper2;
    }
    modes.push_back({q, lower, upper});
  };
  std::vector<ZMode> modes;
  // The poles in order: the next m / ka against the next n / kb.
  int m = 1;
  int n = 1;
  double left = 0;
  while (left < kMaxRoot) {
    const long lhs = static_cast<long>(m) * kb;
    const long rhs = static_cast<long>(n) * ka;
    const bool double_pole = lhs == rhs;
    const double right = lhs <= rhs ? kPi * m / a : kPi * n / b;
    if (lhs <= rhs) {
      m++;
    }
    if (lhs >= rhs) {
      n++;
    }
    // The single root of g in (left, right).
    double lo = left;
    double hi = right;
    for (int iter = 0; iter < 60; iter++) {
      const double mid = 0.5 * (lo + hi);
      if (g(mid) > 0) {
        lo = mid;
      } else {
        hi = mid;
      }
    }
    const double q = 0.5 * (lo + hi);
    if (q > kMaxRoot) {
      break;
    }
    addMode(modes, q);
    if (double_pole && right <= kMaxRoot) {
      addMode(modes, right);
    }
    left = right;
  }
  return modes;
}

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
  const double a = 1 + t_;
  const double b = 1 - t_;
  const auto ints = panelSineIntegrals(n_);
  for (auto* table : {&side_, &fz_side_, &fx_normal_, &fx_tangential_}) {
    table->assign(n_ * n_, 0.0);
  }
  const int position = static_cast<int>(std::lround(t_ * kPositions));
  for (const ZMode& mode : zModes(position, e_lo, e_hi)) {
    const double q = mode.q;
    const double norm = e_lo * mode.lower * mode.lower
                            * (a / 2 - std::sin(2 * q * a) / (4 * q))
                        + e_hi * mode.upper * mode.upper
                              * (b / 2 - std::sin(2 * q * b) / (4 * q));
    const bool center_below = t_ >= 0;
    const double value
        = center_below ? mode.lower * std::sin(q) : mode.upper * std::sin(q);
    const double z_flux = center_below ? e_lo * mode.lower * q * std::cos(q)
                                       : -e_hi * mode.upper * q * std::cos(q);
    // eps-weighted integral of Z over each z panel. The interface lies on a
    // panel boundary, so each panel is in one dielectric.
    std::vector<double> z_ints(n_);
    for (int j = 0; j < n_; j++) {
      const double z0 = -1.0 + 2.0 * j / n_;
      const double z1 = -1.0 + 2.0 * (j + 1) / n_;
      if (z1 <= t_ + 1e-12) {
        z_ints[j] = e_lo * mode.lower
                    * (std::cos(q * (z0 + 1)) - std::cos(q * (z1 + 1))) / q;
      } else {
        z_ints[j] = e_hi * mode.upper
                    * (std::cos(q * (1 - z1)) - std::cos(q * (1 - z0))) / q;
      }
    }
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
