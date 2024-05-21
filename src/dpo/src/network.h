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
#include "odb/geom.h"
#include "orientation.h"

namespace dpo {

////////////////////////////////////////////////////////////////////////////////
// Forward declarations.
////////////////////////////////////////////////////////////////////////////////
class Pin;

const int EDGETYPE_DEFAULT = 0;

////////////////////////////////////////////////////////////////////////////////
// Classes.
////////////////////////////////////////////////////////////////////////////////
class Node
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

  int getArea() const { return w_ * h_; }
  unsigned getAvailOrient() const { return availOrient_; }
  int getBottom() const { return bottom_; }
  int getBottomPower() const { return powerBot_; }
  int getTopPower() const { return powerTop_; }
  unsigned getCurrOrient() const { return currentOrient_; }
  Fixity getFixed() const { return fixed_; }
  int getHeight() const { return h_; }
  int getId() const { return id_; }
  int getLeft() const { return left_; }
  double getOrigBottom() const { return origBottom_; }
  double getOrigLeft() const { return origLeft_; }
  int getRegionId() const { return regionId_; }
  int getRight() const { return left_ + w_; }
  int getTop() const { return bottom_ + h_; }
  Type getType() const { return type_; }
  int getWidth() const { return w_; }

  void setAvailOrient(unsigned avail) { availOrient_ = avail; }
  void setBottom(int bottom) { bottom_ = bottom; }
  void setBottomPower(int bot) { powerBot_ = bot; }
  void setTopPower(int top) { powerTop_ = top; }
  void setCurrOrient(unsigned orient) { currentOrient_ = orient; }
  void setFixed(Fixity fixed) { fixed_ = fixed; }
  void setHeight(int h) { h_ = h; }
  void setId(int id) { id_ = id; }
  void setLeft(int left) { left_ = left; }
  void setOrigBottom(double bottom) { origBottom_ = bottom; }
  void setOrigLeft(double left) { origLeft_ = left; }
  void setRegionId(int id) { regionId_ = id; }
  void setType(Type type) { type_ = type; }
  void setWidth(int w) { w_ = w; }

  bool adjustCurrOrient(unsigned newOrient);

  bool isTerminal() const { return (type_ == TERMINAL); }
  bool isFiller() const { return (type_ == FILLER); }
  bool isFixed() const { return (fixed_ != NOT_FIXED); }

  int getLeftEdgeType() const { return etl_; }
  int getRightEdgeType() const { return etr_; }

  void setLeftEdgeType(int etl) { etl_ = etl; }
  void setRightEdgeType(int etr) { etr_ = etr; }
  void swapEdgeTypes() { std::swap<int>(etl_, etr_); }

  int getNumPins() const { return (int) pins_.size(); }
  const std::vector<Pin*>& getPins() const { return pins_; }

 private:
  // Id.
  int id_ = 0;
  // Current position; bottom corner.
  int left_ = 0;
  int bottom_ = 0;
  // Original position.  Stored as double still.
  double origLeft_ = 0;
  double origBottom_ = 0;
  // Width and height.
  int w_ = 0;
  int h_ = 0;
  // Type.
  Type type_ = UNKNOWN;
  // Fixed or not fixed.
  Fixity fixed_ = NOT_FIXED;
  // For edge types and spacing tables.
  int etl_ = EDGETYPE_DEFAULT;
  int etr_ = EDGETYPE_DEFAULT;
  // For power.
  int powerTop_ = Architecture::Row::Power_UNK;
  int powerBot_ = Architecture::Row::Power_UNK;
  // Regions.
  int regionId_ = 0;
  // Orientations.
  unsigned currentOrient_ = Orientation_N;
  unsigned availOrient_ = Orientation_N;
  // Pins.
  std::vector<Pin*> pins_;

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
  Node* createAndAddFillerNode(int left,
                               int bottom,
                               int width,
                               int height);  // Extras to block space.

  // For creating and adding edges.
  Edge* createAndAddEdge();

  void createAndAddBlockage(const odb::Rect& bounds);

 private:
  Pin* createAndAddPin();

  std::vector<Edge*> edges_;  // The edges in the netlist...
  std::unordered_map<int, std::string> edgeNames_;  // Names of edges...
  std::vector<Node*> nodes_;  // The nodes in the netlist...
  std::unordered_map<int, std::string> nodeNames_;  // Names of nodes...
  std::vector<Pin*> pins_;            // The pins in the network...
  std::vector<odb::Rect> blockages_;  // The placement blockages ...
};

}  // namespace dpo
