// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024-2025, The OpenROAD Authors

#pragma once

#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "boost/geometry/geometry.hpp"
#include "connection.h"
#include "node.h"
#include "odb/PtrSetMap.h"
#include "odb/db.h"
#include "odb/geom.h"
#include "odb/geom_boost.h"
#include "psm/pdnsim.h"
#include "utl/Logger.h"

namespace psm {

template <typename T>
struct RectIndexableGetter
{
  using result_type = odb::Rect;  // NOLINT(readability-identifier-naming)
  odb::Rect operator()(const T* t) const { return t->getShape(); }
};

template <>
struct RectIndexableGetter<TerminalNode>
{
  using result_type = odb::Rect;  // NOLINT(readability-identifier-naming)
  odb::Rect operator()(const TerminalNode* t) const { return t->getBBox(); }
};

template <typename T>
struct PointIndexableGetter
{
  using result_type = odb::Point;  // NOLINT(readability-identifier-naming)
  odb::Point operator()(const T* t) const { return t->getPoint(); }
};

class IRNetwork
{
 public:
  using Nodes = std::vector<Node*>;
  template <typename T>
  using NodePtrMap = std::map<Node*, std::vector<T*>>;

  template <typename T>
  using LayerMap = odb::PtrMap<odb::dbTechLayer, T>;

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

  IRNetwork(odb::dbNet* net, utl::Logger* logger, bool floorplanning);
  IRNetwork(odb::dbChipNet* chip_net,
            utl::Logger* logger,
            bool floorplanning = false,
            std::optional<double> interchip_resistance = std::nullopt,
            bool connectivity_only = false);
  ~IRNetwork();

  odb::dbNet* getNet() const { return net_; };
  odb::dbChipNet* getChipNet() const { return chip_net_; }
  bool isAssembly() const { return chip_net_ != nullptr; }

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
  odb::PtrSet<odb::dbTechLayer> getLayers() const;

  std::size_t getNodeCount(bool include_iterms = false) const;
  std::size_t getConnectionCount() const;
  std::size_t getChipletNetworkCount() const
  {
    return chiplet_networks_.size();
  }
  std::size_t getInterDieConnectionCount() const
  {
    return interdie_connections_.size();
  }

  void clearVisitedNodes();

  const Connections& getConnections() const { return connections_; }
  Nodes getAllNodes() const;
  std::vector<Connection*> getAllConnections() const;
  const Connections& getInterDieConnections() const
  {
    return interdie_connections_;
  }
  NodePtrMap<Connection> getConnectionMap() const;

  odb::PtrMap<odb::dbInst, Node::NodeSet> getInstanceNodeMapping() const;
  odb::PtrMap<odb::dbInst, Node::NodeSet> getInstanceNodeMapping(
      odb::dbNet* net) const;
  std::vector<odb::dbNet*> getNets() const;
  std::vector<std::pair<odb::dbChipInst*, odb::dbNet*>> getChipletNets() const;
  IRNetwork* getChipletNetwork(odb::dbChipInst* chip_inst) const;
  std::vector<std::pair<odb::dbChipInst*, ITermNode*>> getChipletITermNodes()
      const;
  std::map<Node*, odb::dbChipInst*> getNodeChiplets() const;
  bool hasSameBonds() const;
  bool isConnected() const;
  Nodes getDisconnectedNodes() const;
  ShapeTree getShapeTree(odb::dbTechLayer* layer) const;

  bool hasNodes() const;

  // For debug only
  void dumpNodes(const std::map<Node*, std::size_t>& node_map,
                 const std::string& name = "nodes") const;
  void dumpNodes(const std::string& name = "nodes") const;

 private:
  struct BondState
  {
    uint32_t id;
    uint32_t source_id;
    uint32_t target_id;
    uint32_t source_bump_id;
    uint32_t target_bump_id;
    float resistance;

    bool operator==(const BondState&) const = default;
  };

  struct ChipletNetwork
  {
    odb::dbChipInst* chip_inst;
    odb::dbNet* net;
    std::unique_ptr<IRNetwork> network;
  };

  // Shapes with 45 degree edges, which the rectilinear (polygon_90) sets
  // the network is built from cannot hold
  using Polygon45 = boost::polygon::polygon_45_data<int>;
  using Polygon45WithHoles = boost::polygon::polygon_45_with_holes_data<int>;
  using Polygon45Set = boost::polygon::polygon_45_set_data<int>;

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

  // The shapes with 45 degree edges are added to shapes_45 instead of being
  // returned with the others
  LayerMap<odb::geom::BoostPolygon90Set> generatePolygonsFromSWire(
      odb::dbSWire* wire,
      LayerMap<Polygon45Set>& shapes_45);
  LayerMap<odb::geom::BoostPolygon90Set> generatePolygonsFromITerms(
      std::vector<TerminalNode*>& terminals,
      LayerMap<Polygon45Set>& shapes_45);
  LayerMap<odb::geom::BoostPolygon90Set> generatePolygonsFromBTerms(
      std::vector<TerminalNode*>& terminals);
  void processPolygonToRectangles(
      odb::dbTechLayer* layer,
      const odb::geom::BoostPolygon90WithHoles& polygon,
      const TerminalTree& terminals,
      std::vector<std::unique_ptr<Shape>>& new_shapes,
      std::vector<std::unique_ptr<Node>>& new_nodes,
      std::map<Shape*, std::set<Node*>>& terminal_connections);
  // Slices a polygon with 45 degree edges into trapezoids, each of which
  // becomes a shape
  void processPolygonToTrapezoids(
      odb::dbTechLayer* layer,
      const Polygon45WithHoles& polygon,
      const TerminalTree& terminals,
      std::vector<std::unique_ptr<Shape>>& new_shapes,
      std::vector<std::unique_ptr<Node>>& new_nodes,
      std::map<Shape*, std::set<Node*>>& terminal_connections);
  void generateCutNodesForSBox(odb::dbSBox* box,
                               bool single_via,
                               std::vector<std::unique_ptr<Node>>& new_nodes,
                               Connections& new_connections);
  LayerMap<odb::geom::BoostPolygon90Set> generatePolygonsFromBox(
      odb::dbBox* box,
      const odb::dbTransform& transform) const;

  TerminalTree getTerminalTree(
      const std::vector<TerminalNode*>& terminals) const;
  NodeTree getNodeTree(odb::dbTechLayer* layer) const;

  void initMinimumNodePitch();
  void constructAssembly();
  void addChipletNetwork(odb::dbChipBumpInst* bump_inst,
                         odb::dbChipInst* chip_inst);
  void addInterDieConnection(odb::dbChipRSeg* rseg);
  void createMatchedInterDieConnections(std::optional<double> resistance);
  std::vector<BondState> getBondState() const;
  ChipletNetwork* findChipletNetwork(odb::dbChipBumpInst* bump_inst);
  Node* findBondEndpoint(odb::dbChipCapNode* cap_node);
  Node* findBondEndpoint(odb::dbChipBumpInst* bump_inst);

  void recoverMemory();

  void reset();

  odb::dbNet* net_;
  odb::dbChipNet* chip_net_ = nullptr;

  utl::Logger* logger_;

  bool floorplanning_;

  LayerMap<std::vector<std::unique_ptr<Shape>>> shapes_;
  LayerMap<std::vector<std::unique_ptr<Node>>> nodes_;

  Connections connections_;

  std::vector<std::unique_ptr<ITermNode>> iterm_nodes_;
  std::vector<std::unique_ptr<BPinNode>> bpin_nodes_;

  odb::PtrMap<odb::dbTechLayer, int> min_node_pitch_;

  std::vector<ChipletNetwork> chiplet_networks_;
  Connections interdie_connections_;
  std::vector<BondState> bond_state_;
  std::optional<double> interchip_resistance_;
  bool connectivity_only_ = false;

  static constexpr int kMinNodePitchMultiplier = 10;
  static constexpr double kMinNodePitchInUm = 10.0;
};

}  // namespace psm
