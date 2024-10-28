///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2024, The Regents of the University of California
// All rights reserved.
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

#pragma once

#include <boost/geometry.hpp>
#include <boost/polygon/polygon.hpp>
#include <map>
#include <memory>
#include <optional>
#include <vector>

#include "connection.h"
#include "node.h"
#include "odb/db.h"
#include "psm/pdnsim.h"
#include "utl/Logger.h"

namespace psm {

template <typename T>
struct RectIndexableGetter
{
  using result_type = odb::Rect;
  odb::Rect operator()(const T* t) const { return t->getShape(); }
};

template <typename T>
struct PointIndexableGetter
{
  using result_type = odb::Point;
  odb::Point operator()(const T* t) const { return t->getPoint(); }
};

class IRNetwork
{
 public:
  template <typename T>
  using NodePtrMap = std::map<Node*, std::vector<T*>>;

  template <typename T>
  using LayerMap = std::map<odb::dbTechLayer*, T>;

  using Point
      = boost::geometry::model::d2::point_xy<int,
                                             boost::geometry::cs::cartesian>;

  using TerminalTree
      = boost::geometry::index::rtree<TerminalNode*,
                                      boost::geometry::index::quadratic<16>,
                                      RectIndexableGetter<TerminalNode>>;

  using ShapeTree
      = boost::geometry::index::rtree<Shape*,
                                      boost::geometry::index::quadratic<16>,
                                      RectIndexableGetter<Shape>>;

  using NodeTree
      = boost::geometry::index::rtree<Node*,
                                      boost::geometry::index::quadratic<16>,
                                      PointIndexableGetter<Node>>;
  using Polygon90 = boost::polygon::polygon_90_with_holes_data<int>;
  using Polygon90Set = boost::polygon::polygon_90_set_data<int>;

  IRNetwork(odb::dbNet* net, utl::Logger* logger, bool floorplanning);

  odb::dbNet* getNet() const { return net_; };

  void construct();

  bool isFloorplanningOnly() const { return floorplanning_; }
  void setFloorplanning(bool value) { floorplanning_ = value; }

  void reportStats() const;

  bool belongsTo(Node* node) const;
  bool belongsTo(Connection* connection) const;

  const LayerMap<std::vector<std::unique_ptr<Shape>>>& getShapes() const
  {
    return shapes_;
  }

  const LayerMap<std::vector<std::unique_ptr<Node>>>& getNodes() const
  {
    return nodes_;
  }
  const std::vector<std::unique_ptr<ITermNode>>& getITermNodes() const
  {
    return iterm_nodes_;
  }
  const std::vector<std::unique_ptr<BPinNode>>& getBPinNodes() const
  {
    return bpin_nodes_;
  }
  Node::NodeSet getBPinShapeNodes() const;

  odb::dbTechLayer* getTopLayer() const;
  const std::vector<std::unique_ptr<Node>>& getTopLayerNodes() const;
  NodeTree getTopLayerNodeTree() const;

  std::size_t getNodeCount(bool include_iterms = false) const;

  const std::vector<std::unique_ptr<Connection>>& getConnections() const
  {
    return connections_;
  }
  NodePtrMap<Connection> getConnectionMap() const;

  std::map<odb::dbInst*, Node::NodeSet> getInstanceNodeMapping() const;

  // For debug only
  void dumpNodes(const std::map<Node*, std::size_t>& node_map,
                 const std::string& name = "nodes") const;
  void dumpNodes(const std::string& name = "nodes") const;

 private:
  odb::dbBlock* getBlock() const;
  odb::dbTech* getTech() const;

  void addStrapNodes(odb::dbSBox* box);
  void addViaNodes(odb::dbSBox* box);

  void sortShapes();

  void generateRoutingLayerShapesAndNodes();
  void generateCutLayerNodes();
  void generateTopLayerFillerNodes();
  void sortNodes();
  void cleanupNodes();
  void cleanupOverlappingNodes(NodePtrMap<Connection>& connection_map);
  void mergeNodes(NodePtrMap<Connection>& connection_map);

  void connectLayerNodes();
  void cleanupConnections();
  void cleanupInvalidConnections();
  void cleanupDuplicateConnections();
  void sortConnections();

  void removeNodes(std::set<Node*>& removes,
                   odb::dbTechLayer* layer,
                   std::vector<std::unique_ptr<Node>>& nodes,
                   const NodePtrMap<Connection>& connection_map);
  void removeConnections(std::set<Connection*>& removes);

  int getEffectiveNumberOfCuts(const odb::dbShape& shape) const;

  void copy(Node* keep, Node* remove, NodePtrMap<Connection>& connection_map);

  std::set<Node*> getSharedShapeNodes() const;

  Polygon90 rectToPolygon(const odb::Rect& rect) const;
  LayerMap<Polygon90Set> generatePolygonsFromSWire(odb::dbSWire* wire);
  LayerMap<Polygon90Set> generatePolygonsFromITerms(
      std::vector<TerminalNode*>& terminals);
  LayerMap<Polygon90Set> generatePolygonsFromBTerms(
      std::vector<TerminalNode*>& terminals);
  void processPolygonToRectangles(
      odb::dbTechLayer* layer,
      const Polygon90& polygon,
      const TerminalTree& terminals,
      std::vector<std::unique_ptr<Shape>>& new_shapes,
      std::vector<std::unique_ptr<Node>>& new_nodes,
      std::map<Shape*, std::set<Node*>>& terminal_connections);
  void generateCutNodesForSBox(
      odb::dbSBox* box,
      bool single_via,
      std::vector<std::unique_ptr<Node>>& new_nodes,
      std::vector<std::unique_ptr<Connection>>& new_connections);
  LayerMap<Polygon90Set> generatePolygonsFromBox(
      odb::dbBox* box,
      const odb::dbTransform& transform) const;

  TerminalTree getTerminalTree(
      const std::vector<TerminalNode*>& terminals) const;
  ShapeTree getShapeTree(odb::dbTechLayer* layer) const;
  NodeTree getNodeTree(odb::dbTechLayer* layer) const;

  void initMinimumNodePitch();

  void recoverMemory();

  void reset();

  odb::dbNet* net_;

  utl::Logger* logger_;

  bool floorplanning_;

  LayerMap<std::vector<std::unique_ptr<Shape>>> shapes_;
  LayerMap<std::vector<std::unique_ptr<Node>>> nodes_;

  std::vector<std::unique_ptr<Connection>> connections_;

  std::vector<std::unique_ptr<ITermNode>> iterm_nodes_;
  std::vector<std::unique_ptr<BPinNode>> bpin_nodes_;

  std::map<odb::dbTechLayer*, int> min_node_pitch_;

  static constexpr int min_node_pitch_multiplier_ = 10;
  static constexpr double min_node_pitch_um_ = 10.0;
};

}  // namespace psm
