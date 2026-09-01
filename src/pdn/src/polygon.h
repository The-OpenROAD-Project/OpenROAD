// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2026, The OpenROAD Authors

#pragma once

#include <vector>

#include "odb/db.h"
#include "odb/geom.h"
#include "odb/geom_boost.h"

namespace pdn {

// A rectilinear region.
//
// PDN reasons about three areas -- the domain, the grid and the ring outline --
// which are rectangles on a rectangular floorplan and rectilinear polygons on a
// polygon one.  Region is the common representation, so the geometry code does
// not have to branch on which it has: a Region built from a Rect behaves the
// same as that Rect.
//
// It holds a boost polygon-90 set, so it can also carry a disjoint or holed
// region, which is what a merge of rows around a macro produces.
class Region
{
 public:
  // How much to grow (or, negative, shrink) each side of a region.  This
  // mirrors pdn::EdgeSpec, which lives in grid.h and cannot be included here
  // without a cycle.
  struct Margin
  {
    int left{0};
    int bottom{0};
    int right{0};
    int top{0};
  };

  // A boundary edge, walked so that the interior lies to its left.  Collinear
  // vertices are merged away, so an edge always runs corner to corner:
  // dbBlock::computeCoreArea leaves a vertex wherever two rows met, which on a
  // 68 row core is 133 vertices describing 6 corners.
  struct Edge
  {
    odb::Point start;
    odb::Point end;
    // Outward normal, one of (+-1, 0) or (0, +-1).
    odb::Point normal;
    // Whether the corner at each end turns outward (interior angle 90) rather
    // than inward (270).  A ring side is extended past a convex corner to meet
    // its neighbour; at a concave corner the two already overlap.
    bool convex_at_start{true};
    bool convex_at_end{true};

    bool isHorizontal() const { return start.y() == end.y(); }
  };

  Region() = default;
  explicit Region(const odb::Rect& rect);
  explicit Region(const odb::Polygon& polygon);
  explicit Region(const std::vector<odb::Rect>& rects);
  explicit Region(const std::vector<odb::Polygon>& polygons);

  bool isEmpty() const;
  odb::Rect getEnclosingRect() const;

  // Grow each side independently.  Negative values shrink.
  Region bloat(const Margin& margin) const;

  // Drop interior holes, keeping the outer boundaries.  Rows cut around a
  // macro merge into a holed region; the hole is where the macro is, not a
  // place the core stops.
  Region fillHoles() const;

  Region intersect(const Region& other) const;
  Region intersect(const odb::Rect& rect) const;
  Region subtract(const Region& other) const;
  Region unite(const Region& other) const;

  // True when every part of other lies inside this region.
  bool contains(const Region& other) const;

  // Which way the rectangles of a decomposition are made to run.
  enum class Runs
  {
    kHorizontal,
    kVertical
  };

  std::vector<odb::Rect> getRects() const;
  // Decomposed so that each rectangle is a maximal run in the given
  // direction, which for a strap or a ring side is the direction it travels.
  std::vector<odb::Rect> getRects(Runs runs) const;
  std::vector<odb::Polygon> getPolygons() const;
  std::vector<Edge> getEdges() const;

  // How far the region reaches outward from a face, measured along the whole
  // width of that face.  Returns 0 when the region does not extend past it at
  // all, and the distance to the first point where the band leaves the region
  // otherwise.  This answers both "how much room is there between the core and
  // the die on this side" and "where should a shape extended to the boundary
  // stop".
  //
  // The face is the side of `from` that `normal` points out of; for the Edge
  // overload it is the edge itself.
  int getMarginBeyond(const odb::Rect& from, const odb::Point& normal) const;
  int getMarginBeyond(const Edge& edge) const;

  // True when the named face of `rect` sits on a wall of the region that lies
  // strictly inside its bounding box -- the wall of a notch, which is as much
  // an edge of a polygon die as the sides of its bounding box are.
  //
  // Faces on the bounding box itself are deliberately excluded, so a caller
  // that already compares against the bounding box can add this as a second
  // condition and be sure of changing nothing on a rectangular region: there,
  // no face can be both strictly inside the box and on the boundary.
  bool isOnInteriorWall(const odb::Rect& rect, const odb::Point& normal) const;

 private:
  explicit Region(odb::geom::BoostPolygon90Set set);

  odb::geom::BoostPolygon90Set set_;
};

// The real outline of a placed instance, in placed coordinates.
//
// LEF has no polygonal MACRO SIZE, so a non-rectangular macro carries its
// occupied area as an OVERLAP-layer obstruction, which is what LEF/DEF defines
// that layer for.  A master that declares none -- which is every macro in a
// purely rectangular library -- falls back to its placement bounding box, so
// this is the bounding box wherever the bounding box was already right.
Region getInstanceOutline(odb::dbInst* inst);

}  // namespace pdn
