// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024-2025, The OpenROAD Authors

#pragma once

#include <string>
#include <vector>

#include "Coordinates.h"
#include "Opendp.h"

namespace dpl {

using odb::dbOrientType;
using odb::dbSite;
using odb::Rect;

class MasterEdge
{
 public:
  MasterEdge(unsigned int type, const Rect& box);
  unsigned int getEdgeType() const;
  const Rect& getBBox() const;

 private:
  unsigned int edge_type_idx_{0};
  Rect bbox_;
};

class Master
{
 public:
  bool isMultiRow() const;
  const std::vector<MasterEdge>& getEdges() const;
  Rect getBBox() const;
  void setMultiRow(bool in);
  void addEdge(const MasterEdge& edge);
  void clearEdges();
  void setBBox(Rect box);

 private:
  Rect boundary_box_;
  bool is_multi_row_ = false;
  std::vector<MasterEdge> edges_;
};

class Pin;
class Group;

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
  ~Node();
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
  int getNumPins() const;
  const std::vector<Pin*>& getPins() const;
  int getGroupId() const;

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
  void addPin(Pin* pin);
  void setGroupId(int id);

  bool adjustCurrOrient(const dbOrientType& newOrient);

 protected:
  int id_ = 0;
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
  std::string getName() const;
  const std::vector<Rect>& getRects() const;
  std::vector<Node*> getCells() const;
  const Rect& getBBox() const;
  double getUtil() const;
  int getId() const;
  // setters
  void setId(int id);
  void setName(const std::string& in);
  void addRect(const Rect& in);
  void addCell(Node* cell);
  void setBoundary(const Rect& in);
  void setUtil(double in);

 private:
  int id_;
  std::string name_;
  std::vector<Rect> region_boundaries_;
  std::vector<Node*> cells_;
  Rect boundary_;
  double util_{0.0};
};

class Edge
{
 public:
  int getId() const;
  void setId(int id);
  int getNumPins() const;
  const std::vector<Pin*>& getPins() const;
  void addPin(Pin* pin);

 private:
  int id_ = 0;
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

  Pin();
  void setDirection(int dir);
  int getDirection() const;
  void setNode(Node* node);
  void setEdge(Edge* ed);
  Node* getNode() const;
  Edge* getEdge() const;
  void setOffsetX(DbuX offsetX);
  DbuX getOffsetX() const;
  void setOffsetY(DbuY offsetY);
  DbuY getOffsetY() const;
  void setPinLayer(int layer);
  int getPinLayer() const;
  void setPinWidth(DbuX width);
  DbuX getPinWidth() const;
  void setPinHeight(DbuY height);
  DbuY getPinHeight() const;

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
};

}  // namespace dpl
