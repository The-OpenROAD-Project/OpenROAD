// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024-2025, The OpenROAD Authors

#include "RDLRoute.h"

#include <algorithm>
#include <array>
#include <map>
#include <memory>
#include <set>
#include <tuple>
#include <vector>

#include "boost/geometry/geometry.hpp"
#include "boost/polygon/polygon_90_set_data.hpp"
#include "boost/polygon/polygon_90_with_holes_data.hpp"
#include "boost/polygon/rectangle_concept.hpp"
#include "boost/polygon/rectangle_data.hpp"
#include "odb/db.h"
#include "odb/geom.h"
#include "odb/geom_boost.h"

namespace pad {

RDLRoute::RDLRoute(odb::dbITerm* source,
                   const std::vector<odb::dbITerm*>& dests)
    : iterm_(source),
      priority_(0),
      route_pending_(false),
      locked_(false),
      routed_(false),
      terminals_(dests)
{
  terminals_.erase(std::remove_if(terminals_.begin(),
                                  terminals_.end(),
                                  [this](odb::dbITerm* other) {
                                    return iterm_->getInst()
                                           == other->getInst();
                                  }),
                   terminals_.end());

  const odb::Point iterm_center = iterm_->getBBox().center();
  std::stable_sort(terminals_.begin(),
                   terminals_.end(),
                   [&iterm_center](odb::dbITerm* lhs, odb::dbITerm* rhs) {
                     const bool lhs_cover = RDLRouter::isCoverTerm(lhs);
                     const bool rhs_cover = RDLRouter::isCoverTerm(rhs);

                     const auto lhs_dist = odb::Point::squaredDistance(
                         iterm_center, lhs->getBBox().center());
                     const auto rhs_dist = odb::Point::squaredDistance(
                         iterm_center, rhs->getBBox().center());
                     // sort non-cover terms first
                     return std::tie(lhs_cover, lhs_dist)
                            < std::tie(rhs_cover, rhs_dist);
                   });

  resetRoute();
}

odb::dbITerm* RDLRoute::peakNextTerminal() const
{
  return *next_;
}

odb::dbITerm* RDLRoute::getNextTerminal()
{
  odb::dbITerm* iterm = peakNextTerminal();

  next_++;

  return iterm;
}

bool RDLRoute::compare(const std::shared_ptr<RDLRoute>& other) const
{
  const auto lhs_priority = -getPriority();
  const auto rhs_priority = -other->getPriority();
  if (!hasNextTerminal() || !other->hasNextTerminal()
      || lhs_priority != rhs_priority) {
    return lhs_priority > rhs_priority;
  }

  const auto& lhs_shortest = peakNextTerminal();
  const auto lhs_dist = odb::Point::squaredDistance(
      getTerminal()->getBBox().center(), lhs_shortest->getBBox().center());

  const auto& rhs_shortest = other->peakNextTerminal();
  const auto rhs_dist
      = odb::Point::squaredDistance(other->getTerminal()->getBBox().center(),
                                    rhs_shortest->getBBox().center());

  if (lhs_dist == rhs_dist) {
    // if distances are equal, use id for stable sorting
    return compare_by_id(getTerminal(), other->getTerminal());
  }

  return lhs_dist > rhs_dist;
}

void RDLRoute::setRoute(
    const std::map<GridGraphVertex, odb::Point>& vertex_point_map,
    const std::vector<GridGraphVertex>& vertex,
    const std::vector<RDLRouter::GridEdge>& removed_edges,
    const RouteTarget* source,
    const RouteTarget* target,
    const RDLRouter::TerminalAccess& access_source,
    const RDLRouter::TerminalAccess& access_dest)
{
  route_vertex_ = vertex;
  for (const auto& vertex : route_vertex_) {
    route_pts_.push_back(vertex_point_map.at(vertex));
  }
  route_edges_ = removed_edges;
  route_source_ = source;
  route_dest_ = target;
  access_source_ = access_source;
  access_dest_ = access_dest;

  // Forward removed edges from access
  for (const auto& edge : access_source_.removed_edges) {
    if (contains(edge.source) || contains(edge.target)) {
      route_edges_.push_back(edge);
    }
  }
  for (const auto& edge : access_dest_.removed_edges) {
    if (contains(edge.source) || contains(edge.target)) {
      route_edges_.push_back(edge);
    }
  }

  routed_ = true;
}

void RDLRoute::resetRoute()
{
  if (locked_) {
    return;
  }

  routed_ = false;

  route_vertex_.clear();
  route_pts_.clear();
  route_edges_.clear();
  route_source_ = nullptr;
  route_dest_ = nullptr;
  route_pending_ = true;
  access_source_ = {};
  access_dest_ = {};

  next_ = terminals_.begin();
}

bool RDLRoute::isIntersecting(RDLRoute* other, int extent) const
{
  // check current next_
  // if at end of next, select begin + priority offset (modulo)
  // this ensures the ripup does something different (by checking
  // different destinations, where possible) if a route fails to
  // route more than one time.
  const int dst_idx = priority_ % terminals_.size();
  odb::dbITerm* dst = terminals_[dst_idx];

  // create line with width (extent * (priority + 1))
  const odb::Point pt0 = iterm_->getBBox().center();
  const odb::Point pt1 = dst->getBBox().center();
  const std::vector<odb::Point> line_segment = {pt0, pt1};

  const int margin = (priority_ + 1) * extent;

  // check intersection with routed other
  for (const auto& pt : other->getRoutePoints()) {
    const odb::Rect rect(
        pt.x() - margin, pt.y() - margin, pt.x() + margin, pt.y() + margin);

    if (boost::geometry::intersects(line_segment, rect)) {
      return true;
    }
  }

  return false;
}

bool RDLRoute::isIntersecting(const odb::Line& line, int extent) const
{
  if (!isRouted()) {
    return false;
  }

  const std::vector<odb::Point> line_segment = line.getPoints();

  for (const auto& pt : route_pts_) {
    const odb::Rect rect(
        pt.x() - extent, pt.y() - extent, pt.x() + extent, pt.y() + extent);

    if (boost::geometry::intersects(line_segment, rect)) {
      return true;
    }
  }

  return false;
}

bool RDLRoute::isIntersecting(const odb::Point& point, int extent) const
{
  if (!isRouted()) {
    return false;
  }

  extent /= 2;

  const odb::Rect point_rect(point.x() - extent,
                             point.y() - extent,
                             point.x() + extent,
                             point.y() + extent);

  for (const auto& pt : route_pts_) {
    const odb::Rect rect(
        pt.x() - extent, pt.y() - extent, pt.x() + extent, pt.y() + extent);

    if (point_rect.intersects(rect)) {
      return true;
    }
  }

  return false;
}

bool RDLRoute::contains(const odb::Point& pt) const
{
  return std::find(route_pts_.begin(), route_pts_.end(), pt)
         != route_pts_.end();
}

void RDLRoute::preprocess(odb::dbTechLayer* layer, utl::Logger* logger)
{
  using boost::polygon::operators::operator+=;
  using boost::polygon::operators::operator-=;

  using Rectangle = boost::polygon::rectangle_data<int>;
  using Polygon90 = boost::polygon::polygon_90_with_holes_data<int>;
  using Polygon90Set = boost::polygon::polygon_90_set_data<int>;
  using Pt = Polygon90::point_type;

  auto rect_to_poly = [](const odb::Rect& rect) -> Polygon90 {
    std::array<Pt, 4> pts = {Pt(rect.xMin(), rect.yMin()),
                             Pt(rect.xMax(), rect.yMin()),
                             Pt(rect.xMax(), rect.yMax()),
                             Pt(rect.xMin(), rect.yMax())};

    Polygon90 poly;
    poly.set(pts.begin(), pts.end());
    return poly;
  };

  Polygon90Set source_iterms;

  bool has_layer = false;
  for (const auto& [iterm_layer, iterm_rect] : iterm_->getGeometries()) {
    if (iterm_layer == layer) {
      has_layer = true;
      source_iterms += rect_to_poly(iterm_rect);
    }
  }
  if (!has_layer) {
    return;
  }

  std::map<odb::dbITerm*, Polygon90Set> iterms_geoms;

  // Create geom of all shapes
  for (odb::dbITerm* iterm : terminals_) {
    bool iterm_has_layer = false;
    for (const auto& [iterm_layer, iterm_rect] : iterm->getGeometries()) {
      if (iterm_layer == layer) {
        iterm_has_layer = true;
        iterms_geoms[iterm] += rect_to_poly(iterm_rect);
      }
    }
    if (!iterm_has_layer) {
      return;
    }
  }

  // check if route is even needed, ie, shapes overlap
  for (const auto& [iterm, polys] : iterms_geoms) {
    Polygon90Set check = polys;
    check.interact(source_iterms);
    if (!check.empty()) {
      // There is nothing to do
      locked_ = true;
      routed_ = true;

      routed_terminals_.insert(iterm_);
      routed_terminals_.insert(iterm);
      return;
    }
  }

  const int min_dist = layer->getSpacing();

  for (const auto& [iterm, polys] : iterms_geoms) {
    Polygon90Set check = polys;
    check.bloat(min_dist, min_dist, min_dist, min_dist);
    check.interact(source_iterms);
    if (!check.empty()) {
      Polygon90Set source_bloat = source_iterms;
      source_bloat.bloat(min_dist, min_dist, min_dist, min_dist);
      check += source_bloat;
      check.shrink(min_dist, min_dist, min_dist, min_dist);
      check -= source_iterms;

      std::vector<Rectangle> combined_rects;
      check.get_rectangles(combined_rects);
      for (const auto& rect : combined_rects) {
        stubs_.emplace(xl(rect), yl(rect), xh(rect), yh(rect));
      }
      // There is nothing to do
      locked_ = true;
      routed_ = true;
      routed_terminals_.insert(iterm_);
      routed_terminals_.insert(iterm);

      return;
    }
  }
}

std::set<odb::dbITerm*> RDLRoute::getRoutedTerminals() const
{
  if (!routed_terminals_.empty()) {
    return routed_terminals_;
  }

  std::set<odb::dbITerm*> terms;
  if (route_source_) {
    terms.insert(route_source_->terminal);
  }
  if (route_dest_) {
    terms.insert(route_dest_->terminal);
  }

  return terms;
}

}  // namespace pad
