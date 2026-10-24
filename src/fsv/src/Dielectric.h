// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <array>
#include <vector>

#include "SpatialIndex.h"
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
  // Over the shells' outer boxes, by shell index.
  const SpatialIndex<D>& shellIndex() const { return shell_index_; }

 private:
  // A piece of an interface plane near a point, with the L-infinity
  // distance from the point to it. Pieces at equal distances rank by shell:
  // planar interfaces (shell -1) first, then shells by index, so that
  // results do not depend on the order in which the shell index visits
  // shells. The pieces of one shell (and the planar ones) are always visited
  // together and in a fixed order, so the first of them wins a tie.
  struct Piece
  {
    int axis;
    double coordinate;
    double distance;
    int shell = -1;

    bool before(const Piece& other) const
    {
      return distance != other.distance ? distance < other.distance
                                        : shell < other.shell;
    }
  };

  // Indices of shells, on the stack.
  struct ShellList
  {
    static constexpr int kCapacity = 64;
    std::array<int, kCapacity> index;
    int size = 0;
  };

  // Below this many shells a scan beats the index.
  static constexpr size_t kScanLimit = 16;

  // Lists the shells whose outer boxes overlap `query`. Returns false
  // instead when there are too many candidates to list; callers then scan
  // every shell, as they do when there are at most kScanLimit.
  bool listShells(const AlignedBox<D>& query, ShellList& list) const;
  // The shells within half-size r of p, in `list`: returns the list, or
  // null to scan every shell.
  const ShellList* nearShells(const Vec<D>& p, double r, ShellList& list) const;
  // The permittivity at q, from the listed shells (every shell when
  // `near` is null), which must include every shell containing q.
  double eps(const Vec<D>& q, const ShellList* near) const;
  // The loops below run over every shell (kAll, with no list) or the listed
  // ones; each is a separate instantiation, so the scan stays a plain loop.
  template <bool kAll>
  const Shell<D>* shellAt(const Vec<D>& p, const ShellList* list) const;
  template <bool kAll, typename Visit>
  void shellPieces(const Vec<D>& p,
                   double max_half_size,
                   const ShellList* list,
                   Visit visit) const;
  const Shell<D>* shellAt(const Vec<D>& p) const;
  // Calls visit(Piece) for each interface piece closer than max_half_size,
  // over the shells of nearShells(p, max_half_size, ...).
  template <typename Visit>
  void forEachPiece(const Vec<D>& p,
                    double max_half_size,
                    const ShellList* near,
                    Visit visit) const;

  double base_eps_;
  std::vector<Stack::PlanarInterface> planar_;
  std::vector<Shell<D>> shells_;
  // Over the shells' outer boxes.
  SpatialIndex<D> shell_index_;
};

}  // namespace fsv
