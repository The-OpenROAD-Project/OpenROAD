// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024-2025, The OpenROAD Authors

#pragma once

#include <map>
#include <memory>
#include <set>
#include <vector>

#include "RDLRouter.h"
#include "odb/geom.h"

namespace odb {
class dbITerm;
class dbNet;
}  // namespace odb

namespace pad {

class RDLRoute
{
 public:
  RDLRoute(odb::dbITerm* source, const std::vector<odb::dbITerm*>& dests);

  void setRoute(const std::map<GridGraphVertex, odb::Point>& vertex_point_map,
                const std::vector<GridGraphVertex>& vertex,
                const std::vector<RDLRouter::GridEdge>& removed_edges,
                const RouteTarget* source,
                const RouteTarget* target,
                const RDLRouter::TerminalAccess& access_source,
                const RDLRouter::TerminalAccess& access_dest);
  void resetRoute();

  bool isRouted() const { return routed_; }
  bool isFailed() const
  {
    return !route_pending_ && !isRouted() && !hasNextTerminal();
  }
  bool allowRipup(int other_priority) const
  {
    return isRouted() && other_priority >= priority_;
  }

  void markRouting() { route_pending_ = false; }

  void preprocess(odb::dbTechLayer* layer, utl::Logger* logger);

  int getPriority() const { return priority_; }
  odb::dbITerm* getTerminal() const { return iterm_; }
  odb::dbNet* getNet() const { return iterm_->getNet(); }

  const std::vector<odb::dbITerm*>& getTerminals() const { return terminals_; }
  std::set<odb::dbITerm*> getRoutedTerminals() const;
  const std::set<odb::Rect>& getStubs() const { return stubs_; }

  void increasePriority() { priority_++; }

  bool hasNextTerminal() const { return next_ != terminals_.end(); }
  odb::dbITerm* getNextTerminal();
  odb::dbITerm* peakNextTerminal() const;
  void moveNextTerminalToEnd() { next_ = terminals_.end(); }

  bool compare(const std::shared_ptr<RDLRoute>& other) const;

  const std::vector<GridGraphVertex>& getRouteVerticies() const
  {
    return route_vertex_;
  }
  const std::vector<odb::Point>& getRoutePoints() const { return route_pts_; }
  const std::vector<RDLRouter::GridEdge>& getRouteEdges() const
  {
    return route_edges_;
  }
  const RouteTarget* getRouteTargetSource() const { return route_source_; }
  const RouteTarget* getRouteTargetDestination() const { return route_dest_; }
  const RDLRouter::TerminalAccess& getTerminalAccessSource() const
  {
    return access_source_;
  }
  const RDLRouter::TerminalAccess& getTerminalAccessDestination() const
  {
    return access_dest_;
  }

  odb::Rect getBBox(int bloat = 0) const;

  static odb::Rect getPointObstruction(const odb::Point& pt, int dist);
  static odb::Polygon getEdgeObstruction(const odb::Point& pt0,
                                         const odb::Point& pt1,
                                         int dist);
  static bool is45DegreeEdge(const odb::Point& pt0, const odb::Point& pt1);

  bool isIntersecting(RDLRoute* other, int extent) const;
  bool isIntersecting(const odb::Line& line, int extent) const;
  bool isIntersecting(const odb::Point& point, int width, int spacing) const;

 private:
  odb::dbITerm* iterm_;
  int priority_;

  bool route_pending_;
  bool locked_;
  bool routed_;

  std::vector<odb::dbITerm*> terminals_;
  std::vector<odb::dbITerm*>::iterator next_;

  std::vector<GridGraphVertex> route_vertex_;
  std::vector<odb::Point> route_pts_;
  std::vector<RDLRouter::GridEdge> route_edges_;
  const RouteTarget* route_source_;
  const RouteTarget* route_dest_;
  std::set<odb::dbITerm*> routed_terminals_;
  std::set<odb::Rect> stubs_;
  odb::Rect bbox_;

  RDLRouter::TerminalAccess access_source_;
  RDLRouter::TerminalAccess access_dest_;

  bool contains(const odb::Point& pt) const;
  void setRouted();
};

}  // namespace pad
