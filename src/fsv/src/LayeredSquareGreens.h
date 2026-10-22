// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <array>
#include <memory>
#include <vector>

#include "AliasTable.h"
#include "BoxFace.h"
#include "CubePanels.h"
#include "LayerModes.h"
#include "Philox.h"
#include "fsv/Geometry.h"

namespace fsv {

// Surface Green's function of the square [-1, 1]^2 in the y-z plane seen
// from its center when an interface z = t splits it into eps_below and
// eps_above: the 2D counterpart of LayeredCubeGreens, with the same
// positions, series and kernel conventions.
//
// - Top and bottom edges: sine modes along y, with hyperbolic z profiles
//   joined at the interface.
// - Side edges: the Sturm-Liouville eigenfunctions in z of LayerModes.
//
// First-hop kernels at the center, per unit half-size: along z the flux
// eps * dP/dz with eps in units of eps_below; along y the plain derivative
// dP/dy, which the caller multiplies by the permittivity on its side.
class LayeredSquareGreens
{
 public:
  static constexpr int kPositions = kInterfacePositions;

  // `ratio` = eps_above / eps_below. `panels` must be a multiple of
  // 2 * kPositions so that the interface lies on a panel boundary.
  LayeredSquareGreens(int panels, int position, double ratio);

  // Tables for (panels, position, ratio), built once and shared while any
  // user holds them. Thread-safe; the tables are immutable.
  static std::shared_ptr<const LayeredSquareGreens> shared(int panels,
                                                           int position,
                                                           double ratio);

  int panels() const { return n_; }
  // Height of the interface in the unit square.
  double interfaceZ() const { return t_; }

  // Probability of panel i on the bottom edge z = -1 (i along y), the top
  // edge z = +1, and a side edge y = +-1 (i along z; the same for both).
  double bottomProbability(int i) const { return bottom_[i]; }
  double topProbability(int i) const { return top_[i]; }
  double sideProbability(int i) const { return side_[i]; }

  // First-hop kernel for panel i of an edge: eps * dP/dz along z, dP/dy
  // along y (see above).
  double flux(Axis axis, BoxFace edge, int i) const;
  // Sum of |flux(axis, ...)| over all panels of all edges.
  double fluxNorm(Axis axis) const { return flux_norm_[index(axis) - 1]; }

  // Draws a landing point on the boundary of the square (x = 0).
  Vec3 sampleTransition(Philox& rng) const;
  // Draws a landing point with density |flux(axis, ...)| / fluxNorm(axis).
  FirstHop sampleFirstHop(Axis axis, Philox& rng) const;

 private:
  void tabulateTopBottom(double ratio);
  void tabulateSides(double ratio);
  void buildSamplers();

  int n_;
  double t_;
  std::vector<double> bottom_;
  std::vector<double> top_;
  std::vector<double> side_;
  // Flux along z on the bottom, top and side edges.
  std::vector<double> fz_bottom_;
  std::vector<double> fz_top_;
  std::vector<double> fz_side_;
  // Flux along y on the edge y = -1 (negated on y = +1), and on the bottom
  // and top edges.
  std::vector<double> fy_normal_;
  std::vector<double> fy_bottom_;
  std::vector<double> fy_top_;

  std::unique_ptr<AliasTable> alias_;  // over kSquareEdges * n panels
  std::array<std::vector<double>, 2> hop_sign_;
  std::array<std::unique_ptr<AliasTable>, 2> hop_alias_;
  std::array<double, 2> flux_norm_{};
};

}  // namespace fsv
