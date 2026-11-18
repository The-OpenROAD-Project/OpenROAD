// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "LayerModes.h"

#include <cmath>
#include <numbers>
#include <vector>

namespace fsv {

namespace {

constexpr double kPi = std::numbers::pi;

// Largest side-face eigenvalue root q kept. A mode's weight decays like
// exp(-q), so roots past ~130 are below double precision.
constexpr double kMaxRoot = 130.0;

}  // namespace

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
  constexpr int kK = kInterfacePositions;
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

ModeTerms modeTerms(const ZMode& mode,
                    const double t,
                    const int n,
                    const double e_lo,
                    const double e_hi)
{
  const double q = mode.q;
  const double a = 1 + t;
  const double b = 1 - t;
  ModeTerms terms;
  terms.norm
      = e_lo * mode.lower * mode.lower * (a / 2 - std::sin(2 * q * a) / (4 * q))
        + e_hi * mode.upper * mode.upper
              * (b / 2 - std::sin(2 * q * b) / (4 * q));
  const bool center_below = t >= 0;
  terms.value
      = center_below ? mode.lower * std::sin(q) : mode.upper * std::sin(q);
  terms.flux = center_below ? e_lo * mode.lower * q * std::cos(q)
                            : -e_hi * mode.upper * q * std::cos(q);
  terms.panel.resize(n);
  for (int j = 0; j < n; j++) {
    const double z0 = -1.0 + 2.0 * j / n;
    const double z1 = -1.0 + 2.0 * (j + 1) / n;
    if (z1 <= t + 1e-12) {
      terms.panel[j] = e_lo * mode.lower
                       * (std::cos(q * (z0 + 1)) - std::cos(q * (z1 + 1))) / q;
    } else {
      terms.panel[j] = e_hi * mode.upper
                       * (std::cos(q * (1 - z1)) - std::cos(q * (1 - z0))) / q;
    }
  }
  return terms;
}

}  // namespace fsv
