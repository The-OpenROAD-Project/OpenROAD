// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2025, The OpenROAD Authors

#pragma once

#include <cstdint>
#include <map>
#include <memory>
#include <set>
#include <tuple>
#include <utility>
#include <vector>

#include "boost/geometry/geometry.hpp"
#include "boost/geometry/index/rtree.hpp"
#include "boost/graph/adjacency_list.hpp"
#include "gui/gui.h"
#include "odb/db.h"
#include "odb/dbObject.h"
#include "odb/geom.h"
#include "odb/geom_boost.h"

namespace odb {
class dbBlock;
class dbITerm;
class dbTechLayer;
class dbTechVia;
class dbNet;
}  // namespace odb

namespace utl {
class Logger;
}

namespace pad {

class RDLRouter;

using GridGraph
    = boost::adjacency_list<boost::listS,
                            boost::vecS,
                            boost::undirectedS,
                            boost::no_property,
                            boost::property<boost::edge_weight_t, int64_t>>;

using GridWeightMap
    = boost::property_map<GridGraph, boost::edge_weight_t>::type;
using GridGraphVertex = GridGraph::vertex_descriptor;
using GridGraphEdge = GridGraph::edge_descriptor;

struct RouteTarget
{
  // center point of the target shape
  odb::Point center;
  odb::Rect shape;
  odb::dbITerm* terminal;
  odb::dbTechLayer* layer;
  std::set<odb::Point> grid_access;
};

class RDLGui;
class RDLRoute;

class RDLRouter
{
 public:
  struct TargetPair
  {
    const RouteTarget* target0;
    const RouteTarget* target1;
  };
  struct Edge
  {
    odb::Point pt0;
    odb::Point pt1;
  };
  struct GridEdge
  {
    odb::Point source;
    odb::Point target;
    float weight;
  };
  struct NetRoute
  {
    std::vector<GridGraphVertex> route;
    std::vector<GridEdge> removed_edges;
    const RouteTarget* source;
    const RouteTarget* target;
  };
  struct ITermRouteOrder
  {
    std::vector<odb::dbITerm*> terminals;
    std::vector<odb::dbITerm*>::iterator next;
  };
  struct TerminalAccess
  {
    std::vector<GridEdge> removed_edges;
    std::vector<Edge> added_edges;
    std::set<odb::Point> added_points;
  };

  using NetRoutingTargetMap
      = std::map<odb::dbNet*,
                 std::map<odb::dbITerm*, std::vector<RouteTarget>>>;

  using RDLRoutePtr = std::shared_ptr<RDLRoute>;

  RDLRouter(utl::Logger* logger,
            odb::dbBlock* block,
            odb::dbTechLayer* layer,
            odb::dbTechVia* bump_via,
            odb::dbTechVia* pad_via,
            const std::map<odb::dbITerm*, odb::dbITerm*>& routing_map,
            int width,
            int spacing,
            bool allow45,
            float turn_penalty,
            int max_iterations);
  ~RDLRouter();

  void route(const std::vector<odb::dbNet*>& nets);

  static int64_t distance(const odb::Point& p0, const odb::Point& p1);
  static int64_t distance(const TargetPair& pair);

  using ObsValue
      = std::tuple<odb::Rect, odb::Polygon, odb::dbNet*, odb::dbObject*>;
  using ObsTree
      = boost::geometry::index::rtree<ObsValue,
                                      boost::geometry::index::quadratic<16>>;

  using GridValue = std::pair<odb::Rect, GridGraphVertex>;
  using GridTree
      = boost::geometry::index::rtree<GridValue,
                                      boost::geometry::index::quadratic<16>>;

  const GridGraph& getGraph() const { return graph_; };
  const std::map<GridGraphVertex, odb::Point>& getVertexMap() const
  {
    return vertex_point_map_;
  }
  const ObsTree& getObstructions() const { return obstructions_; }
  const NetRoutingTargetMap& getRoutingTargets() const
  {
    return routing_targets_;
  }
  const std::vector<RDLRoutePtr>& getRoutes() const { return routes_; }
  std::vector<RDLRoutePtr> getFailedRoutes() const;

  void setRDLGui(RDLGui* gui) { gui_ = gui; }
  void setRDLDebugNet(odb::dbNet* net) { debug_net_ = net; }
  void setRDLDebugPin(odb::dbITerm* term) { debug_pin_ = term; }

  odb::Rect getPointObstruction(const odb::Point& pt) const;
  odb::Polygon getEdgeObstruction(const odb::Point& pt0,
                                  const odb::Point& pt1) const;
  bool is45DegreeEdge(const odb::Point& pt0, const odb::Point& pt1) const;

  static bool isCoverTerm(odb::dbITerm* term);

 private:
  void makeGraph();
  bool addGraphVertex(const odb::Point& point);
  void removeGraphVertex(const odb::Point& point);
  bool addGraphEdge(const odb::Point& point0,
                    const odb::Point& point1,
                    float edge_weight_scale = 1.0,
                    bool check_obstructions = true,
                    bool check_routes = true);
  GridEdge removeGraphEdge(const GridGraphEdge& edge);

  std::vector<GridGraphVertex> run(const odb::Point& source,
                                   const odb::Point& dest);
  std::vector<GridEdge> commitRoute(const std::vector<GridGraphVertex>& route);
  void uncommitRoute(const std::vector<GridEdge>& route);

  void writeToDb(odb::dbNet* net,
                 const std::vector<odb::Point>& route,
                 const RouteTarget* source,
                 const RouteTarget* target,
                 const std::set<odb::Rect>& stubs);
  std::vector<std::pair<odb::Point, odb::Point>> simplifyRoute(
      const std::vector<odb::Point>& route) const;
  odb::Rect correctEndPoint(const odb::Rect& route,
                            bool is_horizontal,
                            const odb::Rect& target) const;

  std::set<odb::Polygon> getITermShapes(odb::dbITerm* iterm) const;
  void populateObstructions(const std::vector<odb::dbNet*>& nets);
  bool isEdgeObstructed(const odb::Point& pt0,
                        const odb::Point& pt1,
                        bool use_routes) const;

  void populateTerminalAccessPoints(RouteTarget& target) const;
  std::set<odb::Point> generateTerminalAccessPoints(const odb::Point& pt,
                                                    bool do_x) const;
  TerminalAccess insertTerminalAccess(const RouteTarget& target,
                                      const RouteTarget& source);
  void removeTerminalAccess(const TerminalAccess& access);

  std::map<odb::dbITerm*, std::vector<RouteTarget>> generateRoutingTargets(
      odb::dbNet* net) const;
  odb::dbTechLayer* getOtherLayer(odb::dbTechVia* via) const;
  std::set<GridGraphEdge> getVertexEdges(const GridGraphVertex& vertex) const;

  void buildIntialRouteSet();
  int reportFailedRoutes(
      const std::map<odb::dbITerm*, odb::dbITerm*>& routed_pairs) const;
  std::set<odb::dbInst*> getRoutedInstances() const;
  int getRoutingInstanceCount() const;

  int getBloatFactor() const;
  bool isDebugNet(odb::dbNet* net) const;
  bool isDebugPin(odb::dbITerm* pin) const;

  utl::Logger* logger_;
  odb::dbBlock* block_;
  odb::dbTechLayer* layer_;
  odb::dbTechVia* bump_accessvia_;
  odb::dbTechVia* pad_accessvia_;

  int width_;
  int spacing_;
  bool allow45_;
  float turn_penalty_;
  int max_router_iterations_;

  const std::map<odb::dbITerm*, odb::dbITerm*>& routing_map_;

  GridGraph graph_;
  GridWeightMap graph_weight_;
  ObsTree obstructions_;

  // Lookup tables
  std::map<odb::Point, GridGraphVertex> point_vertex_map_;
  GridTree vertex_grid_tree_;
  std::map<GridGraphVertex, odb::Point> vertex_point_map_;
  std::map<odb::dbITerm*, std::vector<Edge>> iterm_edges_;

  // Routing grid
  std::vector<int> x_grid_;
  std::vector<int> y_grid_;

  // Routing information
  NetRoutingTargetMap routing_targets_;
  std::vector<RDLRoutePtr> routes_;

  // Debugging
  RDLGui* gui_;
  odb::dbNet* debug_net_{nullptr};
  odb::dbITerm* debug_pin_{nullptr};

  // Consts
  static constexpr const char* kRouteProperty = "RDL_ROUTE";
};

}  // namespace pad
