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

namespace dpl {
class Master;
class Pin;
class Grid;
class Edge;
class PlacementDRC;

class Network
{
 public:
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
  const std::string& getEdgeName(int i) const { return edgeNames_.at(i); }

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

  odb::Rect core_;  // Core area of the design.
  std::vector<std::unique_ptr<Master>> masters_;
  std::vector<std::unique_ptr<Node>> nodes_;  // The nodes in the netlist...
  std::vector<std::unique_ptr<Edge>> edges_;  // The edges in the netlist...
  std::vector<std::unique_ptr<Pin>> pins_;    // The pins in the network...
  std::vector<odb::Rect> blockages_;          // The placement blockages ..

  std::unordered_map<int, std::string> edgeNames_;  // Names of edges...

  std::unordered_map<odb::dbInst*, int> inst_to_node_idx_;
  std::unordered_map<odb::dbBTerm*, int> term_to_node_idx_;
  std::unordered_map<odb::dbMaster*, int> master_to_idx_;
  std::unordered_map<odb::dbNet*, int> net_to_edge_idx_;
  uint32_t cells_cnt_{0};
  uint32_t terminals_cnt_{0};
};

}  // namespace dpl
