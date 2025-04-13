// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2025, The OpenROAD Authors

#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "architecture.h"
#include "dpl/Coordinates.h"
#include "dpl/Objects.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"
namespace dpl {
class Master;
class Pin;
class Grid;
class Edge;
}  // namespace dpl

namespace dpo {

using dpl::DbuX;
using dpl::DbuY;
using dpl::Edge;
using dpl::GridX;
using dpl::GridY;
using dpl::Master;
using dpl::Pin;
using odb::dbOrientType;

class Network
{
 public:
  struct comparePinsByNodeId
  {
    bool operator()(const Pin* a, const Pin* b)
    {
      return a->getNode()->getId() < b->getNode()->getId();
    }
  };

  class comparePinsByEdgeId
  {
   public:
    explicit comparePinsByEdgeId(Network* nw) : nw_(nw) {}
    bool operator()(const Pin* a, const Pin* b)
    {
      return a->getEdge()->getId() < b->getEdge()->getId();
    }
    Network* nw_ = nullptr;
  };

  class comparePinsByOffset
  {
   public:
    explicit comparePinsByOffset(Network* nw) : nw_(nw) {}
    bool operator()(const Pin* a, const Pin* b)
    {
      if (a->getOffsetX() == b->getOffsetX()) {
        return a->getOffsetY() < b->getOffsetY();
      }
      return a->getOffsetX() < b->getOffsetX();
    }
    Network* nw_ = nullptr;
  };

 public:
  ~Network();

  int getNumNodes() const { return (int) nodes_.size(); }
  Node* getNode(int i) { return nodes_[i]; }
  void setNodeName(int i, const std::string& name) { nodeNames_[i] = name; }
  void setNodeName(int i, const char* name) { nodeNames_[i] = name; }
  const std::string& getNodeName(int i) const { return nodeNames_.at(i); }

  int getNumEdges() const { return (int) edges_.size(); }
  Edge* getEdge(int i) const { return edges_[i]; }
  void setEdgeName(int i, std::string& name) { edgeNames_[i] = name; }
  void setEdgeName(int i, const char* name) { edgeNames_[i] = name; }
  const std::string& getEdgeName(int i) const { return edgeNames_.at(i); }

  int getNumPins() const { return (int) pins_.size(); }

  int getNumBlockages() const { return (int) blockages_.size(); }
  odb::Rect getBlockage(int i) const { return blockages_[i]; }

  // For creating and adding pins.
  Pin* createAndAddPin(Node* nd, Edge* ed);

  // For creating and adding cells.
  Node* createAndAddNode();  // Network cells.
  Node* createAndAddFillerNode(DbuX left,
                               DbuY bottom,
                               DbuX width,
                               DbuY height);  // Extras to block space.

  // For creating and adding edges.
  Edge* createAndAddEdge();

  // For creating masters.
  Master* createAndAddMaster();

  void createAndAddBlockage(const odb::Rect& bounds);

 private:
  Pin* createAndAddPin();

  std::vector<Edge*> edges_;  // The edges in the netlist...
  std::unordered_map<int, std::string> edgeNames_;  // Names of edges...
  std::vector<Node*> nodes_;  // The nodes in the netlist...
  std::unordered_map<int, std::string> nodeNames_;  // Names of nodes...
  std::vector<Pin*> pins_;            // The pins in the network...
  std::vector<odb::Rect> blockages_;  // The placement blockages ..
  std::vector<std::unique_ptr<Master>> masters_;
};

}  // namespace dpo
