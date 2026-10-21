// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <array>
#include <memory>
#include <vector>

#include "AliasTable.h"
#include "BoxFace.h"
#include "Philox.h"
#include "fsv/Geometry.h"

namespace fsv {

// Surface Green's function of a homogeneous cube [-1, 1]^3 seen from its
// center, tabulated over N x N panels per face from the exact Fourier series.
//
// P(s) is the probability that a walk from the center first reaches the
// surface at s; the potential at the center is the P-weighted average of the
// surface potential. P is the same on all six faces and does not depend on
// the dielectric constant or on the cube size.
//
// The gradient tables give d/dz of P at the center (per unit half-size), used
// for the first hop from the Gaussian surface, where the charge is the flux
// of the field. By symmetry two tables cover every face: one for the faces
// normal to the derivative axis and one for the faces parallel to it.
class CubeGreens
{
 public:
  explicit CubeGreens(int panels);

  // Tables for a panel count, built once and shared while any user holds
  // them. Thread-safe; the tables are immutable.
  static std::shared_ptr<const CubeGreens> shared(int panels);

  int panels() const { return n_; }

  // Probability of panel (i, j) on one face; faces sum to 1/6 each.
  double panelProbability(int i, int j) const { return prob_[i * n_ + j]; }
  // d/dz of P for panel (i, j) on the face z = -1 (i along x, j along y),
  // which is normal to z. On the face z = +1 the value is negated.
  double normalFaceGradient(int i, int j) const
  {
    return normal_face_[i * n_ + j];
  }
  // d/dz of P for panel (i, j) on the face x = -1 (i along y, j along z),
  // which is parallel to z. The same table applies to the other three faces
  // parallel to z.
  double parallelFaceGradient(int i, int j) const
  {
    return parallel_face_[i * n_ + j];
  }

  // Draws a landing point on the surface of [-1, 1]^3.
  Vec3 sampleTransition(Philox& rng) const;

  struct FirstHop
  {
    Vec3 offset;  // landing point on the surface of [-1, 1]^3
    double sign;  // sign of dP/d(derivative_axis) at that point
  };
  // Draws a landing point with density |dP/d(derivative_axis)| /
  // gradientNorm().
  FirstHop sampleFirstHop(Axis derivative_axis, Philox& rng) const;
  // Sum of |dP/dz| over all panels of all faces.
  double gradientNorm() const { return gradient_norm_; }

  struct Tables
  {
    std::vector<double> prob;
    std::vector<double> normal_face;
    std::vector<double> parallel_face;
  };

 private:
  CubeGreens(int panels, Tables tables);

  // Point on a face of [-1, 1]^3 at panel (i, j), offset (u, v) within it;
  // i runs along the face's first tangent axis and j along the second.
  Vec3 facePoint(BoxFace face, int i, int j, double u, double v) const;

  int n_;
  std::vector<double> prob_;
  std::vector<double> normal_face_;
  std::vector<double> parallel_face_;
  AliasTable prob_alias_;
  // First-hop tables per derivative axis over BoxFace::kCount * N * N panels.
  std::array<std::vector<double>, 3> hop_sign_;
  std::vector<AliasTable> hop_alias_;
  double gradient_norm_ = 0;
};

}  // namespace fsv
