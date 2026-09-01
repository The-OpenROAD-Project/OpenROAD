// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2026, The OpenROAD Authors

#include "polygon.h"

#include <algorithm>
#include <limits>
#include <utility>
#include <vector>

#include "odb/db.h"
#include "odb/dbTransform.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"
#include "odb/geom_boost.h"

namespace pdn {

using odb::geom::BoostPolygon90Set;
using odb::geom::BoostPolygon90WithHoles;

using boost::polygon::operators::operator&=;
using boost::polygon::operators::operator+=;
using boost::polygon::operators::operator-=;
using boost::polygon::operators::operator|=;

namespace {

// Sign of the cross product of the two segments meeting at b.  Positive is a
// left turn, which for a counter-clockwise contour is a convex corner.
int turn(const odb::Point& a, const odb::Point& b, const odb::Point& c)
{
  const int64_t cross = static_cast<int64_t>(b.x() - a.x()) * (c.y() - b.y())
                        - static_cast<int64_t>(b.y() - a.y()) * (c.x() - b.x());
  if (cross > 0) {
    return 1;
  }
  if (cross < 0) {
    return -1;
  }
  return 0;
}

// Twice the signed area of a closed contour.  Positive is counter-clockwise.
int64_t signedArea2(const std::vector<odb::Point>& points)
{
  int64_t area = 0;
  for (size_t i = 0; i < points.size(); i++) {
    const odb::Point& p0 = points[i];
    const odb::Point& p1 = points[(i + 1) % points.size()];
    area += static_cast<int64_t>(p0.x()) * p1.y()
            - static_cast<int64_t>(p1.x()) * p0.y();
  }
  return area;
}

// Strip the closing duplicate and every vertex that only sits in the middle of
// a straight run, then orient the contour counter-clockwise.
std::vector<odb::Point> toCornerLoop(const std::vector<odb::Point>& points)
{
  std::vector<odb::Point> loop = points;
  if (loop.size() > 1 && loop.front() == loop.back()) {
    loop.pop_back();
  }
  if (loop.size() < 3) {
    return {};
  }

  std::vector<odb::Point> corners;
  corners.reserve(loop.size());
  for (size_t i = 0; i < loop.size(); i++) {
    const odb::Point& prev = loop[(i + loop.size() - 1) % loop.size()];
    const odb::Point& curr = loop[i];
    const odb::Point& next = loop[(i + 1) % loop.size()];
    if (turn(prev, curr, next) != 0) {
      corners.push_back(curr);
    }
  }
  if (corners.size() < 4) {
    return {};
  }

  if (signedArea2(corners) < 0) {
    std::reverse(corners.begin(), corners.end());
  }
  return corners;
}

}  // namespace

Region::Region(odb::geom::BoostPolygon90Set set) : set_(std::move(set))
{
}

Region::Region(const odb::Rect& rect)
{
  if (rect.dx() > 0 && rect.dy() > 0) {
    set_ = odb::geom::toPolygonSet90(rect);
  }
}

Region::Region(const odb::Polygon& polygon)
{
  if (!polygon.getPoints().empty()) {
    set_ = odb::geom::toPolygonSet90(polygon);
  }
}

Region::Region(const std::vector<odb::Rect>& rects)
    : set_(odb::geom::toPolygonSet90(rects))
{
}

Region::Region(const std::vector<odb::Polygon>& polygons)
    : set_(odb::geom::toPolygonSet90(polygons))
{
}

bool Region::isEmpty() const
{
  return set_.empty();
}

odb::Rect Region::getEnclosingRect() const
{
  if (isEmpty()) {
    return {};
  }
  return odb::geom::getEnclosingRect(set_);
}

Region Region::bloat(const Margin& margin) const
{
  if (isEmpty()) {
    return *this;
  }

  BoostPolygon90Set set = set_;
  // boost's bloat takes unsigned distances, so a shrink has to go through
  // shrink() on the same axis.
  const auto resize = [&set](int west, int east, int south, int north) {
    set.bloat(std::max(west, 0),
              std::max(east, 0),
              std::max(south, 0),
              std::max(north, 0));
    set.shrink(std::max(-west, 0),
               std::max(-east, 0),
               std::max(-south, 0),
               std::max(-north, 0));
  };
  resize(margin.left, margin.right, margin.bottom, margin.top);

  return Region(std::move(set));
}

Region Region::fillHoles() const
{
  if (isEmpty()) {
    return *this;
  }

  std::vector<BoostPolygon90WithHoles> polygons;
  set_.get(polygons);

  BoostPolygon90Set filled;
  for (const BoostPolygon90WithHoles& polygon : polygons) {
    std::vector<odb::Point> points;
    points.reserve(polygon.size());
    for (const auto& pt : polygon) {
      points.emplace_back(pt.x(), pt.y());
    }
    filled += odb::geom::toPolygon90(odb::Polygon(points));
  }

  return Region(std::move(filled));
}

Region Region::intersect(const Region& other) const
{
  BoostPolygon90Set set = set_;
  set &= other.set_;
  return Region(std::move(set));
}

Region Region::intersect(const odb::Rect& rect) const
{
  return intersect(Region(rect));
}

Region Region::subtract(const Region& other) const
{
  BoostPolygon90Set set = set_;
  set -= other.set_;
  return Region(std::move(set));
}

Region Region::unite(const Region& other) const
{
  BoostPolygon90Set set = set_;
  set |= other.set_;
  return Region(std::move(set));
}

bool Region::contains(const Region& other) const
{
  return other.subtract(*this).isEmpty();
}

std::vector<odb::Rect> Region::getRects() const
{
  if (isEmpty()) {
    return {};
  }
  return odb::geom::extractRectangles(set_);
}

std::vector<odb::Rect> Region::getRects(Runs runs) const
{
  if (isEmpty()) {
    return {};
  }

  // boost slices along the orientation it is given, so the runs come out along
  // the other one.
  return odb::geom::extractRectangles(set_,
                                      runs == Runs::kHorizontal
                                          ? boost::polygon::VERTICAL
                                          : boost::polygon::HORIZONTAL);
}

std::vector<odb::Polygon> Region::getPolygons() const
{
  if (isEmpty()) {
    return {};
  }
  return odb::geom::extractPolygons(set_);
}

std::vector<Region::Edge> Region::getEdges() const
{
  std::vector<Edge> edges;

  for (const odb::Polygon& polygon : getPolygons()) {
    const std::vector<odb::Point> corners = toCornerLoop(polygon.getPoints());
    if (corners.empty()) {
      continue;
    }

    const size_t count = corners.size();
    for (size_t i = 0; i < count; i++) {
      const odb::Point& prev = corners[(i + count - 1) % count];
      const odb::Point& start = corners[i];
      const odb::Point& end = corners[(i + 1) % count];
      const odb::Point& after = corners[(i + 2) % count];

      // The interior is to the left of start->end, so the outward normal is
      // the direction turned clockwise.
      const int dx = end.x() - start.x();
      const int dy = end.y() - start.y();
      const odb::Point normal(dy > 0 ? 1 : (dy < 0 ? -1 : 0),
                              dx > 0 ? -1 : (dx < 0 ? 1 : 0));

      Edge edge;
      edge.start = start;
      edge.end = end;
      edge.normal = normal;
      edge.convex_at_start = turn(prev, start, end) > 0;
      edge.convex_at_end = turn(start, end, after) > 0;
      edges.push_back(edge);
    }
  }

  return edges;
}

int Region::getMarginBeyond(const odb::Rect& from,
                            const odb::Point& normal) const
{
  if (isEmpty()) {
    return 0;
  }

  const odb::Rect bounds = getEnclosingRect();
  // Far enough to leave the region on any side.
  const int reach = bounds.dx() + bounds.dy() + 1;

  // The band the face could grow into, out to reach.
  odb::Rect band;
  if (normal.y() > 0) {
    band
        = odb::Rect(from.xMin(), from.yMax(), from.xMax(), from.yMax() + reach);
  } else if (normal.y() < 0) {
    band
        = odb::Rect(from.xMin(), from.yMin() - reach, from.xMax(), from.yMin());
  } else if (normal.x() > 0) {
    band
        = odb::Rect(from.xMax(), from.yMin(), from.xMax() + reach, from.yMax());
  } else {
    band
        = odb::Rect(from.xMin() - reach, from.yMin(), from.xMin(), from.yMax());
  }
  if (band.dx() <= 0 || band.dy() <= 0) {
    return 0;
  }

  // Whatever of the band is not in the region blocks the growth, and the
  // nearest such point is where the margin ends.  The band spans the full
  // width of the face, so this is the distance the whole face can move.
  const Region blocked = Region(band).subtract(*this);
  if (blocked.isEmpty()) {
    return reach;
  }

  int margin = std::numeric_limits<int>::max();
  for (const odb::Rect& rect : blocked.getRects()) {
    int distance = 0;
    if (normal.y() > 0) {
      distance = rect.yMin() - from.yMax();
    } else if (normal.y() < 0) {
      distance = from.yMin() - rect.yMax();
    } else if (normal.x() > 0) {
      distance = rect.xMin() - from.xMax();
    } else {
      distance = from.xMin() - rect.xMax();
    }
    margin = std::min(margin, distance);
  }

  return std::max(margin, 0);
}

int Region::getMarginBeyond(const Edge& edge) const
{
  const odb::Rect face(std::min(edge.start.x(), edge.end.x()),
                       std::min(edge.start.y(), edge.end.y()),
                       std::max(edge.start.x(), edge.end.x()),
                       std::max(edge.start.y(), edge.end.y()));

  return getMarginBeyond(face, edge.normal);
}

bool Region::isOnInteriorWall(const odb::Rect& rect,
                              const odb::Point& normal) const
{
  if (isEmpty()) {
    return false;
  }

  const bool towards_high = normal.x() > 0 || normal.y() > 0;
  const bool horizontal = normal.x() != 0;
  const auto face_of = [&](const odb::Rect& r) {
    if (horizontal) {
      return towards_high ? r.xMax() : r.xMin();
    }
    return towards_high ? r.yMax() : r.yMin();
  };

  const odb::Rect bounds = getEnclosingRect();
  const int box = face_of(bounds);
  const int face = face_of(rect);

  // Strictly inside the bounding box, which is what keeps this from firing on
  // a rectangular region.
  if (towards_high ? face >= box : face <= box) {
    return false;
  }

  // Measure across the part of the shape that is actually in the region, not
  // across all of it.  A rail on the edge of the core hangs half its width
  // past the die, and measuring across that overhang finds the region missing
  // immediately -- which would make every face of it look like a wall.
  const Region touching = Region(rect).intersect(*this);
  if (touching.isEmpty()) {
    return false;
  }
  const odb::Rect clipped = touching.getEnclosingRect();
  if (face_of(clipped) != face) {
    // the region stops short of this face, so the face is not on it
    return false;
  }

  return getMarginBeyond(clipped, normal) == 0;
}

Region getInstanceOutline(odb::dbInst* inst)
{
  odb::dbMaster* master = inst->getMaster();
  const odb::dbTransform transform = inst->getTransform();

  const auto is_overlap = [](odb::dbTechLayer* layer) {
    return layer != nullptr
           && layer->getType() == odb::dbTechLayerType::OVERLAP;
  };

  std::vector<odb::Polygon> shapes;
  for (odb::dbPolygon* obstruction : master->getPolygonObstructions()) {
    if (!is_overlap(obstruction->getTechLayer())) {
      continue;
    }
    odb::Polygon polygon = obstruction->getPolygon();
    transform.apply(polygon);
    shapes.push_back(polygon);
  }
  // The decomposed halves of the polygons above are excluded, so a macro that
  // draws its outline as plain rectangles is picked up here and one that draws
  // a polygon is not counted twice.
  for (odb::dbBox* obstruction :
       master->getObstructions(/* include_decomposed_polygons */ false)) {
    if (!is_overlap(obstruction->getTechLayer())) {
      continue;
    }
    odb::Rect rect = obstruction->getBox();
    transform.apply(rect);
    shapes.emplace_back(rect);
  }

  if (shapes.empty()) {
    return Region(inst->getBBox()->getBox());
  }

  return Region(shapes);
}

}  // namespace pdn
