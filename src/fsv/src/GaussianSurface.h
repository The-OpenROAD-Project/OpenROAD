// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <functional>
#include <type_traits>
#include <vector>

#include "Dielectric.h"
#include "SpatialIndex.h"
#include "fsv/Geometry.h"

namespace fsv {

// Where the surface sits around a box with conformal shells: clear of all
// shell faces (kClearOfShells), or inside the box's own innermost shell
// (kInsideShells), where a first cube meets no parallel face of it but is
// smaller.
enum class SurfacePlacement
{
  kClearOfShells,
  kInsideShells,
};

// The closed surface around the master on which walks start; its charge is
// the flux through it. It is the boundary of the union of the master's
// boxes, merged (see mergeBoxes) and each grown by its own offset, cut into
// rectangles (intervals, in 2D) on the faces of the grown boxes. In 2D its
// edges carry the charge per unit length.
//
// A walk's weight scales as 1 / h, h the half-size of its first cube, which
// shrinks where the surface nears another interface or a tight spot. Walks
// start on a piece with a probability in proportion to its area times the
// mean of 1 / h over a grid of 4 x 4 points on it (4 in 2D), so that they
// carry about equal weights, and the weight is scaled by the piece's area
// over that probability. Any positive probabilities give the same
// expectation; these lower the variance.
template <int D>
struct GaussianSurface
{
  // A rectangle of the surface on face (axis, high) of a grown box, flat
  // along `axis`. The pieces do not overlap.
  struct Piece
  {
    AlignedBox<D> rect;
    int axis;
    bool high;
    // Area (length, in 2D) over the probability of starting on the piece.
    double scale = 0;

    // The product of the tangent extents: an area, or a length in 2D.
    double area() const
    {
      double a = 1;
      for (int t = 0; t < D; t++) {
        if (t != axis) {
          a *= rect.hi[t] - rect.lo[t];
        }
      }
      return a;
    }
  };

  std::vector<Piece> pieces;
  // The running sum of the pieces' unnormalized start probabilities.
  std::vector<double> cumulative;
  // Whether kInsideShells moved any box: one had its own shells nearby.
  bool inside_shells = false;
  // The area-weighted mean of offset / h: how much the first cubes shrink
  // below the box's offset, near other interfaces (1 where none do).
  double first_cube_shrink = 1;
};

// Fewer boxes with the same union: a box inside another is dropped, and two
// boxes equal on every axis but one, overlapping or touching on it, become
// their union. A wire drawn in pieces comes back as one box, so its offset
// follows the wire's dimensions rather than a piece's length.
template <int D>
std::vector<AlignedBox<D>> mergeBoxes(std::vector<AlignedBox<D>> boxes);

// The parts of face (axis, high) of boxes[b] on the boundary of the union of
// `boxes` that b owns: not inside another box, not where another box
// continues outward from the face, and not on a coplanar face of a
// lower-index box, so that a face shared by several boxes counts once.
// `index` is over `boxes`.
template <int D>
std::vector<typename GaussianSurface<D>::Piece> exposedPieces(
    const std::vector<AlignedBox<D>>& boxes,
    const SpatialIndex<D>& index,
    int b,
    int axis,
    bool high);

// The master's Gaussian surface: its boxes merged, then each grown by its
// own offset, a fraction offset_frac of its smallest dimension that keeps
// clear of other conductors and is placed against the shells' faces by
// `placement`. Tight spots get small offsets locally instead of shrinking
// the whole surface. `conductors` is
// the scene's index over `boxes`, for the first cubes. Throws
// std::invalid_argument if the master has no boxes or another conductor
// touches it.
template <int D>
GaussianSurface<D> makeGaussianSurface(const std::vector<AlignedBox<D>>& boxes,
                                       const std::function<int(int)>& box_net,
                                       const SpatialIndex<D>& conductors,
                                       const Dielectric<D>& dielectric,
                                       int master,
                                       double offset_frac,
                                       SurfacePlacement placement
                                       = SurfacePlacement::kClearOfShells);

// The half-size of the first cube of a walk starting at p: the largest cube
// clear of conductors, shrunk to meet at most one dielectric interface.
template <int D>
double firstCubeSize(const std::type_identity_t<Vec<D>>& p,
                     const SpatialIndex<D>& conductors,
                     const Dielectric<D>& dielectric);

}  // namespace fsv
