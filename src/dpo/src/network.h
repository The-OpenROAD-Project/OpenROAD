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

class Master
{
 public:
  Master() = default;
  odb::Rect boundary_box_;
  std::vector<MasterEdge> edges_;
};

class Node : public GridNode
{
 public:
  enum Type
  {
    UNKNOWN,
    CELL,
    TERMINAL,
    MACROCELL,
    FILLER
  };

  enum Fixity
  {
    NOT_FIXED,
    FIXED_X,
    FIXED_Y,
    FIXED_XY,
  };

  Node();
  bool isPlaced() const override { return false; }
  bool isHybrid() const override { return false; }
  DbuX xMin() const override { return left_; }
  DbuY yMin() const override { return DbuY(bottom_); }
  DbuX dx() const override { return DbuX(w_); }
  DbuY dy() const override { return DbuY(h_); }
  odb::dbInst* getDbInst() const override { return db_inst_; }
  DbuX siteWidth() const override { return DbuX(0); }

  int getArea() const { return w_.v * h_.v; }
  DbuY getBottom() const { return bottom_; }
  int getBottomPower() const { return powerBot_; }
  int getTopPower() const { return powerTop_; }
  dbOrientType getCurrOrient() const { return currentOrient_; }
  Fixity getFixed() const { return fixed_; }
  DbuY getHeight() const { return h_; }
  int getId() const { return id_; }
  DbuX getLeft() const { return left_; }
  DbuY getOrigBottom() const { return origBottom_; }
  DbuX getOrigLeft() const { return origLeft_; }
  int getRegionId() const { return regionId_; }
  DbuX getRight() const { return left_.v + DbuX(w_); }
  DbuY getTop() const { return bottom_ + h_; }
  Type getType() const { return type_; }
  DbuX getWidth() const { return w_; }
  Master* getMaster() const { return master_; }

  void setBottom(DbuY bottom) { bottom_ = bottom; }
  void setBottomPower(int bot) { powerBot_ = bot; }
  void setTopPower(int top) { powerTop_ = top; }
  void setCurrOrient(const dbOrientType& orient) { currentOrient_ = orient; }
  void setFixed(Fixity fixed) { fixed_ = fixed; }
  void setHeight(DbuY h) { h_ = h; }
  void setId(int id) { id_ = id; }
  void setLeft(DbuX left) { left_ = left; }
  void setOrigBottom(DbuY bottom) { origBottom_ = bottom; }
  void setOrigLeft(DbuX left) { origLeft_ = left; }
  void setRegionId(int id) { regionId_ = id; }
  void setType(Type type) { type_ = type; }
  void setWidth(DbuX w) { w_ = w; }
  void setMaster(Master* in) { master_ = in; }
  void setDbInst(odb::dbInst* inst) { db_inst_ = inst; }

  bool adjustCurrOrient(const dbOrientType& newOrient);

  bool isTerminal() const { return (type_ == TERMINAL); }
  bool isFiller() const { return (type_ == FILLER); }
  bool isFixed() const override { return (fixed_ != NOT_FIXED); }

  void addLeftEdgeType(int etl) { etls_.emplace_back(etl); }
  void addRigthEdgeType(int etr) { etrs_.emplace_back(etr); }
  const std::vector<int>& getLeftEdgeTypes() const { return etls_; }
  const std::vector<int>& getRightEdgeTypes() const { return etrs_; }
  void swapEdgeTypes() { std::swap(etls_, etrs_); }

  int getNumPins() const { return (int) pins_.size(); }
  const std::vector<Pin*>& getPins() const { return pins_; }

 private:
  // Id.
  int id_ = 0;
  // Current position; bottom corner.
  DbuX left_{0};
  DbuY bottom_{0};
  // Original position.
  DbuX origLeft_{0};
  DbuY origBottom_{0};
  // Width and height.
  DbuX w_{0};
  DbuY h_{0};
  // Type.
  Type type_ = UNKNOWN;
  // Fixed or not fixed.
  Fixity fixed_ = NOT_FIXED;
  // For edge types and spacing tables.
  std::vector<int> etls_, etrs_;
  // For power.
  int powerTop_ = Architecture::Row::Power_UNK;
  int powerBot_ = Architecture::Row::Power_UNK;
  // Regions.
  int regionId_ = 0;
  // Orientations.
  dbOrientType currentOrient_;
  // Pins.
  std::vector<Pin*> pins_;
  // Master and edges
  Master* master_{nullptr};

  // dbInst
  odb::dbInst* db_inst_{nullptr};

  friend class Network;
};

class Edge
{
 public:
  int getId() const { return id_; }
  void setId(int id) { id_ = id; }

  void setNdr(int ndr) { ndr_ = ndr; }
  int getNdr() const { return ndr_; }

  int getNumPins() const { return (int) pins_.size(); }
  const std::vector<Pin*>& getPins() const { return pins_; }

 private:
  // Id.
  int id_ = 0;
  // Refer to routing rule stored elsewhere.
  int ndr_ = 0;
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

  void setOffsetX(double offsetX) { offsetX_ = offsetX; }
  double getOffsetX() const { return offsetX_; }

  void setOffsetY(double offsetY) { offsetY_ = offsetY; }
  double getOffsetY() const { return offsetY_; }

  void setPinLayer(int layer) { pinLayer_ = layer; }
  int getPinLayer() const { return pinLayer_; }

  void setPinWidth(double width) { pinWidth_ = width; }
  double getPinWidth() const { return pinWidth_; }

  void setPinHeight(double height) { pinHeight_ = height; }
  double getPinHeight() const { return pinHeight_; }

 private:
  // Pin width and height.
  double pinWidth_ = 0;
  double pinHeight_ = 0;
  // Direction.
  int dir_ = Dir_INOUT;
  // Layer.
  int pinLayer_ = 0;
  // Node and edge for pin.
  Node* node_ = nullptr;
  Edge* edge_ = nullptr;
  // Offsets from cell center.
  double offsetX_ = 0;
  double offsetY_ = 0;

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
