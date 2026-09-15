// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2025, The OpenROAD Authors

#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "Coordinates.h"
#include "Objects.h"
#include "architecture.h"
#include "dpl/Opendp.h"
#include "odb/db.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"
namespace odb {
class dbInst;
class dbBTerm;
class dbNet;
}  // namespace odb

namespace utl {
class Logger;
}  // namespace utl

namespace dpl {
class Master;
class Pin;
class Grid;
class Edge;
class PlacementDRC;

class Network
{
 public:
  void init(utl::Logger* logger) { logger_ = logger; }
  std::vector<std::unique_ptr<Node>>& getNodes() { return nodes_; }
  int getNumNodes() const { return (int) nodes_.size(); }
  uint32_t getNumCells() const { return cells_cnt_; }
  uint32_t getNumTerminals() const { return terminals_cnt_; }
  Node* getNode(int i) { return nodes_[i].get(); }
  Node* getNode(odb::dbInst* inst);
  Node* getNode(odb::dbBTerm* term);
  Master* getMaster(odb::dbMaster*);
  int getNumEdges() const { return (int) edges_.size(); }
  Edge* getEdge(odb::dbNet* net) const;
  Edge* getEdge(int i) const { return edges_[i].get(); }
  void setEdgeName(int i, const std::string& name) { edgeNames_[i] = name; }
  // Edge names are not materialized eagerly (that cost a std::string allocation
  // per net during createNetwork()).  If an explicit name was set via
  // setEdgeName() it is returned; otherwise the name is derived lazily from the
  // backing dbNet.  Returns by value because the lazy path has no stored string
  // to reference.
  std::string getEdgeName(int i) const
  {
    auto it = edgeNames_.find(i);
    if (it != edgeNames_.end()) {
      return it->second;
    }
    if (i >= 0 && i < static_cast<int>(edge_to_net_.size())
        && edge_to_net_[i] != nullptr) {
      return edge_to_net_[i]->getName();
    }
    return "";
  }

  int getNumPins() const { return (int) pins_.size(); }

  int getNumBlockages() const { return (int) blockages_.size(); }
  odb::Rect getBlockage(int i) const { return blockages_[i]; }

  // For creating and adding cells.
  void addNode(odb::dbInst*);
  void addNode(odb::dbBTerm*);
  void addFillerNode(DbuX left,
                     DbuY bottom,
                     DbuX width,
                     DbuY height);  // Extras to block space.

  // For creating and adding edges.
  void addEdge(odb::dbNet* net);

  // Reserve capacity for the netlist containers to avoid repeated reallocation
  // of the unique_ptr vectors while the network is built.
  void reserve(size_t num_nodes, size_t num_edges)
  {
    nodes_.reserve(num_nodes);
    edges_.reserve(num_edges);
  }

  // For creating masters.
  Master* addMaster(odb::dbMaster* db_master,
                    const Grid* grid,
                    const PlacementDRC* drc_engine);
  void createAndAddBlockage(const odb::Rect& bounds);

  void clear();

  // setting and getting core area
  void setCore(const odb::Rect& core) { core_ = core; }
  const odb::Rect& getCore() const { return core_; }

 private:
  Pin* addPin(odb::dbITerm* term);
  Pin* addPin(odb::dbBTerm* term);
  void connect(Pin* pin, Node* node);
  void connect(Pin* pin, Edge* edge);

  utl::Logger* logger_ = nullptr;
  odb::Rect core_;  // Core area of the design.
  std::vector<std::unique_ptr<Master>> masters_;
  std::vector<std::unique_ptr<Node>> nodes_;  // The nodes in the netlist...
  std::vector<std::unique_ptr<Edge>> edges_;  // The edges in the netlist...
  std::vector<std::unique_ptr<Pin>> pins_;    // The pins in the network...
  std::vector<odb::Rect> blockages_;          // The placement blockages ..

  std::unordered_map<int, std::string> edgeNames_;  // Names of edges...
  // Backing nets indexed by edge id, used to derive edge names lazily without
  // allocating a std::string for every net up front.
  std::vector<odb::dbNet*> edge_to_net_;

  std::unordered_map<odb::dbInst*, int> inst_to_node_idx_;
  std::unordered_map<odb::dbBTerm*, int> term_to_node_idx_;
  std::unordered_map<odb::dbMaster*, int> master_to_idx_;
  std::unordered_map<odb::dbNet*, int> net_to_edge_idx_;
  // Cache of the routing-layer bitmask used by each mterm.  The layer set
  // depends only on the mterm geometry (shared across all instances of a
  // master), so it is computed once and reused, avoiding a per-pin walk of the
  // pin geometry for every iterm during createNetwork().
  std::unordered_map<odb::dbMTerm*, uint8_t> mterm_layers_;
  uint32_t cells_cnt_{0};
  uint32_t terminals_cnt_{0};
};

}  // namespace dpl
