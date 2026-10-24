// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <array>
#include <memory>
#include <vector>

#include "AliasTable.h"
#include "CubePanels.h"
#include "Philox.h"
#include "fsv/Geometry.h"

namespace fsv {

// Surface Green's function of a homogeneous square [-1, 1]^2 in the x-z
// plane seen from its center, tabulated over N panels per edge from the
// exact Fourier series: the 2D counterpart of CubeGreens, for cross
// sections. P is the same on all four edges.
//
// The gradient tables give d/dz of P at the center (per unit half-size), for
// the first hop: one for the edges normal to the derivative axis and one
// for the edges parallel to it.
class SquareGreens
{
 public:
  explicit SquareGreens(int panels);

  // Tables for a panel count, built once and shared while any user holds
  // them. Thread-safe; the tables are immutable.
  static std::shared_ptr<const SquareGreens> shared(int panels);

  int panels() const { return n_; }

  // Probability of panel i on one edge; edges sum to 1/4 each.
  double panelProbability(int i) const { return prob_[i]; }
  // d/dz of P for panel i (along x) on the edge z = -1, which is normal to
  // z. On the edge z = +1 the value is negated.
  double normalEdgeGradient(int i) const { return normal_edge_[i]; }
  // d/dz of P for panel i (along z) on the edge x = -1, which is parallel
  // to z; the same on x = +1.
  double parallelEdgeGradient(int i) const { return parallel_edge_[i]; }

  // Draws a landing point on the boundary of the square.
  Vec2 sampleTransition(Philox& rng) const;

  // Draws a landing point with density |dP/d(derivative_axis)| /
  // gradientNorm(); the axis is 0 (x) or 1 (z).
  FirstHop2D sampleFirstHop(int derivative_axis, Philox& rng) const;
  // Sum of |dP/dz| over all panels of all edges.
  double gradientNorm() const { return gradient_norm_; }

  struct Tables
  {
    std::vector<double> prob;
    std::vector<double> normal_edge;
    std::vector<double> parallel_edge;
  };

 private:
  SquareGreens(int panels, Tables tables);

  int n_;
  std::vector<double> prob_;
  std::vector<double> normal_edge_;
  std::vector<double> parallel_edge_;
  AliasTable prob_alias_;
  // First-hop tables for x and z over kSquareEdges * N panels.
  std::array<std::vector<double>, 2> hop_sign_;
  std::vector<AliasTable> hop_alias_;
  double gradient_norm_ = 0;
};

}  // namespace fsv
