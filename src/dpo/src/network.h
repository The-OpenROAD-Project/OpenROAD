///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2021, Andrew Kennings
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

////////////////////////////////////////////////////////////////////////////////
// File: network.h
////////////////////////////////////////////////////////////////////////////////

#pragma once

////////////////////////////////////////////////////////////////////////////////
// Includes.
////////////////////////////////////////////////////////////////////////////////
#include <string>
#include <unordered_map>
#include <vector>

#include "architecture.h"
#include "dpl/Coordinates.h"
#include "dpl/Grid.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"
namespace dpl {
class Master;
}
namespace dpo {

////////////////////////////////////////////////////////////////////////////////
// Forward declarations.
////////////////////////////////////////////////////////////////////////////////
class Pin;

const int EDGETYPE_DEFAULT = 0;
using dpl::DbuX;
using dpl::DbuY;
using dpl::GridNode;
using dpl::GridX;
using dpl::GridY;
using odb::dbOrientType;

////////////////////////////////////////////////////////////////////////////////
// Classes.
////////////////////////////////////////////////////////////////////////////////
class MasterEdge
{
 public:
  MasterEdge(unsigned int type, const odb::Rect& box)
      : edge_type_idx_(type), bbox_(box)
  {
  }
  unsigned int getEdgeType() const { return edge_type_idx_; }
  const odb::Rect& getBBox() const { return bbox_; }

 private:
  unsigned int edge_type_idx_;
  odb::Rect bbox_;
};

class Node : public GridNode
{
 public:
  Node();

  int getRegionId() const { return regionId_; }

  void setRegionId(int id) { regionId_ = id; }

  bool adjustCurrOrient(const dbOrientType& newOrient);

  int getNumPins() const { return (int) pins_.size(); }
  const std::vector<Pin*>& getPins() const { return pins_; }

 private:
  // Regions.
  int regionId_ = 0;
  // Pins.
  std::vector<Pin*> pins_;

  friend class Network;
};

class Edge
{
 public:
  int getId() const { return id_; }
  void setId(int id) { id_ = id; }
  int getNumPins() const { return (int) pins_.size(); }
  const std::vector<Pin*>& getPins() const { return pins_; }

 private:
  // Id.
  int id_ = 0;
  // Pins.
  std::vector<Pin*> pins_;

  friend class Network;
};

class Pin
{
 public:
  enum Direction
  {
    Dir_IN,
    Dir_OUT,
    Dir_INOUT,
    Dir_UNKNOWN
  };

  Pin();

  void setDirection(int dir) { dir_ = dir; }
  int getDirection() const { return dir_; }

  Node* getNode() const { return node_; }
  Edge* getEdge() const { return edge_; }

  void setOffsetX(DbuX offsetX) { offsetX_ = offsetX; }
  DbuX getOffsetX() const { return offsetX_; }

  void setOffsetY(DbuY offsetY) { offsetY_ = offsetY; }
  DbuY getOffsetY() const { return offsetY_; }

  void setPinLayer(int layer) { pinLayer_ = layer; }
  int getPinLayer() const { return pinLayer_; }

  void setPinWidth(DbuX width) { pinWidth_ = width; }
  DbuX getPinWidth() const { return pinWidth_; }

  void setPinHeight(DbuY height) { pinHeight_ = height; }
  DbuY getPinHeight() const { return pinHeight_; }

 private:
  // Pin width and height.
  DbuX pinWidth_{0};
  DbuY pinHeight_{0};
  // Direction.
  int dir_ = Dir_INOUT;
  // Layer.
  int pinLayer_ = 0;
  // Node and edge for pin.
  Node* node_ = nullptr;
  Edge* edge_ = nullptr;
  // Offsets from cell center.
  DbuX offsetX_{0};
  DbuY offsetY_{0};

  friend class Network;
};

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
  dpl::Master* createAndAddMaster();

  void createAndAddBlockage(const odb::Rect& bounds);

 private:
  Pin* createAndAddPin();

  std::vector<Edge*> edges_;  // The edges in the netlist...
  std::unordered_map<int, std::string> edgeNames_;  // Names of edges...
  std::vector<Node*> nodes_;  // The nodes in the netlist...
  std::unordered_map<int, std::string> nodeNames_;  // Names of nodes...
  std::vector<Pin*> pins_;            // The pins in the network...
  std::vector<odb::Rect> blockages_;  // The placement blockages ..
  std::vector<std::unique_ptr<dpl::Master>> masters_;
};

}  // namespace dpo
