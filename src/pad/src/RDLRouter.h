/////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2023, The Regents of the University of California
// All rights reserved.
//
// BSD 3-Clause License
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <boost/geometry.hpp>
#include <boost/geometry/index/rtree.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <map>
#include <memory>
#include <vector>

#include "gui/gui.h"
#include "odb/geom.h"

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

class RDLGui : public gui::Renderer
{
 public:
  RDLGui();

  void setRouter(RDLRouter* router) { router_ = router; }

  void drawObjects(gui::Painter& painter) override;

  const char* getDisplayControlGroupName() override { return "RDL Router"; }

 private:
  RDLRouter* router_ = nullptr;

  static constexpr const char* draw_vertex_ = "Vertices";
  static constexpr const char* draw_edge_ = "Edges";
  static constexpr const char* draw_obs_ = "Obstructions";
};

class RDLRouter
{
 public:
  struct RouteTarget
  {
    // center point of the target shape
    odb::Point center;
    odb::Rect shape;
    odb::dbITerm* terminal;
    odb::dbTechLayer* layer;
  };
  struct TargetPair
  {
    RouteTarget target0;
    RouteTarget target1;
  };
  struct Edge
  {
    odb::Point pt0;
    odb::Point pt1;
  };

  RDLRouter(utl::Logger* logger,
            odb::dbBlock* block,
            odb::dbTechLayer* layer,
            odb::dbTechVia* bump_via,
            odb::dbTechVia* pad_via,
            const std::map<odb::dbITerm*, odb::dbITerm*>& routing_map,
            int width,
            int spacing,
            bool allow45,
            float turn_penalty);

  void route(const std::vector<odb::dbNet*>& nets);

  static int64_t distance(const odb::Point& p0, const odb::Point& p1);

  using Point
      = boost::geometry::model::d2::point_xy<int,
                                             boost::geometry::cs::cartesian>;
  using Box = boost::geometry::model::box<Point>;
  using ObsValue = std::pair<Box, odb::dbNet*>;
  using ObsTree
      = boost::geometry::index::rtree<ObsValue,
                                      boost::geometry::index::quadratic<16>>;

  using GridGraph
      = boost::adjacency_list<boost::listS,
                              boost::vecS,
                              boost::undirectedS,
                              boost::no_property,
                              boost::property<boost::edge_weight_t, int64_t>>;

  using GridWeightMap
      = boost::property_map<GridGraph, boost::edge_weight_t>::type;
  using grid_vertex = GridGraph::vertex_descriptor;
  using grid_edge = GridGraph::edge_descriptor;

  const GridGraph& getGraph() const { return graph_; };
  const std::map<grid_vertex, odb::Point>& getVertexMap() const
  {
    return vertex_point_map_;
  }
  const ObsTree& getObstructions() const { return obstructions_; }

 private:
  void makeGraph();
  void addGraphVertex(const odb::Point& point);
  bool addGraphEdge(const odb::Point& point0,
                    const odb::Point& point1,
                    float edge_weight_scale = 1.0,
                    bool check_obstructions = true);

  std::vector<grid_vertex> run(const odb::Point& source,
                               const odb::Point& dest);
  std::set<std::pair<odb::Point, odb::Point>> commitRoute(
      const std::vector<grid_vertex>& route);
  void uncommitRoute(const std::set<std::pair<odb::Point, odb::Point>>& route);

  void writeToDb(odb::dbNet* net,
                 const std::vector<grid_vertex>& route,
                 const RouteTarget& source,
                 const RouteTarget& target);
  std::vector<std::pair<odb::Point, odb::Point>> simplifyRoute(
      const std::vector<grid_vertex>& route) const;
  odb::Rect correctEndPoint(const odb::Rect& route,
                            bool is_horizontal,
                            const odb::Rect& target) const;

  std::set<odb::Rect> getITermShapes(odb::dbITerm* iterm) const;
  void populateObstructions(const std::vector<odb::dbNet*>& nets);
  bool isObstructed(const odb::Point& pt) const;

  std::vector<Edge> insertTerminalVertex(const RouteTarget& target,
                                         const RouteTarget& source);

  std::vector<TargetPair> generateRoutingPairs(odb::dbNet* net) const;

  int getBloatFactor() const;

  utl::Logger* logger_;
  odb::dbBlock* block_;
  odb::dbTechLayer* layer_;
  odb::dbTechVia* bump_accessvia_;
  odb::dbTechVia* pad_accessvia_;

  int width_;
  int spacing_;
  bool allow45_;
  float turn_penalty_;

  const std::map<odb::dbITerm*, odb::dbITerm*>& routing_map_;

  GridGraph graph_;
  GridWeightMap graph_weight_;
  ObsTree obstructions_;

  // Lookup tables
  std::map<odb::Point, grid_vertex> point_vertex_map_;
  std::map<grid_vertex, odb::Point> vertex_point_map_;
  std::map<odb::dbITerm*, std::vector<Edge>> iterm_edges_;

  // Routing grid
  std::vector<int> x_grid_;
  std::vector<int> y_grid_;
  std::map<odb::dbNet*, std::vector<TargetPair>> routing_terminals_;
};

}  // namespace pad
