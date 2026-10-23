// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <vector>

#include "fsv/Geometry.h"
#include "fsv/Stack.h"

namespace fsv {

// One conformal dielectric around one conductor box: `outer` is the box
// grown by the summed thicknesses of this conformal and those inside it,
// and `inner` the same for the next one in (or the box itself).
//
// The boxes of one net on one layer form a group, whose shells grow from
// the union of its boxes: a point takes the conformal of the smallest depth
// whose outer box, of any box in the group, contains it. For a single box
// that is the region inside `outer` and outside `inner`.
template <int D>
struct Shell
{
  AlignedBox<D> outer;
  AlignedBox<D> inner;
  double eps;
  // Identifies the conformal dielectric. Shells of the same one may overlap
  // (they merge); outside a group, shells of different ones may not.
  int material;
  int box;    // the conductor box it wraps
  int group;  // its net and layer
  int depth;  // 1 for the innermost conformal of its box
};

// The permittivity of a scene outside its conductors: planar layers, normal
// to the last axis (z), and conformal shells, which replace the planar
// layers they cross. Instantiated for D = 3 and for cross sections, D = 2.
//
// Interfaces are the faces of the shells' outer boxes and the planar
// interfaces. A face or plane hidden inside a higher-precedence region, such
// as a planar interface inside a shell, still counts; that only makes cubes
// smaller, never wrong.
template <int D>
class Dielectric
{
 public:
  // The largest cube (square, in 2D) at a point, of at most a given
  // half-size, in which the permittivity is uniform or changes across one
  // axis-aligned plane.
  struct Cube
  {
    double half_size;
    // The plane, when the permittivity changes across it inside the cube.
    bool split = false;
    int axis = D - 1;       // the plane's normal, a component index
    double coordinate = 0;  // of the plane along `axis`
    double distance = 0;    // from the point to the plane
    double eps_minus = 0;   // below the plane along `axis`
    double eps_plus = 0;    // above it
    // The exact cube was smaller than the minimum half-size (near an edge
    // where two interfaces meet), so this one has the minimum size and
    // accounts for the nearest plane only.
    bool floored = false;
  };

  // `base_eps` applies below the first planar interface, and everywhere
  // when there are none. The interfaces are in increasing z.
  Dielectric(double base_eps,
             std::vector<Stack::PlanarInterface> planar,
             std::vector<Shell<D>> shells);

  // Relative permittivity at a point outside the conductors.
  double eps(const Vec<D>& p) const;
  // Relative permittivity of the planar layers at height z; on an
  // interface, the one above.
  double planarEps(double z) const;
  // Below min_half_size the cube is floored: walks that drift into an edge
  // would otherwise take ever smaller cubes, and stall once rounding puts
  // them on it.
  Cube cube(const Vec<D>& p,
            double max_half_size,
            double min_half_size = 0) const;

  bool uniform() const { return planar_.empty() && shells_.empty(); }
  // The permittivity below the first planar interface (everywhere, when
  // there are none) outside the shells.
  double baseEps() const { return base_eps_; }
  const std::vector<Stack::PlanarInterface>& planar() const { return planar_; }
  const std::vector<Shell<D>>& shells() const { return shells_; }

 private:
  // A piece of an interface plane near a point, with the L-infinity
  // distance from the point to it.
  struct Piece
  {
    int axis;
    double coordinate;
    double distance;
  };

  const Shell<D>* shellAt(const Vec<D>& p) const;
  // Calls visit(Piece) for each interface piece closer than max_half_size.
  template <typename Visit>
  void forEachPiece(const Vec<D>& p, double max_half_size, Visit visit) const;

  double base_eps_;
  std::vector<Stack::PlanarInterface> planar_;
  std::vector<Shell<D>> shells_;
};

}  // namespace fsv
