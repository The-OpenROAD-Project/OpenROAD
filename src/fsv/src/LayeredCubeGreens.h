// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <array>
#include <memory>
#include <vector>

#include "AliasTable.h"
#include "BoxFace.h"
#include "CubePanels.h"
#include "Philox.h"
#include "fsv/Geometry.h"

namespace fsv {

// Surface Green's function of the cube [-1, 1]^3 seen from its center when a
// planar interface z = t splits it into two dielectrics: eps_below for z < t
// and eps_above for z > t. Only their ratio matters. The interface sits at
// one of the tabulated positions t = k / kPositions, |k| < kPositions, which
// the walk reaches by sizing the cube (snapping).
//
// The tables come from separable series, exact like the homogeneous ones:
// - top and bottom faces: sine modes along x and y, with a z profile made of
//   hyperbolic functions joined at the interface so that the potential and
//   eps * dphi/dz are continuous;
// - side faces: sine modes along the face's horizontal axis, and along z the
//   eigenfunctions of -(eps Z')' = lambda eps Z with Z(+-1) = 0 and the same
//   interface conditions (a Sturm-Liouville problem whose eigenvalues are the
//   roots of a transcendental equation).
//
// Besides the transition probabilities P, the cube carries first-hop kernels
// at the center, per unit half-size:
// - along z, the flux eps * dP/dz, with eps in units of eps_below. It is
//   continuous across the interface.
// - along x and y, the plain derivative dP/dx (or dP/dy). The flux eps *
//   dP/dx jumps across the interface, so the caller multiplies by the
//   permittivity on the walker's side; this keeps the kernels continuous in
//   t down to t = 0, where they are interpolated.
class LayeredCubeGreens
{
 public:
  // Tabulated interface positions per half cube.
  static constexpr int kPositions = 8;

  // `ratio` = eps_above / eps_below. `panels` must be a multiple of
  // 2 * kPositions so that the interface lies on a panel boundary.
  LayeredCubeGreens(int panels, int position, double ratio);

  // Tables for (panels, position, ratio), built once and shared while any
  // user holds them. Thread-safe; the tables are immutable.
  static std::shared_ptr<const LayeredCubeGreens> shared(int panels,
                                                         int position,
                                                         double ratio);

  int panels() const { return n_; }
  // Height of the interface in the unit cube.
  double interfaceZ() const { return t_; }

  // Probability of panel (i, j) on the bottom face z = -1 (i along x, j
  // along y), the top face z = +1, and a side face (i along its horizontal
  // tangent axis, j along z; the same for all four side faces).
  double bottomProbability(int i, int j) const { return bottom_[i * n_ + j]; }
  double topProbability(int i, int j) const { return top_[i * n_ + j]; }
  double sideProbability(int i, int j) const { return side_[i * n_ + j]; }

  // First-hop kernel for panel (i, j) of a face, with i and j along the
  // face's tangent axes in increasing order: eps * dP/dz along z, dP/d(axis)
  // along x or y (see above).
  double flux(Axis axis, BoxFace face, int i, int j) const;
  // Sum of |flux(axis, ...)| over all panels of all faces.
  double fluxNorm(Axis axis) const { return flux_norm_[index(axis)]; }

  // Draws a landing point on the surface of [-1, 1]^3.
  Vec3 sampleTransition(Philox& rng) const;
  // Draws a landing point with density |flux(axis, ...)| / fluxNorm(axis).
  FirstHop sampleFirstHop(Axis axis, Philox& rng) const;

 private:
  void tabulateTopBottom(double ratio);
  void tabulateSides(double ratio);
  void buildSamplers();

  int n_;
  double t_;
  // Transition probabilities.
  std::vector<double> bottom_;
  std::vector<double> top_;
  std::vector<double> side_;
  // Flux along z: bottom face, top face, side faces (i horizontal, j z).
  std::vector<double> fz_bottom_;
  std::vector<double> fz_top_;
  std::vector<double> fz_side_;
  // Flux along x on: the face x = -1 (negated on x = +1), the faces y = +-1
  // (i along x), and the bottom and top faces (i along x, j along y). The
  // flux along y uses the same tables with x and y exchanged.
  std::vector<double> fx_normal_;
  std::vector<double> fx_tangential_;
  std::vector<double> fx_bottom_;
  std::vector<double> fx_top_;

  std::unique_ptr<AliasTable> alias_;  // over BoxFace::kCount * n * n panels
  std::array<std::vector<double>, 3> hop_sign_;
  std::array<std::unique_ptr<AliasTable>, 3> hop_alias_;
  std::array<double, 3> flux_norm_{};
};

}  // namespace fsv
