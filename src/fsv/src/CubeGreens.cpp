// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "CubeGreens.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <memory>
#include <mutex>
#include <numbers>
#include <utility>
#include <vector>

#include "AliasTable.h"
#include "BoxFace.h"
#include "CubePanels.h"
#include "Philox.h"
#include "fsv/Geometry.h"

namespace fsv {

namespace {

// Tabulates the three kernels for the face z = -1 (and x = -1 for the
// parallel-face gradient). With boundary data g on that face, the potential is
//   u = sum_mn a_mn phi_m(x) phi_n(y) sinh(k (1 - z)) / sinh(2 k),
// phi_m(x) = sin(m pi (x + 1) / 2), k = (pi / 2) sqrt(m^2 + n^2), and
// a_mn = integral of g phi_m phi_n. Evaluating u or its derivative at the
// center gives the kernels.
CubeGreens::Tables tabulate(const int n)
{
  const auto ints = panelSineIntegrals(n);
  CubeGreens::Tables t;
  t.prob.assign(n * n, 0.0);
  t.normal_face.assign(n * n, 0.0);
  t.parallel_face.assign(n * n, 0.0);
  for (int m = 1; m <= kMaxMode; m++) {
    const double sm = std::sin(m * std::numbers::pi / 2);
    for (int q = 1; q <= kMaxMode; q++) {
      const double k
          = std::numbers::pi / 2
            * std::hypot(static_cast<double>(m), static_cast<double>(q));
      const double sq = std::sin(q * std::numbers::pi / 2);
      // P and dP/dz on the face z = -1 need odd m and q.
      const double c_prob = sm * sq / (2 * std::cosh(k));
      const double c_normal = -sm * sq * k / (2 * std::sinh(k));
      // dP/dz on the face x = -1: odd m along y, even q along z.
      const double c_parallel = sm * (q * std::numbers::pi / 2)
                                * std::cos(q * std::numbers::pi / 2)
                                / (2 * std::cosh(k));
      const bool odd = (m % 2 == 1) && (q % 2 == 1);
      const bool parallel_mode = (m % 2 == 1) && (q % 2 == 0);
      if (!odd && !parallel_mode) {
        continue;
      }
      for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
          const double basis = ints[m][i] * ints[q][j];
          if (odd) {
            t.prob[i * n + j] += c_prob * basis;
            t.normal_face[i * n + j] += c_normal * basis;
          } else {
            t.parallel_face[i * n + j] += c_parallel * basis;
          }
        }
      }
    }
  }
  return t;
}

}  // namespace

CubeGreens::CubeGreens(const int panels) : CubeGreens(panels, tabulate(panels))
{
}

std::shared_ptr<const CubeGreens> CubeGreens::shared(const int panels)
{
  static std::mutex mutex;
  static std::map<int, std::weak_ptr<const CubeGreens>> cache;
  const std::lock_guard lock(mutex);
  std::shared_ptr<const CubeGreens> tables = cache[panels].lock();
  if (!tables) {
    tables = std::make_shared<const CubeGreens>(panels);
    cache[panels] = tables;
  }
  return tables;
}

CubeGreens::CubeGreens(const int panels, Tables tables)
    : n_(panels),
      prob_(std::move(tables.prob)),
      normal_face_(std::move(tables.normal_face)),
      parallel_face_(std::move(tables.parallel_face)),
      prob_alias_(prob_)
{
  // First-hop distributions: for each derivative axis d, the derivative of P
  // along d on every panel of every face.
  for (const Axis d : kAxes) {
    std::vector<double>& signs = hop_sign_[index(d)];
    std::vector<double> weights(BoxFace::kCount * n_ * n_);
    signs.resize(weights.size());
    for (int f = 0; f < BoxFace::kCount; f++) {
      const BoxFace face = BoxFace::fromIndex(f);
      const Axis t1 = tangentAxes(face.axis).second;
      for (int i = 0; i < n_; i++) {
        for (int j = 0; j < n_; j++) {
          double g;
          if (face.axis == d) {
            // On the high face the normal-face gradient changes sign.
            g = face.high ? -normalFaceGradient(i, j)
                          : normalFaceGradient(i, j);
          } else if (t1 == d) {
            g = parallelFaceGradient(i, j);
          } else {
            g = parallelFaceGradient(j, i);
          }
          const int idx = (f * n_ + i) * n_ + j;
          weights[idx] = std::abs(g);
          signs[idx] = g < 0 ? -1.0 : 1.0;
        }
      }
    }
    if (d == Axis::kX) {
      for (const double w : weights) {
        gradient_norm_ += w;
      }
    }
    hop_alias_.emplace_back(weights);
  }
}

Vec3 CubeGreens::sampleTransition(Philox& rng) const
{
  // All six faces carry probability 1/6.
  const int f = std::min(static_cast<int>(rng.uniform() * BoxFace::kCount),
                         BoxFace::kCount - 1);
  const int panel = prob_alias_.sample(rng.uniform());
  const double u = rng.uniform();
  const double v = rng.uniform();
  return cubeFacePoint(BoxFace::fromIndex(f), n_, panel / n_, panel % n_, u, v);
}

FirstHop CubeGreens::sampleFirstHop(const Axis derivative_axis,
                                    Philox& rng) const
{
  const int d = index(derivative_axis);
  const int idx = hop_alias_[d].sample(rng.uniform());
  const BoxFace face = BoxFace::fromIndex(idx / (n_ * n_));
  const int panel = idx % (n_ * n_);
  const double u = rng.uniform();
  const double v = rng.uniform();
  return {cubeFacePoint(face, n_, panel / n_, panel % n_, u, v),
          hop_sign_[d][idx]};
}

}  // namespace fsv
