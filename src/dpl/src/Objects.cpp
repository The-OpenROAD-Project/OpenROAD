// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024-2025, The OpenROAD Authors

#include "dpl/Objects.h"

#include <string>
#include <vector>

namespace dpl {
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
MasterEdge::MasterEdge(unsigned int type, const Rect& box)
    : edge_type_idx_(type), bbox_(box)
{
}

unsigned int MasterEdge::getEdgeType() const
{
  return edge_type_idx_;
}
const Rect& MasterEdge::getBBox() const
{
  return bbox_;
}
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

bool Master::isMultiRow() const
{
  return is_multi_row_;
}
const std::vector<MasterEdge>& Master::getEdges() const
{
  return edges_;
}
Rect Master::getBBox() const
{
  return boundary_box_;
}
void Master::setMultiRow(const bool in)
{
  is_multi_row_ = in;
}
void Master::addEdge(const MasterEdge& edge)
{
  edges_.emplace_back(edge);
}
void Master::clearEdges()
{
  edges_.clear();
}
void Master::setBBox(const Rect box)
{
  boundary_box_ = box;
}
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

Node::~Node() = default;
int Node::getId() const
{
  return id_;
}
DbuY Node::getOrigBottom() const
{
  return orig_bottom_;
}
DbuX Node::getOrigLeft() const
{
  return orig_left_;
}
DbuX Node::getLeft() const
{
  return left_;
}
DbuY Node::getBottom() const
{
  return bottom_;
}
DbuX Node::getRight() const
{
  return left_ + width_;
}
DbuY Node::getTop() const
{
  return bottom_ + height_;
}
DbuX Node::getWidth() const
{
  return width_;
}
DbuY Node::getHeight() const
{
  return height_;
}
dbInst* Node::getDbInst() const
{
  return db_inst_;
}
dbOrientType Node::getOrient() const
{
  return orient_;
}
bool Node::isFixed() const
{
  return fixed_;
};
bool Node::isPlaced() const
{
  return placed_;
}
bool Node::isHold() const
{
  return hold_;
}
dbSite* Node::getSite() const
{
  if (!db_inst_ || !db_inst_->getMaster()) {
    return nullptr;
  }
  return db_inst_->getMaster()->getSite();
}
DbuX Node::siteWidth() const
{
  if (db_inst_) {
    auto site = db_inst_->getMaster()->getSite();
    if (site) {
      return DbuX{site->getWidth()};
    }
  }
  return DbuX{0};
}
bool Node::isHybrid() const
{
  dbSite* site = getSite();
  return site ? site->isHybrid() : false;
}
bool Node::isHybridParent() const
{
  dbSite* site = getSite();
  return site ? site->hasRowPattern() : false;
}
int64_t Node::area() const
{
  dbMaster* master = db_inst_->getMaster();
  return int64_t(master->getWidth()) * master->getHeight();
}
const char* Node::name() const
{
  return db_inst_->getConstName();
}
int Node::getBottomPower() const
{
  return powerBot_;
}
int Node::getTopPower() const
{
  return powerTop_;
}
Node::Type Node::getType() const
{
  return type_;
}
bool Node::isTerminal() const
{
  return (type_ == TERMINAL);
}
bool Node::isFiller() const
{
  return (type_ == FILLER);
}
bool Node::isStdCell() const
{
  if (db_inst_ == nullptr) {
    return false;
  }
  return db_inst_->isCore() || db_inst_->isEndCap();
}
bool Node::isBlock() const
{
  return db_inst_ && db_inst_->getMaster()->getType() == dbMasterType::BLOCK;
}
Group* Node::getGroup() const
{
  return group_;
}
const Rect* Node::getRegion() const
{
  return region_;
}
Master* Node::getMaster() const
{
  return master_;
}
bool Node::inGroup() const
{
  return group_ != nullptr;
}
int Node::getNumPins() const
{
  return (int) pins_.size();
}
const std::vector<Pin*>& Node::getPins() const
{
  return pins_;
}
int Node::getGroupId() const
{
  return group_id_;
}
void Node::setId(int id)
{
  id_ = id;
}
void Node::setFixed(bool in)
{
  fixed_ = in;
}
void Node::setDbInst(dbInst* inst)
{
  db_inst_ = inst;
}
void Node::setLeft(DbuX x)
{
  left_ = x;
}
void Node::setBottom(DbuY y)
{
  bottom_ = y;
}
void Node::setOrient(const dbOrientType& in)
{
  orient_ = in;
}
void Node::setWidth(DbuX width)
{
  width_ = width;
}
void Node::setHeight(DbuY height)
{
  height_ = height;
}
void Node::setPlaced(bool in)
{
  placed_ = in;
}
void Node::setHold(bool in)
{
  hold_ = in;
}
void Node::setBottomPower(int bot)
{
  powerBot_ = bot;
}
void Node::setTopPower(int top)
{
  powerTop_ = top;
}
void Node::setOrigBottom(DbuY bottom)
{
  orig_bottom_ = bottom;
}
void Node::setOrigLeft(DbuX left)
{
  orig_left_ = left;
}
void Node::setType(Type type)
{
  type_ = type;
}
void Node::setGroup(Group* in)
{
  group_ = in;
}
void Node::setRegion(const Rect* in)
{
  region_ = in;
}
void Node::setMaster(Master* in)
{
  master_ = in;
}
void Node::addPin(Pin* pin)
{
  pins_.emplace_back(pin);
}
void Node::setGroupId(int id)
{
  group_id_ = id;
}
bool Node::adjustCurrOrient(const dbOrientType& newOri)
{
  // Change the orientation of the cell, but leave the lower-left corner
  // alone.  This means changing the locations of pins and possibly
  // changing the edge types as well as the height and width.
  auto curOri = orient_;
  if (newOri == curOri) {
    return true;
  }

  if (curOri == dbOrientType::R90 || curOri == dbOrientType::MXR90
      || curOri == dbOrientType::R270 || curOri == dbOrientType::MYR90) {
    if (newOri == dbOrientType::R0 || newOri == dbOrientType::MY
        || newOri == dbOrientType::MX || newOri == dbOrientType::R180) {
      // Rotate the cell counter-clockwise by 90 degrees.
      for (Pin* pin : pins_) {
        const auto dx = pin->getOffsetX().v;
        const auto dy = pin->getOffsetY().v;
        pin->setOffsetX(DbuX{-dy});
        pin->setOffsetY(DbuY{dx});
      }
      {
        int tmp = width_.v;
        width_ = DbuX{height_.v};
        height_ = DbuY{tmp};
      }
      if (curOri == dbOrientType::R90) {
        curOri = dbOrientType::R0;
      } else if (curOri == dbOrientType::MXR90) {
        curOri = dbOrientType::MX;
      } else if (curOri == dbOrientType::MYR90) {
        curOri = dbOrientType::MY;
      } else {
        curOri = dbOrientType::R180;
      }
    }
  } else {
    if (newOri == dbOrientType::R90 || newOri == dbOrientType::MXR90
        || newOri == dbOrientType::MYR90 || newOri == dbOrientType::R270) {
      // Rotate the cell clockwise by 90 degrees.
      for (Pin* pin : pins_) {
        const auto dx = pin->getOffsetX().v;
        const auto dy = pin->getOffsetY().v;
        pin->setOffsetX(DbuX{dy});
        pin->setOffsetY(DbuY{-dx});
      }
      {
        int tmp = width_.v;
        width_ = DbuX{height_.v};
        height_ = DbuY{tmp};
      }
      if (curOri == dbOrientType::R0) {
        curOri = dbOrientType::R90;
      } else if (curOri == dbOrientType::MX) {
        curOri = dbOrientType::MXR90;
      } else if (curOri == dbOrientType::MY) {
        curOri = dbOrientType::MYR90;
      } else {
        curOri = dbOrientType::R270;
      }
    }
  }
  // Both the current and new orientations should be {N, FN, FS, S} or {E, FE,
  // FW, W}.
  int mX = 1;
  int mY = 1;
  if (curOri == dbOrientType::R90 || curOri == dbOrientType::MXR90
      || curOri == dbOrientType::MYR90 || curOri == dbOrientType::R270) {
    const bool test1
        = (curOri == dbOrientType::R90 || curOri == dbOrientType::MYR90);
    const bool test2
        = (newOri == dbOrientType::R90 || newOri == dbOrientType::MYR90);
    if (test1 != test2) {
      mX = -1;
    }
    const bool test3
        = (curOri == dbOrientType::R90 || curOri == dbOrientType::MXR90);
    const bool test4
        = (newOri == dbOrientType::R90 || newOri == dbOrientType::MXR90);
    if (test3 != test4) {
      mY = -1;
    }
  } else {
    const bool test1
        = (curOri == dbOrientType::R0 || curOri == dbOrientType::MX);
    const bool test2
        = (newOri == dbOrientType::R0 || newOri == dbOrientType::MX);
    if (test1 != test2) {
      mX = -1;
    }
    const bool test3
        = (curOri == dbOrientType::R0 || curOri == dbOrientType::MY);
    const bool test4
        = (newOri == dbOrientType::R0 || newOri == dbOrientType::MY);
    if (test3 != test4) {
      mY = -1;
    }
  }

  for (Pin* pin : pins_) {
    pin->setOffsetX(pin->getOffsetX() * DbuX{mX});
    pin->setOffsetY(pin->getOffsetY() * DbuY{mY});
  }
  orient_ = newOri;
  return true;
}
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

std::string Group::getName() const
{
  return name_;
}
const std::vector<Rect>& Group::getRects() const
{
  return region_boundaries_;
}
std::vector<Node*> Group::getCells() const
{
  return cells_;
}
const Rect& Group::getBBox() const
{
  return boundary_;
}
double Group::getUtil() const
{
  return util_;
}
int Group::getId() const
{
  return id_;
}
void Group::setId(int id)
{
  id_ = id;
}
void Group::setName(const std::string& in)
{
  name_ = in;
}
void Group::addRect(const Rect& in)
{
  region_boundaries_.emplace_back(in);
}
void Group::addCell(Node* cell)
{
  cells_.emplace_back(cell);
}
void Group::setBoundary(const Rect& in)
{
  boundary_ = in;
}
void Group::setUtil(const double in)
{
  util_ = in;
}
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

int Edge::getId() const
{
  return id_;
}
void Edge::setId(int id)
{
  id_ = id;
}
int Edge::getNumPins() const
{
  return (int) pins_.size();
}
const std::vector<Pin*>& Edge::getPins() const
{
  return pins_;
}
void Edge::addPin(Pin* pin)
{
  pins_.emplace_back(pin);
}
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

Pin::Pin() = default;
void Pin::setDirection(int dir)
{
  dir_ = dir;
}
int Pin::getDirection() const
{
  return dir_;
}
void Pin::setNode(Node* node)
{
  node_ = node;
}
void Pin::setEdge(Edge* ed)
{
  edge_ = ed;
}
Node* Pin::getNode() const
{
  return node_;
}
Edge* Pin::getEdge() const
{
  return edge_;
}
void Pin::setOffsetX(DbuX offsetX)
{
  offsetX_ = offsetX;
}
DbuX Pin::getOffsetX() const
{
  return offsetX_;
}
void Pin::setOffsetY(DbuY offsetY)
{
  offsetY_ = offsetY;
}
DbuY Pin::getOffsetY() const
{
  return offsetY_;
}
void Pin::setPinLayer(int layer)
{
  pinLayer_ = layer;
}
int Pin::getPinLayer() const
{
  return pinLayer_;
}
void Pin::setPinWidth(DbuX width)
{
  pinWidth_ = width;
}
DbuX Pin::getPinWidth() const
{
  return pinWidth_;
}
void Pin::setPinHeight(DbuY height)
{
  pinHeight_ = height;
}
DbuY Pin::getPinHeight() const
{
  return pinHeight_;
}

}  // namespace dpl
