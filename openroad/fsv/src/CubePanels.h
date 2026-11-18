// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <vector>

#include "BoxFace.h"
#include "fsv/Geometry.h"

namespace fsv {

// Shared by the Green's-function tables of the cube [-1, 1]^3, whose faces
// are each split into n x n equal panels, and of the square [-1, 1]^2, whose
// edges are each split into n equal panels.

// Highest Fourier mode kept along a face. Terms decay like exp(-pi * m / 2),
// so modes past ~80 are below double precision.
constexpr int kMaxMode = 81;

// Integral of sin(m * pi * (x + 1) / 2) over each of n equal panels of
// [-1, 1]; index [m][i], m = 0 .. kMaxMode.
std::vector<std::vector<double>> panelSineIntegrals(int n);

// A first-hop landing point with the sign of the flux kernel there. The
// first hop draws panels by the kernel's magnitude and carries its sign in
// the walk's weight.
template <int D>
struct BasicFirstHop
{
  Vec<D> offset;  // landing point on the surface of [-1, 1]^D
  double sign;
};
using FirstHop = BasicFirstHop<3>;
using FirstHop2D = BasicFirstHop<2>;

// Point on a face at panel (i, j), offset (u, v) in [0, 1) within it; i runs
// along the face's first tangent axis and j along the second.
Vec3 cubeFacePoint(BoxFace face, int n, int i, int j, double u, double v);

// An edge of the square [-1, 1]^2 in the x-z plane: the one normal to `axis`
// (0 for x, 1 for z) at its low or high coordinate.
struct SquareEdge
{
  int axis;
  bool high;

  double normalSign() const { return high ? 1.0 : -1.0; }
};

// Edge e of the four is axis e / 2, high e % 2.
inline constexpr int kSquareEdges = 4;
inline SquareEdge squareEdge(const int e)
{
  return {e / 2, e % 2 == 1};
}

// Point on an edge at panel i, offset u in [0, 1) within it.
Vec2 squareEdgePoint(SquareEdge edge, int n, int i, double u);

}  // namespace fsv
