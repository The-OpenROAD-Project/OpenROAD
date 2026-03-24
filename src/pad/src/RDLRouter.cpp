// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2025, The OpenROAD Authors

#include "RDLRouter.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <list>
#include <map>
#include <memory>
#include <queue>
#include <set>
#include <tuple>
#include <utility>
#include <vector>

#include "RDLGui.h"
#include "RDLRoute.h"
#include "Utilities.h"
#include "boost/geometry/geometries/point_xy.hpp"
#include "boost/geometry/geometry.hpp"
#include "boost/graph/astar_search.hpp"
#include "boost/graph/lookup_edge.hpp"
#include "boost/polygon/polygon.hpp"
#include "odb/db.h"
#include "odb/dbObject.h"
#include "odb/dbTransform.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"
#include "pad/ICeWall.h"
#include "utl/Logger.h"

namespace pad {

class RDLRouterDistanceHeuristic
    : public boost::astar_heuristic<GridGraph, int64_t>
{
 public:
  RDLRouterDistanceHeuristic(
      const std::map<GridGraphVertex, odb::Point>& vertex_map,
      const std::vector<GridGraphVertex>& predecessor,
      const GridGraphVertex& start_vertex,
      const odb::Point& goal,
      float turn_penalty)
      : vertex_map_(vertex_map),
        predecessor_(predecessor),
        start_vertex_(start_vertex),
        goal_(goal),
        turn_penalty_(turn_penalty)
  {
  }
  int64_t operator()(GridGraphVertex vt_next)
  {
    const auto& pt_next = vertex_map_.at(vt_next);

    const int64_t distance = RDLRouter::distance(goal_, pt_next);

    const auto& vt_curr = predecessor_[vt_next];
    if (start_vertex_ == vt_curr) {
      return distance;
    }

    const auto& vt_prev = predecessor_[vt_curr];
    if (start_vertex_ == vt_prev) {
      return distance;
    }

    const auto& pt_curr = vertex_map_.at(vt_curr);
    const auto& pt_prev = vertex_map_.at(vt_prev);

    const odb::Point incoming_vec(pt_curr.x() - pt_prev.x(),
                                  pt_curr.y() - pt_prev.y());
    const odb::Point outgoing_vec(pt_next.x() - pt_curr.x(),
                                  pt_next.y() - pt_curr.y());

    int64_t penalty = 0;
    if (incoming_vec != outgoing_vec) {
      penalty = turn_penalty_ * RDLRouter::distance(pt_prev, pt_curr);
    }

    return distance + penalty;
  }

 private:
  const std::map<GridGraphVertex, odb::Point>& vertex_map_;
  const std::vector<GridGraphVertex>& predecessor_;
  const GridGraphVertex& start_vertex_;
  odb::Point goal_;
  const float turn_penalty_;
};

struct RDLRouterGoalFound
{
};  // exception for termination

// visitor that terminates when we find the goal
template <class Vertex>
class RDLRouterGoalVisitor : public boost::default_astar_visitor
{
 public:
  explicit RDLRouterGoalVisitor(GridGraphVertex goal) : goal_(goal) {}
  template <class Graph>
  void examine_vertex(Vertex u, Graph& g)
  {
    if (u == goal_) {
      throw RDLRouterGoalFound();
    }
  }

 private:
  GridGraphVertex goal_;
};

//////////////////////////////////////////////////////////////

RDLRouter::RDLRouter(utl::Logger* logger,
                     odb::dbBlock* block,
                     odb::dbTechLayer* layer,
                     odb::dbTechVia* bump_via,
                     odb::dbTechVia* pad_via,
                     const std::map<odb::dbITerm*, odb::dbITerm*>& routing_map,
                     int width,
                     int spacing,
                     bool allow45,
                     float turn_penalty,
                     int max_iterations)
    : logger_(logger),
      block_(block),
      layer_(layer),
      bump_accessvia_(bump_via),
      pad_accessvia_(pad_via),
      width_(width),
      spacing_(spacing),
      allow45_(allow45),
      turn_penalty_(turn_penalty),
      max_router_iterations_(max_iterations),
      routing_map_(routing_map),
      gui_(nullptr)
{
  if (width_ == 0) {
    width_ = layer_->getWidth();
  }
  if (width_ < layer_->getWidth()) {
    const double dbus = block_->getDbUnitsPerMicron();
    logger_->warn(
        utl::PAD,
        3,
        "{:.3f}um is below the minimum width for {}, changing to {:.3f}um",
        width_ / dbus,
        layer_->getName(),
        layer_->getWidth() / dbus);
    width_ = layer_->getWidth();
  }
  if (spacing_ == 0) {
    spacing_ = layer_->getSpacing(width_);
  }
  if (spacing_ < layer_->getSpacing(width_)) {
    const double dbus = block_->getDbUnitsPerMicron();
    logger_->warn(
        utl::PAD,
        4,
        "{:.3f}um is below the minimum spacing for {}, changing to {:.3f}um",
        spacing_ / dbus,
        layer_->getName(),
        layer_->getSpacing(width_) / dbus);
    spacing_ = layer_->getSpacing(width_);
  }
}

RDLRouter::~RDLRouter()
{
  if (gui_ != nullptr) {
    gui_->setRouter(nullptr);
  }
}

void RDLRouter::buildIntialRouteSet()
{
  routes_.clear();

  for (const auto& [net, iterm_targets] : routing_targets_) {
    for (const auto& [iterm, targets] : iterm_targets) {
      if (!isCoverTerm(iterm)) {
        // only collect cover terms
        continue;
      }

      std::vector<odb::dbITerm*> iterm_pairs;
      auto assigned_route = routing_map_.find(iterm);
      if (assigned_route != routing_map_.end()) {
        if (assigned_route->second != nullptr) {
          iterm_pairs.push_back(assigned_route->second);
        }
      } else {
        for (const auto& [piterm, targets] : iterm_targets) {
          if (iterm->getInst() != piterm->getInst()) {
            iterm_pairs.push_back(piterm);
          }
        }
      }

      if (iterm_pairs.empty()) {
        continue;
      }

      routes_.emplace_back(new RDLRoute(iterm, iterm_pairs));
    }
  }
}

int RDLRouter::getRoutingInstanceCount() const
{
  std::set<odb::dbInst*> insts;
  for (const auto& route : routes_) {
    if (route->isRouted()) {
      for (odb::dbITerm* iterm : route->getRoutedTerminals()) {
        insts.insert(iterm->getInst());
      }
    } else {
      insts.insert(route->getTerminal()->getInst());
      for (const auto* iterm : route->getTerminals()) {
        insts.insert(iterm->getInst());
      }
    }
  }
  return insts.size();
}

std::set<odb::dbInst*> RDLRouter::getRoutedInstances() const
{
  std::set<odb::dbInst*> insts;
  for (const auto& route : routes_) {
    if (route->isRouted()) {
      for (odb::dbITerm* iterm : route->getRoutedTerminals()) {
        insts.insert(iterm->getInst());
      }
    }
  }
  return insts;
}

std::vector<RDLRouter::RDLRoutePtr> RDLRouter::getFailedRoutes() const
{
  // record sucessful
  std::set<odb::dbInst*> success_covers;
  for (auto& route : routes_) {
    if (route->isRouted()) {
      for (odb::dbITerm* iterm : route->getRoutedTerminals()) {
        if (isCoverTerm(iterm)) {
          success_covers.insert(iterm->getInst());
        }
      }
    }
  }

  std::vector<RDLRoutePtr> failed;
  for (auto& route : routes_) {
    if (!route->isFailed()) {
      continue;
    }

    if (success_covers.find(route->getTerminal()->getInst())
        != success_covers.end()) {
      continue;
    }

    failed.push_back(route);
  }

  return failed;
}

int RDLRouter::reportFailedRoutes(
    const std::map<odb::dbITerm*, odb::dbITerm*>& routed_pairs) const
{
  std::map<odb::dbNet*, std::set<odb::dbITerm*>> failed;
  std::map<odb::dbITerm*, RDLRoute*> route_map;
  for (const auto& route : getFailedRoutes()) {
    route_map[route->getTerminal()] = route.get();
    failed[route->getNet()].insert(route->getTerminal());
  }

  if (!failed.empty()) {
    logger_->warn(
        utl::PAD, 6, "Failed to route the following {} nets:", failed.size());
    for (const auto& [net, iterms] : failed) {
      logger_->report("  {}", net->getName());
      for (auto* iterm : iterms) {
        std::vector<odb::dbITerm*> no_routes_to;
        for (auto* dst_iterm : route_map[iterm]->getTerminals()) {
          auto find_routing = routed_pairs.find(iterm);
          if (find_routing != routed_pairs.end()
              && find_routing->second == dst_iterm) {
            continue;
          }
          no_routes_to.push_back(dst_iterm);
        }

        const size_t max_print_length = 5;
        std::string terms;
        for (size_t i = 0; i < no_routes_to.size() && i < max_print_length;
             i++) {
          if (!terms.empty()) {
            terms += ", ";
          }
          terms += no_routes_to[i]->getName();
        }
        if (no_routes_to.size() < max_print_length) {
          logger_->report("    {} -> {}", iterm->getName(), terms);
        } else {
          logger_->report("    {} -> {}, ... ({} possible terminals)",
                          iterm->getName(),
                          terms,
                          no_routes_to.size());
        }
      }
    }
  }

  return failed.size();
}

void RDLRouter::route(const std::vector<odb::dbNet*>& nets)
{
  if (debug_net_ != nullptr) {
    logger_->report("RDL Router debugging net: {}", debug_net_->getName());
  }
  routing_targets_.clear();

  // Build list of routing targets
  for (auto* net : nets) {
    const auto targets = generateRoutingTargets(net);
    if (targets.size() > 1) {
      routing_targets_[net] = targets;
    }
  }

  if (routing_targets_.empty()) {
    logger_->error(utl::PAD, 43, "Nothing to route");
  }

  const double dbus = block_->getDbUnitsPerMicron();

  // Build obstructions
  populateObstructions(nets);

  // build graph
  makeGraph();

  // Determine access points
  for (auto& [net, iterm_targets] : routing_targets_) {
    for (auto& [iterm, targets] : iterm_targets) {
      for (auto& target : targets) {
        populateTerminalAccessPoints(target);
      }
    }
  }

  if (gui_ != nullptr) {
    gui_->pause(false);
  }

  // Build list of routes
  buildIntialRouteSet();

  // Process preprocessing
  for (const auto& route : routes_) {
    route->preprocess(layer_, logger_);
  }

  // create priority queue
  auto route_compare
      = [](const std::shared_ptr<RDLRoute>& lhs,
           const std::shared_ptr<RDLRoute>& rhs) { return lhs->compare(rhs); };
  std::priority_queue<RDLRoutePtr,
                      std::vector<RDLRoutePtr>,
                      decltype(route_compare)>
      route_queue;

  logger_->info(utl::PAD, 5, "Routing {} nets", nets.size());

  // track sets of routes, so we don't route the reverse by accident
  std::map<odb::dbITerm*, odb::dbITerm*> routed_pairs;
  // track cover instances we dont route the same one twice
  std::set<odb::dbInst*> routed_covers;
  // track non-cover iterms we dont route the same one twice
  std::set<odb::dbITerm*> routed_non_covers;
  // track iteration information
  int iteration_count = 0;
  std::set<odb::dbInst*> last_itr_routed;

  // add initial queue
  for (const auto& route : routes_) {
    if (route->isRouted()) {
      for (odb::dbITerm* iterm0 : route->getRoutedTerminals()) {
        if (isCoverTerm(iterm0)) {
          routed_covers.insert(iterm0->getInst());
        } else {
          routed_non_covers.insert(iterm0);
        }
        for (odb::dbITerm* iterm1 : route->getRoutedTerminals()) {
          if (iterm0 == iterm1) {
            continue;
          }
          routed_pairs[iterm0] = iterm1;
          routed_pairs[iterm1] = iterm0;
        }
      }
    } else {
      // Only add routes that need to routed
      route_queue.push(route);
    }
  }
  while (!route_queue.empty()) {
    RDLRoutePtr route = route_queue.top();
    odb::dbITerm* src = route->getTerminal();
    route_queue.pop();

    route->markRouting();

    odb::dbNet* net = src->getNet();
    auto& net_targets = routing_targets_[net];

    if (routed_covers.find(src->getInst()) != routed_covers.end()) {
      // we've already routed this cover once (indicates the cover has multiple
      // iterms), so go ahead and mark is as complete and skip.

      debugPrint(logger_,
                 utl::PAD,
                 "Router",
                 2,
                 "Cover already routed: {}",
                 src->getName());

      routed_pairs[src] = nullptr;
      route->moveNextTerminalToEnd();
    } else {
      // get the next destination
      odb::dbITerm* dst = nullptr;
      do {
        if (!route->hasNextTerminal()) {
          dst = nullptr;
          break;
        }
        dst = route->getNextTerminal();
      } while (routed_non_covers.find(dst) != routed_non_covers.end()
               || routed_pairs[src] == dst);

      if (dst != nullptr) {
        // create ordered set of iterm targets
        std::vector<TargetPair> targets;
        for (const auto& src_target : net_targets[src]) {
          for (const auto& dst_target : net_targets[dst]) {
            targets.push_back(TargetPair{&src_target, &dst_target});
          }
        }

        std::ranges::stable_sort(targets, [](const auto& lhs, const auto& rhs) {
          return distance(lhs) < distance(rhs);
        });

        debugPrint(
            logger_,
            utl::PAD,
            "Router",
            2,
            "Routing {} -> {} ({}): ({:.3f}um) / {} target pairs / {} priority",
            src->getName(),
            dst->getName(),
            net->getName(),
            distance(src->getBBox().center(), dst->getBBox().center()) / dbus,
            targets.size(),
            route->getPriority());

        for (auto& points : targets) {
          debugPrint(
              logger_,
              utl::PAD,
              "Router",
              3,
              "Routing {} ({:.3f}um, {:.3f}um) -> ({:.3f}um, {:.3f}um) : "
              "({:.3f}um)",
              net->getName(),
              points.target0->center.x() / dbus,
              points.target0->center.y() / dbus,
              points.target1->center.x() / dbus,
              points.target1->center.y() / dbus,
              distance(points) / dbus);

          const TerminalAccess access0
              = insertTerminalAccess(*points.target0, *points.target1);
          const TerminalAccess access1
              = insertTerminalAccess(*points.target1, *points.target0);

          auto route_vextex
              = run(points.target0->center, points.target1->center);

          if (!route_vextex.empty()) {
            debugPrint(logger_,
                       utl::PAD,
                       "Router",
                       3,
                       "Route segments {}",
                       route_vextex.size());
            const auto route_edges = commitRoute(route_vextex);
            route->setRoute(vertex_point_map_,
                            route_vextex,
                            route_edges,
                            points.target0,
                            points.target1,
                            access0,
                            access1);

            // record cover instance
            if (isCoverTerm(src)) {
              routed_covers.insert(src->getInst());
            } else {
              routed_non_covers.insert(src);
            }
            if (isCoverTerm(dst)) {
              routed_covers.insert(dst->getInst());
            } else {
              routed_non_covers.insert(dst);
            }

            // record routed pair (forward and reverse) to avoid routing this
            // segment again
            routed_pairs[src] = dst;
            routed_pairs[dst] = src;
          }

          removeTerminalAccess(access0);
          removeTerminalAccess(access1);

          if (route->isRouted()) {
            break;
          }
        }
      }

      if (!route->isRouted()) {
        if (route->hasNextTerminal()) {
          route_queue.push(route);
        }
      }
    }

    if (gui_ != nullptr
        && (logger_->debugCheck(utl::PAD, "Router", 3) || isDebugNet(net))) {
      const bool use_timeout = route->isRouted() && !isDebugNet(net);
      gui_->pause(use_timeout);
    }

    if (route_queue.empty() && iteration_count < max_router_iterations_) {
      // at the end of the queue, should check for failures and retry
      iteration_count++;

      const auto routed_insts = getRoutedInstances();
      logger_->info(utl::PAD,
                    37,
                    "End of routing iteration {}: {:.1f}% complete",
                    iteration_count,
                    100 * static_cast<double>(routed_insts.size())
                        / getRoutingInstanceCount());

      if (last_itr_routed == routed_insts) {
        continue;
      }
      last_itr_routed = routed_insts;

      std::vector<RDLRoutePtr> failed = getFailedRoutes();

      if (failed.empty()) {
        continue;
      }

      if (gui_ != nullptr
          && (logger_->debugCheck(utl::PAD, "Router", 2) || isDebugNet(net))) {
        gui_->pause(false);
      }

      debugPrint(logger_,
                 utl::PAD,
                 "Router_ripup",
                 1,
                 "Failed routes at end of {} iteration: {} routes",
                 iteration_count,
                 failed.size());

      for (const auto& route : failed) {
        debugPrint(logger_,
                   utl::PAD,
                   "Router_ripup",
                   2,
                   "  {}",
                   route->getTerminal()->getName());
      }

      // find routes to ripup
      std::set<RDLRoutePtr> ripup;
      for (const auto& failed_route : failed) {
        for (const auto& route : routes_) {
          if (!route->isRouted()) {
            continue;
          }
          if (!route->allowRipup(failed_route->getPriority())) {
            continue;
          }
          if (failed_route->isIntersecting(route.get(), spacing_ + width_)) {
            ripup.insert(route);
          }
        }
      }

      debugPrint(logger_,
                 utl::PAD,
                 "Router_ripup",
                 1,
                 "Ripping up {} routes",
                 ripup.size());

      // ripup routing
      for (auto& ripup_route : ripup) {
        uncommitRoute(ripup_route->getRouteEdges());
        routed_covers.erase(
            ripup_route->getRouteTargetSource()->terminal->getInst());
        routed_covers.erase(
            ripup_route->getRouteTargetDestination()->terminal->getInst());
        routed_non_covers.erase(ripup_route->getRouteTargetSource()->terminal);
        routed_non_covers.erase(
            ripup_route->getRouteTargetDestination()->terminal);
        routed_pairs.erase(ripup_route->getRouteTargetSource()->terminal);
        routed_pairs.erase(ripup_route->getRouteTargetDestination()->terminal);
        ripup_route->resetRoute();
      }

      // update failed routes
      for (auto& route : failed) {
        // increment route priority
        route->increasePriority();
        route->resetRoute();
      }

      // push all back
      for (const auto& route : failed) {
        route_queue.push(route);
      }
      for (const auto& route : ripup) {
        route_queue.push(route);
      }

      if (gui_ != nullptr && logger_->debugCheck(utl::PAD, "Router", 2)) {
        gui_->pause(false);
      }
    }
  }

  const int failed_net_count = reportFailedRoutes(routed_pairs);

  // smooth wire
  // write to DB

  // remove old routes
  for (const auto& route : routes_) {
    auto swires = route->getNet()->getSWires();
    for (auto itr = swires.begin(); itr != swires.end();) {
      if ((*itr)->getWireType() == odb::dbWireType::FIXED) {
        itr++;
        continue;
      }
      itr = odb::dbSWire::destroy(itr);
    }
  }

  for (const auto& route : routes_) {
    if (route->isRouted()) {
      writeToDb(route->getNet(),
                route->getRoutePoints(),
                route->getRouteTargetSource(),
                route->getRouteTargetDestination(),
                route->getStubs());
    }
  }

  if (failed_net_count > 0) {
    logger_->error(utl::PAD, 7, "Failed to route {} nets.", failed_net_count);
  }
}

void RDLRouter::removeTerminalAccess(const TerminalAccess& access)
{
  for (const auto& [pt0, pt1] : access.added_edges) {
    boost::remove_edge(point_vertex_map_[pt0], point_vertex_map_[pt1], graph_);
  }

  for (const auto& pt : access.added_points) {
    removeGraphVertex(pt);
  }

  for (const auto& [pt0, pt1, weight] : access.removed_edges) {
    addGraphEdge(pt0, pt1, weight, true, true);
  }
}

std::set<odb::Point> RDLRouter::generateTerminalAccessPoints(
    const odb::Point& pt,
    bool do_x) const
{
  std::set<odb::Point> pts;

  if (do_x) {
    for (const int x : x_grid_) {
      const odb::Point check(x, pt.y());
      if (!isEdgeObstructed(check, check, false)) {
        pts.insert(check);
      }
    }
  } else {
    for (const int y : y_grid_) {
      const odb::Point check(pt.x(), y);
      if (!isEdgeObstructed(check, check, false)) {
        pts.insert(check);
      }
    }
  }

  return pts;
}

template <class InputIt>
static odb::Point getValidGridPoint(
    InputIt begin,
    const InputIt& end,
    const std::function<bool(const odb::Point&)>& valid)
{
  odb::Point snap;
  while (begin != end) {
    if (valid(*begin)) {
      snap = *begin;
    } else {
      break;
    }
    begin++;
  }

  return snap;
}

void RDLRouter::populateTerminalAccessPoints(RouteTarget& target) const
{
  // determine new access point in graph
  std::set<odb::Point> snap_pts;

  const std::set<odb::Point> x_pts
      = generateTerminalAccessPoints(target.center, true);
  snap_pts.insert(getValidGridPoint(
      x_pts.begin(), x_pts.end(), [&target](const odb::Point& pt) {
        return pt.x() < target.center.x();
      }));
  snap_pts.insert(getValidGridPoint(
      x_pts.rbegin(), x_pts.rend(), [&target](const odb::Point& pt) {
        return pt.x() > target.center.x();
      }));

  const std::set<odb::Point> y_pts
      = generateTerminalAccessPoints(target.center, false);
  snap_pts.insert(getValidGridPoint(
      y_pts.begin(), y_pts.end(), [&target](const odb::Point& pt) {
        return pt.y() < target.center.y();
      }));
  snap_pts.insert(getValidGridPoint(
      y_pts.rbegin(), y_pts.rend(), [&target](const odb::Point& pt) {
        return pt.y() > target.center.y();
      }));

  // Remove snap points that would cause a violation
  //   insersects an obstruction
  //   insersects another edge
  for (auto snap_itr = snap_pts.begin(); snap_itr != snap_pts.end();) {
    const odb::Line line(target.center, *snap_itr);
    bool erase = obstructions_.qbegin(
                     boost::geometry::index::intersects(line.getPoints())
                     && boost::geometry::index::satisfies(
                         [&target](const ObsValue& value) {
                           return std::get<3>(value) != target.terminal;
                         }))
                 != obstructions_.qend();

    if (!erase) {
      for (auto itr = vertex_grid_tree_.qbegin(
               boost::geometry::index::intersects(line.getPoints()));
           itr != vertex_grid_tree_.qend();
           itr++) {
        const odb::Point& pt = vertex_point_map_.at(itr->second);
        if (pt.x() == snap_itr->x() || pt.y() == snap_itr->y()) {
          continue;
        }

        for (const auto& edge : getVertexEdges(itr->second)) {
          const odb::Point& pt0 = vertex_point_map_.at(edge.m_source);
          if (pt0 == line.pt0() || pt0 == line.pt1()) {
            // lines will connect, so keep
            continue;
          }
          const odb::Point& pt1 = vertex_point_map_.at(edge.m_target);
          if (pt1 == line.pt0() || pt1 == line.pt1()) {
            // lines will connect, so keep
            continue;
          }
          const odb::Line edge_line(pt0, pt1);

          if (boost::geometry::intersects(line.getPoints(),
                                          edge_line.getPoints())) {
            erase = true;
            break;
          }
        }
      }
    }

    if (erase) {
      snap_itr = snap_pts.erase(snap_itr);
    } else {
      snap_itr++;
    }
  }

  // Check for intersections with non-rect edges
  //   if atleast one passes a non-rect edge, remove all violating points
  std::vector<odb::Polygon> iterm_shapes;
  const auto xform = target.terminal->getInst()->getTransform();
  for (auto* mpin : target.terminal->getMTerm()->getMPins()) {
    for (auto* box : mpin->getPolygonGeometry()) {
      if (box->getTechLayer() == target.layer) {
        odb::Polygon poly = box->getPolygon();
        xform.apply(poly);
        iterm_shapes.push_back(poly);
      }
    }
  }
  if (!iterm_shapes.empty()) {
    std::set<odb::Point> poly_intersect;
    for (const auto& snap_pt : snap_pts) {
      const odb::Line access(target.center, snap_pt);
      for (const auto& poly : iterm_shapes) {
        const auto& points = poly.getPoints();
        for (int i = 1; i < points.size(); i++) {
          const odb::Line edge(points[i - 1], points[i]);

          // ignore rect edges
          if (edge.pt0().x() == edge.pt1().x()
              || edge.pt0().y() == edge.pt1().y()) {
            continue;
          }

          if (boost::geometry::intersects(access.getPoints(),
                                          edge.getPoints())) {
            poly_intersect.insert(snap_pt);
          }
        }
      }
    }

    // check if points can be removed
    if (poly_intersect.size() < snap_pts.size()) {
      for (const odb::Point& pt : poly_intersect) {
        snap_pts.erase(pt);
      }
    }
  }

  if (logger_->debugCheck(utl::PAD, "Terminal", 1) && gui_ != nullptr) {
    for (const auto& snap : snap_pts) {
      gui_->addSnap(target.center, snap);
    }
    gui_->zoomToSnap(true);
    gui_->pause(!isDebugNet(target.terminal->getNet())
                && !isDebugPin(target.terminal));
    gui_->clearSnap();
  }

  target.grid_access = std::move(snap_pts);
}

RDLRouter::TerminalAccess RDLRouter::insertTerminalAccess(
    const RouteTarget& target,
    const RouteTarget& source)
{
  TerminalAccess access;

  // Remove snap points that would cause a violation
  //   intersects another route
  std::set<odb::Point> snap_pts = target.grid_access;
  for (auto snap_itr = snap_pts.begin(); snap_itr != snap_pts.end();) {
    bool erase = false;

    for (const auto& route : routes_) {
      if (route->isIntersecting(*snap_itr, width_, spacing_)) {
        erase = true;
        break;
      }
    }

    debugPrint(logger_,
               utl::PAD,
               "Terminal",
               2,
               "Removing access to ({}, {})",
               snap_itr->x(),
               snap_itr->y());

    if (erase) {
      snap_itr = snap_pts.erase(snap_itr);
    } else {
      snap_itr++;
    }
  }

  if (logger_->debugCheck(utl::PAD, "Terminal", 1) && gui_ != nullptr) {
    for (const auto& snap : snap_pts) {
      gui_->addSnap(target.center, snap);
    }
    gui_->zoomToSnap(true);
    gui_->pause(!isDebugNet(target.terminal->getNet())
                && !isDebugPin(target.terminal));
    gui_->clearSnap();
  }

  // prepare routing graph
  if (point_vertex_map_.find(target.center) == point_vertex_map_.end()) {
    if (addGraphVertex(target.center)) {
      access.added_points.insert(target.center);
    }
  }

  for (const odb::Point& snap : snap_pts) {
    std::vector<GridGraphVertex> vertex_to_modify;

    // remove intersecting edges
    for (auto itr
         = vertex_grid_tree_.qbegin(boost::geometry::index::intersects(snap));
         itr != vertex_grid_tree_.qend();
         itr++) {
      const auto& pt = vertex_point_map_[itr->second];
      if (pt.x() == snap.x() || pt.y() == snap.y()) {
        vertex_to_modify.push_back(itr->second);
      }
    }

    for (const auto& vertex : vertex_to_modify) {
      for (const auto& edge : getVertexEdges(vertex)) {
        if (std::ranges::find(vertex_to_modify, edge.m_source)
                != vertex_to_modify.end()
            && std::ranges::find(vertex_to_modify, edge.m_target)
                   != vertex_to_modify.end()) {
          access.removed_edges.push_back(removeGraphEdge(edge));
        }
      }
    }

    // add new edges
    if (addGraphVertex(snap)) {
      access.added_points.insert(snap);
    }

    // target to new
    if (addGraphEdge(snap, target.center, 1.0, false)) {
      access.added_edges.push_back(Edge{snap, target.center});
    }

    for (const auto& vertex : vertex_to_modify) {
      const odb::Point& pt = vertex_point_map_[vertex];
      if (addGraphEdge(snap, pt)) {
        access.added_edges.push_back(Edge{snap, pt});
      }

      if (allow45_) {
        // remove accute edges
        for (const auto& edge : getVertexEdges(vertex)) {
          bool remove = false;

          const auto other
              = edge.m_source == vertex ? edge.m_target : edge.m_source;
          const odb::Point& other_pt = vertex_point_map_[other];
          if (other_pt == snap) {
            continue;
          }
          if (pt.x() == other_pt.x() || pt.y() == other_pt.y()) {
            // right angle
            continue;
          }
          const int snap_dx = pt.x() - snap.x();
          const int snap_dy = pt.y() - snap.y();

          const int edge_dx = pt.x() - other_pt.x();
          const int edge_dy = pt.y() - other_pt.y();

          if (snap_dy == 0) {
            if ((snap_dx < 0 && edge_dx < 0) || (snap_dx > 0 && edge_dx > 0)) {
              remove = true;
            }
          } else {
            if ((snap_dy < 0 && edge_dy < 0) || (snap_dy > 0 && edge_dy > 0)) {
              remove = true;
            }
          }

          if (remove) {
            access.removed_edges.push_back(removeGraphEdge(edge));
          }
        }
      }
    }

    if (logger_->debugCheck(utl::PAD, "Terminal", 1) && gui_ != nullptr) {
      gui_->addSnap(target.center, snap);
    }
  }

  if (logger_->debugCheck(utl::PAD, "Terminal", 1) && gui_ != nullptr) {
    gui_->zoomToSnap(false);
    gui_->pause(!isDebugNet(target.terminal->getNet())
                && !isDebugPin(target.terminal));
    gui_->clearSnap();
  }

  return access;
}

void RDLRouter::uncommitRoute(const std::vector<RDLRouter::GridEdge>& route)
{
  for (const auto& [p0, p1, weight] : route) {
    addGraphEdge(p0, p1, weight, false, false);
  }
}

odb::Rect RDLRouter::getPointObstruction(const odb::Point& pt) const
{
  const int check_dist = width_ / 2 + spacing_ + 1;
  return RDLRoute::getPointObstruction(pt, check_dist);
}

odb::Polygon RDLRouter::getEdgeObstruction(const odb::Point& pt0,
                                           const odb::Point& pt1) const
{
  const int check_dist = width_ / 2 + spacing_ + 1;
  return RDLRoute::getEdgeObstruction(pt0, pt1, check_dist);
}

bool RDLRouter::is45DegreeEdge(const odb::Point& pt0,
                               const odb::Point& pt1) const
{
  return RDLRoute::is45DegreeEdge(pt0, pt1);
}

std::set<GridGraphEdge> RDLRouter::getVertexEdges(
    const GridGraphVertex& vertex) const
{
  std::set<GridGraphEdge> edges;

  GridGraph::out_edge_iterator oit, oend;
  std::tie(oit, oend) = boost::out_edges(vertex, graph_);
  for (; oit != oend; oit++) {
    edges.insert(*oit);
  }
  GridGraph::in_edge_iterator iit, iend;
  std::tie(iit, iend) = boost::in_edges(vertex, graph_);
  for (; iit != iend; iit++) {
    edges.insert(*iit);
  }

  return edges;
}

std::vector<RDLRouter::GridEdge> RDLRouter::commitRoute(
    const std::vector<GridGraphVertex>& route)
{
  std::set<GridGraphEdge> edges;
  for (const auto& v : route) {
    const auto v_edges = getVertexEdges(v);
    edges.insert(v_edges.begin(), v_edges.end());
  }

  // remove intersecting edges
  using Line = boost::geometry::model::segment<odb::Point>;
  auto handle_rect_edge
      = [this, &edges](const odb::Rect& rect, const GridGraphEdge& edge) {
          const odb::Point& lpt0 = vertex_point_map_[edge.m_source];
          const odb::Point& lpt1 = vertex_point_map_[edge.m_target];
          if (boost::geometry::intersects(rect, Line(lpt0, lpt1))) {
            edges.insert(edge);
          }
        };

  for (const auto& v : route) {
    const odb::Point& pt = vertex_point_map_[v];

    const odb::Rect check_box = getPointObstruction(pt);

    for (auto itr = vertex_grid_tree_.qbegin(
             boost::geometry::index::intersects(check_box));
         itr != vertex_grid_tree_.qend();
         itr++) {
      for (const auto& edge : getVertexEdges(itr->second)) {
        handle_rect_edge(check_box, edge);
      }
    }
  }

  if (allow45_) {
    // remove intersecting edges on 45 degrees

    auto handle_poly_edge
        = [this, &edges](const odb::Polygon& poly, const GridGraphEdge& edge) {
            const odb::Point& lpt0 = vertex_point_map_[edge.m_source];
            const odb::Point& lpt1 = vertex_point_map_[edge.m_target];
            if (boost::geometry::intersects(poly, Line(lpt0, lpt1))) {
              edges.insert(edge);
            }
          };

    for (std::size_t i = 2; i < route.size(); i++) {
      const odb::Point& pt0 = vertex_point_map_[route[i - 1]];
      const odb::Point& pt1 = vertex_point_map_[route[i]];

      if (!is45DegreeEdge(pt0, pt1)) {
        continue;
      }

      const odb::Polygon check_poly = getEdgeObstruction(pt0, pt1);

      for (auto itr = vertex_grid_tree_.qbegin(
               boost::geometry::index::intersects(check_poly));
           itr != vertex_grid_tree_.qend();
           itr++) {
        for (const auto& edge : getVertexEdges(itr->second)) {
          handle_poly_edge(check_poly, edge);
        }
      }
    }
  }

  std::vector<GridEdge> removed_edges;
  removed_edges.reserve(edges.size());
  for (const auto& edge : edges) {
    removed_edges.push_back(removeGraphEdge(edge));
  }
  return removed_edges;
}

RDLRouter::GridEdge RDLRouter::removeGraphEdge(const GridGraphEdge& edge)
{
  const float weight = graph_weight_[edge];
  boost::remove_edge(edge, graph_);

  return {vertex_point_map_[edge.m_source],
          vertex_point_map_[edge.m_target],
          weight
              / distance(vertex_point_map_[edge.m_source],
                         vertex_point_map_[edge.m_target])};
}

std::vector<GridGraphVertex> RDLRouter::run(const odb::Point& source,
                                            const odb::Point& dest)
{
  const int num_vertices = boost::num_vertices(graph_);
  std::vector<GridGraphVertex> p(num_vertices);
  std::vector<int64_t> d(num_vertices);

  const GridGraphVertex& start = point_vertex_map_[source];
  const GridGraphVertex& goal = point_vertex_map_[dest];

  debugPrint(logger_,
             utl::PAD,
             "Router",
             1,
             "Route ({}, {}) -> ({}, {})",
             source.x(),
             source.y(),
             dest.x(),
             dest.y());

  try {
    // call astar named parameter interface
    boost::astar_search_tree(
        graph_,
        start,
        RDLRouterDistanceHeuristic(
            vertex_point_map_, p, start, dest, turn_penalty_),
        boost::predecessor_map(
            boost::make_iterator_property_map(
                p.begin(), boost::get(boost::vertex_index, graph_)))
            .distance_map(boost::make_iterator_property_map(
                d.begin(), boost::get(boost::vertex_index, graph_)))
            .visitor(RDLRouterGoalVisitor<GridGraphVertex>(goal)));
  } catch (const RDLRouterGoalFound&) {  // found a path to the goal
    std::list<GridGraphVertex> shortest_path;
    for (GridGraphVertex v = goal;; v = p[v]) {
      shortest_path.push_front(v);
      if (p[v] == v) {
        break;
      }
    }

    std::vector<GridGraphVertex> route(shortest_path.begin(),
                                       shortest_path.end());
    return route;
  }

  return {};
}

void RDLRouter::makeGraph()
{
  point_vertex_map_.clear();
  vertex_point_map_.clear();
  graph_.clear();

  graph_weight_ = boost::get(boost::edge_weight, graph_);

  odb::dbTrackGrid* tracks = block_->findTrackGrid(layer_);

  // filter grid points based on spacing requirements
  const int pitch = width_ + spacing_ - 1;
  const int start = width_ / 2 + 1;
  x_grid_.clear();
  for (const auto& x : tracks->getGridX()) {
    bool add = false;
    if (x_grid_.empty()) {
      if (x >= start) {
        add = true;
      }
    } else {
      if (*x_grid_.rbegin() + pitch < x) {
        add = true;
      }
    }

    if (add) {
      x_grid_.push_back(x);
    }
  }
  y_grid_.clear();
  for (const auto& y : tracks->getGridY()) {
    bool add = false;
    if (y_grid_.empty()) {
      if (y >= start) {
        add = true;
      }
    } else {
      if (*y_grid_.rbegin() + pitch < y) {
        add = true;
      }
    }

    if (add) {
      y_grid_.push_back(y);
    }
  }

  for (const auto& x : x_grid_) {
    for (const auto& y : y_grid_) {
      addGraphVertex(odb::Point(x, y));
    }
  }

  debugPrint(logger_,
             utl::PAD,
             "Router",
             1,
             "Added {} vertices to graph",
             boost::num_vertices(graph_));

  for (size_t i = 0; i < x_grid_.size(); i++) {
    for (size_t j = 0; j < y_grid_.size(); j++) {
      const odb::Point center(x_grid_[i], y_grid_[j]);

      if (j + 1 < y_grid_.size()) {
        addGraphEdge(center, {x_grid_[i], y_grid_[j + 1]});
      }
      if (j != 0) {
        addGraphEdge(center, {x_grid_[i], y_grid_[j - 1]});
      }
      if (i != 0) {
        addGraphEdge(center, {x_grid_[i - 1], y_grid_[j]});
      }
      if (i + 1 < x_grid_.size()) {
        addGraphEdge(center, {x_grid_[i + 1], y_grid_[j]});
      }

      if (allow45_) {
        if (i % 2 == 1 || j % 2 == 1) {
          // only do every other position
          continue;
        }
        if (i + 1 < x_grid_.size() && j + 1 < y_grid_.size()) {
          addGraphEdge(center, {x_grid_[i + 1], y_grid_[j + 1]});
        }
        if (i + 1 < x_grid_.size() && j != 0) {
          addGraphEdge(center, {x_grid_[i + 1], y_grid_[j - 1]});
        }
        if (i != 0 && j + 1 < y_grid_.size()) {
          addGraphEdge(center, {x_grid_[i - 1], y_grid_[j + 1]});
        }
        if (i != 0 && j != 0) {
          addGraphEdge(center, {x_grid_[i - 1], y_grid_[j - 1]});
        }
      }
    }
  }

  std::vector<GridValue> grid_tree;
  for (const auto& [point, vertex] : point_vertex_map_) {
    odb::Rect rect(point, point);
    for (const auto& edge : getVertexEdges(vertex)) {
      rect.merge(odb::Rect(vertex_point_map_[edge.m_source],
                           vertex_point_map_[edge.m_target]));
    }
    grid_tree.emplace_back(rect, vertex);
  }
  vertex_grid_tree_ = GridTree(grid_tree.begin(), grid_tree.end());

  debugPrint(logger_,
             utl::PAD,
             "Router",
             1,
             "Added {} edges to graph",
             boost::num_edges(graph_));
}

bool RDLRouter::isEdgeObstructed(const odb::Point& pt0,
                                 const odb::Point& pt1,
                                 bool use_routes) const
{
  using Line = boost::geometry::model::segment<odb::Point>;
  const Line line(pt0, pt1);
  for (auto itr
       = obstructions_.qbegin(boost::geometry::index::intersects(line));
       itr != obstructions_.qend();
       itr++) {
    const ObsValue& obs = *itr;
    if (boost::geometry::intersects(line, std::get<1>(obs))) {
      return true;
    }
  }
  if (use_routes) {
    for (const auto& route : routes_) {
      if (route->isIntersecting(odb::Line(pt0, pt1), 0)) {
        return true;
      }
    }
  }
  return false;
}

bool RDLRouter::addGraphVertex(const odb::Point& point)
{
  if (point_vertex_map_.find(point) != point_vertex_map_.end()) {
    return false;
  }

  auto idx = boost::add_vertex(graph_);
  debugPrint(logger_,
             utl::PAD,
             "Router_vertex",
             1,
             "Adding point ({}, {}) as vertex {}",
             point.x(),
             point.y(),
             idx);
  point_vertex_map_[point] = idx;
  vertex_point_map_[idx] = point;

  return true;
}

void RDLRouter::removeGraphVertex(const odb::Point& point)
{
  auto find_idx = point_vertex_map_.find(point);
  if (find_idx == point_vertex_map_.end()) {
    return;
  }

  auto idx = find_idx->second;
  debugPrint(logger_,
             utl::PAD,
             "Router_vertex",
             1,
             "Removing point ({}, {}) as vertex {}",
             point.x(),
             point.y(),
             idx);
  point_vertex_map_.erase(point);
  vertex_point_map_.erase(idx);
}

bool RDLRouter::addGraphEdge(const odb::Point& point0,
                             const odb::Point& point1,
                             float edge_weight_scale,
                             bool check_obstructions,
                             bool check_routes)
{
  auto point0check = point_vertex_map_.find(point0);
  if (point0check == point_vertex_map_.end()) {
    debugPrint(logger_,
               utl::PAD,
               "Router_edge",
               1,
               "Failed to find vertex at ({}, {})",
               point0.x(),
               point0.y());
    return false;
  }
  auto point1check = point_vertex_map_.find(point1);
  if (point1check == point_vertex_map_.end()) {
    debugPrint(logger_,
               utl::PAD,
               "Router_edge",
               1,
               "Failed to find vertex at ({}, {})",
               point1.x(),
               point1.y());
    return false;
  }
  GridGraphVertex v0 = point0check->second;
  GridGraphVertex v1 = point1check->second;
  if (v0 == v1) {
    return false;
  }

  if (check_obstructions && isEdgeObstructed(point0, point1, check_routes)) {
    debugPrint(logger_,
               utl::PAD,
               "Router_edge",
               1,
               "Failed to add edge ({}, {}) -> ({}, {}) intersects obstruction",
               point0.x(),
               point0.y(),
               point1.x(),
               point1.y());
    return false;
  }

  bool added;
  GridGraphEdge edge;

  bool exists;
  boost::tie(edge, exists) = boost::lookup_edge(v0, v1, graph_);
  if (exists) {
    return true;
  }
  boost::tie(edge, exists) = boost::lookup_edge(v1, v0, graph_);
  if (exists) {
    return true;
  }

  boost::tie(edge, added) = boost::add_edge(v0, v1, graph_);
  if (!added) {
    return false;
  }

  const int64_t weight = edge_weight_scale * distance(point0, point1);

  debugPrint(logger_,
             utl::PAD,
             "Router_edge",
             1,
             "Adding edge from ({}, {}) to ({}, {}) with weight {}",
             point0.x(),
             point0.y(),
             point1.x(),
             point1.y(),
             weight);
  graph_weight_[edge] = weight;

  return true;
}

std::vector<std::pair<odb::Point, odb::Point>> RDLRouter::simplifyRoute(
    const std::vector<odb::Point>& route) const
{
  std::vector<std::pair<odb::Point, odb::Point>> wire;

  enum class Direction
  {
    kUnset,
    kHorizontal,
    kVertical,
    kAngle45,
    kAngle135
  };

  auto get_direction
      = [](const odb::Point& s, const odb::Point& t) -> Direction {
    if (s.y() == t.y()) {
      return Direction::kHorizontal;
    }
    if (s.x() == t.x()) {
      return Direction::kVertical;
    }
    if (s.x() < t.x() && s.y() < t.y()) {
      return Direction::kAngle45;
    }
    if (s.x() > t.x() && s.y() > t.y()) {
      return Direction::kAngle45;
    }
    return Direction::kAngle135;
  };

  wire.emplace_back(route[0], route[1]);
  Direction direction
      = get_direction(wire.begin()->first, wire.begin()->second);
  for (size_t i = 2; i < route.size(); i++) {
    odb::Point s = wire.rbegin()->second;
    odb::Point t = route[i];

    Direction segment_direction = get_direction(s, t);
    if (direction == segment_direction) {
      // Extend segment
      wire.rbegin()->second = t;
    } else {
      // Determine if extentions are needed
      int extention = width_ / 2;
      if (direction == Direction::kHorizontal
          && segment_direction == Direction::kVertical) {
        const odb::Point& prev_s = wire.rbegin()->first;
        if (prev_s.x() < s.x()) {
          wire.rbegin()->second.setX(s.x() + extention);
        } else {
          wire.rbegin()->second.setX(s.x() - extention);
        }
        if (s.y() < t.y()) {
          s.setY(s.y() - extention);
        } else {
          s.setY(s.y() + extention);
        }
      } else if (direction == Direction::kVertical
                 && segment_direction == Direction::kHorizontal) {
        const odb::Point& prev_s = wire.rbegin()->first;
        if (prev_s.y() < s.y()) {
          wire.rbegin()->second.setY(s.y() + extention);
        } else {
          wire.rbegin()->second.setY(s.y() - extention);
        }
        if (s.x() < t.x()) {
          s.setX(s.x() - extention);
        } else {
          s.setX(s.x() + extention);
        }
      }

      // Start new segment
      wire.emplace_back(s, t);

      direction = segment_direction;
    }
  }

  return wire;
}

odb::Rect RDLRouter::correctEndPoint(const odb::Rect& route,
                                     const bool is_horizontal,
                                     const odb::Rect& target) const
{
  const int route_width = is_horizontal ? route.dy() : route.dx();
  const int target_width = is_horizontal ? target.dy() : target.dx();

  if (route_width <= target_width) {
    // shape is already fully covered
    return route;
  }

  odb::Rect new_route = route;
  new_route.merge(target);

  return new_route;
}

void RDLRouter::writeToDb(odb::dbNet* net,
                          const std::vector<odb::Point>& route,
                          const RouteTarget* source,
                          const RouteTarget* target,
                          const std::set<odb::Rect>& stubs)
{
  Utilities::makeSpecial(net);

  if (source == nullptr && target == nullptr && stubs.empty()) {
    // Nothing to create a wire for, so return
    return;
  }

  odb::dbSWire* swire = odb::dbSWire::create(net, odb::dbWireType::ROUTED);
  for (const odb::Rect& stub : stubs) {
    odb::dbSBox::create(swire,
                        layer_,
                        stub.xMin(),
                        stub.yMin(),
                        stub.xMax(),
                        stub.yMax(),
                        odb::dbWireShapeType::IOWIRE);
  }
  if (source != nullptr && target != nullptr) {
    const auto simplified_route = simplifyRoute(route);
    for (size_t i = 0; i < simplified_route.size(); i++) {
      const auto& [s, t] = simplified_route[i];
      odb::Rect shape(s, t);
      shape.bloat(width_ / 2, shape);
      odb::dbSBox::Direction dir;
      if (s.x() == t.x()) {
        shape.set_ylo(shape.yMin() + (width_ / 2));
        shape.set_yhi(shape.yMax() - (width_ / 2));
        dir = odb::dbSBox::VERTICAL;
      } else if (s.y() == t.y()) {
        shape.set_xlo(shape.xMin() + (width_ / 2));
        shape.set_xhi(shape.xMax() - (width_ / 2));
        dir = odb::dbSBox::HORIZONTAL;
      } else {
        dir = odb::dbSBox::OCTILINEAR;
      }

      if (dir != odb::dbSBox::OCTILINEAR) {
        if (i == 0) {
          shape = correctEndPoint(shape, s.y() == t.y(), source->shape);
        } else if (i + 1 == simplified_route.size()) {
          shape = correctEndPoint(shape, s.y() == t.y(), target->shape);
        }
      }

      if (dir != odb::dbSBox::OCTILINEAR) {
        odb::dbSBox::create(swire,
                            layer_,
                            shape.xMin(),
                            shape.yMin(),
                            shape.xMax(),
                            shape.yMax(),
                            odb::dbWireShapeType::IOWIRE);
      } else {
        odb::dbSBox::create(swire,
                            layer_,
                            s.x(),
                            s.y(),
                            t.x(),
                            t.y(),
                            odb::dbWireShapeType::IOWIRE,
                            odb::dbSBox::OCTILINEAR,
                            width_);
      }
    }

    if (source->layer != layer_) {
      odb::dbTechVia* via = pad_accessvia_;
      if (isCoverTerm(source->terminal)) {
        via = bump_accessvia_;
      }
      odb::dbSBox::create(swire,
                          via,
                          source->center.x(),
                          source->center.y(),
                          odb::dbWireShapeType::IOWIRE);
    }
    if (target->layer != layer_) {
      odb::dbTechVia* via = pad_accessvia_;
      if (isCoverTerm(target->terminal)) {
        via = bump_accessvia_;
      }
      odb::dbSBox::create(swire,
                          via,
                          target->center.x(),
                          target->center.y(),
                          odb::dbWireShapeType::IOWIRE);
    }
  }
}

int RDLRouter::getBloatFactor() const
{
  return width_ / 2 + spacing_;
}

std::set<odb::Polygon> RDLRouter::getITermShapes(odb::dbITerm* iterm) const
{
  std::set<odb::Polygon> polys;

  const odb::dbTransform xform = iterm->getInst()->getTransform();

  for (auto* mpin : iterm->getMTerm()->getMPins()) {
    for (auto* geom : mpin->getPolygonGeometry()) {
      if (geom->getTechLayer() != layer_) {
        continue;
      }

      odb::Polygon poly = geom->getPolygon();
      xform.apply(poly);
      polys.insert(poly);
    }
    for (auto* geom : mpin->getGeometry(false)) {
      if (geom->getTechLayer() != layer_) {
        continue;
      }

      odb::Rect rect = geom->getBox();
      xform.apply(rect);
      polys.insert(rect);
    }
  }

  return polys;
}

void RDLRouter::populateObstructions(const std::vector<odb::dbNet*>& nets)
{
  std::vector<ObsValue> obstructions;

  const int bloat = getBloatFactor();
  auto insert_obstruction_rect
      = [&obstructions, bloat](
            const odb::Rect& rect, odb::dbNet* net, odb::dbObject* src) {
          odb::Rect bloated;
          rect.bloat(bloat, bloated);

          obstructions.emplace_back(bloated, bloated, net, src);
        };
  auto insert_obstruction_oct
      = [&obstructions, bloat](
            const odb::Oct& oct, odb::dbNet* net, odb::dbObject* src) {
          const odb::Oct bloat_oct = oct.bloat(bloat);

          obstructions.emplace_back(
              bloat_oct.getEnclosingRect(), bloat_oct, net, src);
        };
  auto insert_obstruction_poly
      = [&obstructions, bloat](
            const odb::Polygon& poly, odb::dbNet* net, odb::dbObject* src) {
          const odb::Polygon bloat_poly = poly.bloat(bloat);

          obstructions.emplace_back(
              bloat_poly.getEnclosingRect(), bloat_poly, net, src);
        };

  // Get placed instanced obstructions
  for (auto* inst : block_->getInsts()) {
    if (!inst->isPlaced()) {
      continue;
    }

    const odb::dbTransform xform = inst->getTransform();

    auto* master = inst->getMaster();
    for (auto* obs : master->getPolygonObstructions()) {
      if (obs->getTechLayer() != layer_) {
        continue;
      }

      odb::Polygon poly = obs->getPolygon();
      xform.apply(poly);
      insert_obstruction_poly(poly, nullptr, nullptr);
    }
    for (auto* obs : master->getObstructions(false)) {
      if (obs->getTechLayer() != layer_) {
        continue;
      }

      odb::Rect rect = obs->getBox();
      xform.apply(rect);
      insert_obstruction_rect(rect, nullptr, nullptr);
    }

    for (auto* iterm : inst->getITerms()) {
      auto* net = iterm->getNet();
      for (const auto& poly : getITermShapes(iterm)) {
        insert_obstruction_poly(poly, net, iterm);
      }
    }
  }

  // Get already routed nets obstructions, excluding those that will be routed
  // now
  for (auto* net : block_->getNets()) {
    const bool is_routing_net = std::ranges::find(nets, net) != nets.end();

    for (auto* swire : net->getSWires()) {
      if (is_routing_net && swire->getWireType() != odb::dbWireType::FIXED) {
        continue;
      }

      for (auto* box : swire->getWires()) {
        if (box->getTechLayer() != layer_) {
          continue;
        }

        if (box->getDirection() == odb::dbSBox::OCTILINEAR) {
          insert_obstruction_oct(box->getOct(), net, nullptr);
        } else {
          insert_obstruction_rect(box->getBox(), net, nullptr);
        }
      }
    }
  }

  // Get routing obstructions
  for (auto* obs : block_->getObstructions()) {
    auto* box = obs->getBBox();
    if (box->getTechLayer() != layer_) {
      continue;
    }

    insert_obstruction_rect(box->getBox(), nullptr, nullptr);
  }

  // Add via obstructions when using access vias
  for (const auto& [net, routing_pairs] : routing_targets_) {
    for (const auto& [iterm, targets] : routing_pairs) {
      for (const auto& target : targets) {
        if (isCoverTerm(target.terminal) && bump_accessvia_ != nullptr) {
          insert_obstruction_rect(target.shape, net, iterm);
        } else if (!isCoverTerm(target.terminal) && pad_accessvia_ != nullptr) {
          insert_obstruction_rect(target.shape, net, iterm);
        }
      }
    }
  }

  obstructions_ = ObsTree(obstructions.begin(), obstructions.end());
}

int64_t RDLRouter::distance(const odb::Point& p0, const odb::Point& p1)
{
  const int64_t dx = p0.x() - p1.x();
  const int64_t dy = p0.y() - p1.y();
  return std::sqrt(dx * dx + dy * dy);
}

int64_t RDLRouter::distance(const TargetPair& pair)
{
  return distance(pair.target0->center, pair.target1->center);
}

bool RDLRouter::isCoverTerm(odb::dbITerm* term)
{
  return term->getMTerm()->getMaster()->getType().isCover();
}

odb::dbTechLayer* RDLRouter::getOtherLayer(odb::dbTechVia* via) const
{
  if (via != nullptr) {
    if (via->getBottomLayer() != layer_) {
      return via->getBottomLayer();
    }
    if (via->getTopLayer() != layer_) {
      return via->getTopLayer();
    }
  }
  return nullptr;
}

std::map<odb::dbITerm*, std::vector<RouteTarget>>
RDLRouter::generateRoutingTargets(odb::dbNet* net) const
{
  std::map<odb::dbITerm*, std::vector<RouteTarget>> targets;
  odb::dbTechLayer* bump_pin_layer = getOtherLayer(bump_accessvia_);
  odb::dbTechLayer* pad_pin_layer = getOtherLayer(pad_accessvia_);

  for (auto* iterm : net->getITerms()) {
    if (!iterm->getInst()->isPlaced()) {
      continue;
    }

    auto* prop = odb::dbBoolProperty::find(iterm, kRouteProperty);
    if (prop && !prop->getValue()) {
      debugPrint(logger_,
                 utl::PAD,
                 "Router",
                 2,
                 "Skipping termininal on {}: {}",
                 net->getName(),
                 iterm->getName());
      continue;
    }

    odb::dbTechLayer* other_layer;
    odb::dbTechVia* via;
    if (isCoverTerm(iterm)) {
      other_layer = bump_pin_layer;
      via = bump_accessvia_;
    } else {
      other_layer = pad_pin_layer;
      via = pad_accessvia_;
    }

    const odb::dbTransform xform = iterm->getInst()->getTransform();

    for (auto* mpin : iterm->getMTerm()->getMPins()) {
      for (auto* geom : mpin->getPolygonGeometry()) {
        odb::dbTechLayer* found_layer = geom->getTechLayer();
        if (found_layer != layer_ && found_layer != other_layer) {
          continue;
        }

        bool use_via = false;
        odb::Polygon box = geom->getPolygon();
        const odb::Rect box_rect = box.getEnclosingRect();
        if (found_layer == other_layer) {
          for (const auto& viabox : via->getBoxes()) {
            if (viabox->getTechLayer() == other_layer) {
              odb::Rect via_encl = viabox->getBox();
              via_encl.moveDelta(box_rect.xCenter(), box_rect.yCenter());
              box = via_encl;
              use_via = true;
              break;
            }
          }
        }
        xform.apply(box);

        if (use_via) {
          const odb::Rect via_rect = box.getEnclosingRect();
          targets[iterm].push_back(
              {via_rect.center(), via_rect, iterm, found_layer, {}});
        } else {
          // find rectangles that make suitable targets
          const odb::Polygon small_poly = box.bloat(-width_ / 2);
          const auto points = small_poly.getPoints();

          auto make_rect = [this](const odb::Point& pt0,
                                  const odb::Point& pt1) -> odb::Rect {
            const odb::Point center((pt0.x() + pt1.x()) / 2,
                                    (pt0.y() + pt1.y()) / 2);
            const odb::Rect rect(center.x() - width_ / 4,
                                 center.y() - width_ / 4,
                                 center.x() + width_ / 4,
                                 center.y() + width_ / 4);

            return rect;
          };

          // first try and add only rects that abut the 90degree egdes
          bool targets_added = false;
          for (std::size_t i = 1; i < points.size(); i++) {
            const auto& pt0 = points[i - 1];
            const auto& pt1 = points[i];
            if (pt0.x() == pt1.x() || pt0.y() == pt1.y()) {
              const odb::Rect rect = make_rect(pt0, pt1);
              targets[iterm].push_back(
                  {rect.center(), rect, iterm, found_layer, {}});
              targets_added = true;
            }
          }

          if (!targets_added) {
            // go ahead and add all if no targets could be added
            for (std::size_t i = 1; i < points.size(); i++) {
              const auto& pt0 = points[i - 1];
              const auto& pt1 = points[i];
              const odb::Rect rect = make_rect(pt0, pt1);
              targets[iterm].push_back(
                  {rect.center(), rect, iterm, found_layer, {}});
            }
          }
        }
      }
      for (auto* geom : mpin->getGeometry(false)) {
        odb::dbTechLayer* found_layer = geom->getTechLayer();
        if (found_layer != layer_ && found_layer != other_layer) {
          continue;
        }

        odb::Rect box = geom->getBox();
        if (found_layer == other_layer) {
          for (const auto& viabox : via->getBoxes()) {
            if (viabox->getTechLayer() == other_layer) {
              odb::Rect via_encl = viabox->getBox();
              via_encl.moveDelta(box.xCenter(), box.yCenter());
              box = via_encl;
              break;
            }
          }
        }
        xform.apply(box);

        targets[iterm].push_back({box.center(), box, iterm, found_layer, {}});
      }
    }
  }

  if (targets.size() < 2) {
    logger_->warn(utl::PAD,
                  10,
                  "{} only has one iterm on {} layer",
                  net->getName(),
                  layer_->getName());
  }

  debugPrint(logger_,
             utl::PAD,
             "Router",
             1,
             "{} has {} targets",
             net->getName(),
             targets.size());

  return targets;
}

bool RDLRouter::isDebugNet(odb::dbNet* net) const
{
  if (debug_net_ == nullptr) {
    return false;
  }

  return net == debug_net_;
}

bool RDLRouter::isDebugPin(odb::dbITerm* pin) const
{
  if (debug_pin_ == nullptr) {
    return false;
  }

  return pin == debug_pin_;
}

}  // namespace pad
