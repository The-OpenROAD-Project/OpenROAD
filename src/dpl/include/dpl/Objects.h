/////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2024, Precision Innovations Inc.
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
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include "Opendp.h"
#include "Coordinates.h"

namespace dpl {

using odb::dbOrientType;
using odb::dbSite;

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
  unsigned int edge_type_idx_{0};
  odb::Rect bbox_;
};

struct EdgeSpacingEntry
{
  EdgeSpacingEntry(const int spc_in,
                   const bool is_exact_in,
                   const bool except_abutted_in)
      : spc(spc_in), is_exact(is_exact_in), except_abutted(except_abutted_in)
  {
  }
  bool operator<(const EdgeSpacingEntry& rhs) const { return spc < rhs.spc; }
  int spc;
  bool is_exact;
  bool except_abutted;
};

class Master
{
  public:
  bool isMultiRow() const { return is_multi_row_; }
  const std::vector<MasterEdge>& getEdges() { return edges_; }
  const odb::Rect getBBox() { return boundary_box_; }
  void setMultiRow(const bool in) { is_multi_row_ = in; }
  void addEdge(const MasterEdge& edge) { edges_.emplace_back(edge); }
  void clearEdges() { edges_.clear(); }
  void setBBox(const odb::Rect box) { boundary_box_ = box; }
  private:
  odb::Rect boundary_box_;
  bool is_multi_row_ = false;
  std::vector<MasterEdge> edges_;
};

class Pin;
class Group;

class GridNode
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
  ~GridNode() = default;
  // getters
  int getId() const;
  DbuY getOrigBottom() const;
  DbuX getOrigLeft() const;
  DbuX getLeft() const;
  DbuY getBottom() const;
  DbuX getRight() const;
  DbuY getTop() const;
  DbuX getWidth() const;
  DbuY getHeight() const;
  dbInst* getDbInst() const;
  dbOrientType getOrient() const;
  bool isFixed() const;
  bool isPlaced() const;
  bool isHold() const;
  dbSite* getSite() const;
  DbuX siteWidth() const;
  bool isHybrid() const;
  bool isHybridParent() const;
  int64_t area() const;
  const char* name() const;
  int getBottomPower() const;
  int getTopPower() const;
  Type getType() const;
  bool isTerminal() const;
  bool isFiller() const;
  bool isStdCell() const;
  bool isBlock() const;
  Group* getGroup() const;
  const Rect* getRegion() const;
  Master* getMaster() const;
  bool inGroup() const;
  int getNumPins() const { return (int) pins_.size(); }
  const std::vector<Pin*>& getPins() const { return pins_; }
  int getGroupId() const { return group_id_; }
  

  // setters
  void setId(int id);
  void setFixed(bool in);
  void setDbInst(dbInst* inst);
  void setLeft(DbuX x);
  void setBottom(DbuY y);
  void setOrient(const dbOrientType& in);
  void setWidth(DbuX width);
  void setHeight(DbuY height);
  void setPlaced(bool in);
  void setHold(bool in);
  void setBottomPower(int bot);
  void setTopPower(int top);
  void setOrigBottom(DbuY bottom);
  void setOrigLeft(DbuX left);
  void setType(Type type);
  void setGroup(Group* in);
  void setRegion(const Rect* in);
  void setMaster(Master* in);
  void addPin(Pin* pin) { pins_.emplace_back(pin); }
  bool adjustCurrOrient(const dbOrientType& newOrient);
  void setGroupId(int id) { group_id_ = id; }



 protected:
  int id_ = 0;
  // dbInst
  odb::dbInst* db_inst_{nullptr};
  // Current position; bottom corner.
  DbuX left_{0};
  DbuY bottom_{0};
  dbOrientType orient_;
  // Original position.
  DbuX orig_left_{0};
  DbuY orig_bottom_{0};
  // Width and height.
  DbuX width_{0};
  DbuY height_{0};
  // Type.
  Type type_{UNKNOWN};
  // Fixed or not fixed.
  bool fixed_{false};
  bool placed_{false};
  bool hold_{false};
  // For power.
  int powerTop_{0};
  int powerBot_{0};
  // Master and edges
  Master* master_{nullptr};
  Group* group_{nullptr};
  const Rect* region_{nullptr};  // group rect
  // // Regions.
  int group_id_{0};
  // Pins.
  std::vector<Pin*> pins_;
};

class Group
{
  public:
  // getters
  string getName() { return name_; }
  const vector<Rect>& getRects() const { return region_boundaries_; }
  const vector<GridNode*> getCells() const { return cells_; }
  const Rect& getBBox() const { return boundary_; }
  double getUtil() const { return util_; }
  int getId() const { return id_; }
  // setters
  void setId(int id) { id_ = id; }
  void setName(const string& in) { name_ = in;}
  void addRect(const Rect& in) {region_boundaries_.emplace_back(in);}
  void addCell(GridNode* cell) { cells_.emplace_back(cell); }
  void setBoundary(const Rect& in) { boundary_ = in; }
  void setUtil(const double in) { util_ = in; }
  private:
  int id_;
  string name_;
  vector<Rect> region_boundaries_;
  vector<GridNode*> cells_;
  Rect boundary_;
  double util_{0.0};
};


class Edge
{
 public:
  int getId() const { return id_; }
  void setId(int id) { id_ = id; }
  int getNumPins() const { return (int) pins_.size(); }
  const std::vector<Pin*>& getPins() const { return pins_; }
  void addPin(Pin* pin) { pins_.emplace_back(pin); }

 private:
  // Id.
  int id_ = 0;
  // Pins.
  std::vector<Pin*> pins_;
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

  Pin() = default;

  void setDirection(int dir) { dir_ = dir; }
  int getDirection() const { return dir_; }

  void setNode(GridNode* node) { node_ = node; }
  void setEdge(Edge* ed) { edge_ = ed; }
  GridNode* getNode() const { return node_; }
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
  GridNode* node_ = nullptr;
  Edge* edge_ = nullptr;
  // Offsets from cell center.
  DbuX offsetX_{0};
  DbuY offsetY_{0};
};


}  // namespace dpl
