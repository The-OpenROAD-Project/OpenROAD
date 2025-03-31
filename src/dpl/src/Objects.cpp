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

#include "dpl/Objects.h"

namespace dpl {
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
MasterEdge::MasterEdge(unsigned int type, const odb::Rect& box)
    : edge_type_idx_(type), bbox_(box)
{
}

unsigned int MasterEdge::getEdgeType() const { return edge_type_idx_; }
const odb::Rect& MasterEdge::getBBox() const { return bbox_; }
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

EdgeSpacingEntry::EdgeSpacingEntry(const int spc_in,
                                 const bool is_exact_in,
                                 const bool except_abutted_in)
    : spc(spc_in), is_exact(is_exact_in), except_abutted(except_abutted_in)
{
}

bool EdgeSpacingEntry::operator<(const EdgeSpacingEntry& rhs) const { return spc < rhs.spc; }
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

bool Master::isMultiRow() const { return is_multi_row_; }
const std::vector<MasterEdge>& Master::getEdges() { return edges_; }
const odb::Rect Master::getBBox() { return boundary_box_; }
void Master::setMultiRow(const bool in) { is_multi_row_ = in; }
void Master::addEdge(const MasterEdge& edge) { edges_.emplace_back(edge); }
void Master::clearEdges() { edges_.clear(); }
void Master::setBBox(const odb::Rect box) { boundary_box_ = box; }
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

GridNode::~GridNode() = default;
int GridNode::getId() const { return id_; }
DbuY GridNode::getOrigBottom() const { return orig_bottom_; }
DbuX GridNode::getOrigLeft() const { return orig_left_; }
DbuX GridNode::getLeft() const { return left_; }
DbuY GridNode::getBottom() const { return bottom_; }
DbuX GridNode::getRight() const { return left_ + width_; }
DbuY GridNode::getTop() const { return bottom_ + height_; }
DbuX GridNode::getWidth() const { return width_; }
DbuY GridNode::getHeight() const { return height_; }
dbInst* GridNode::getDbInst() const { return db_inst_; }
dbOrientType GridNode::getOrient() const { return orient_; }
bool GridNode::isFixed() const { return fixed_; };
bool GridNode::isPlaced() const { return placed_; }
bool GridNode::isHold() const { return hold_; }
dbSite* GridNode::getSite() const
{
  if (!db_inst_ || !db_inst_->getMaster()) {
    return nullptr;
  }
  return db_inst_->getMaster()->getSite();
}
DbuX GridNode::siteWidth() const
{
  if (db_inst_) {
    auto site = db_inst_->getMaster()->getSite();
    if (site) {
      return DbuX{site->getWidth()};
    }
  }
  return DbuX{0};
}
bool GridNode::isHybrid() const
{
  dbSite* site = getSite();
  return site ? site->isHybrid() : false;
}
bool GridNode::isHybridParent() const
{
  dbSite* site = getSite();
  return site ? site->hasRowPattern() : false;
}
int64_t GridNode::area() const
{
  dbMaster* master = db_inst_->getMaster();
  return int64_t(master->getWidth()) * master->getHeight();
}
const char* GridNode::name() const { return db_inst_->getConstName(); }
int GridNode::getBottomPower() const { return powerBot_; }
int GridNode::getTopPower() const { return powerTop_; }
GridNode::Type GridNode::getType() const { return type_; }
bool GridNode::isTerminal() const { return (type_ == TERMINAL); }
bool GridNode::isFiller() const { return (type_ == FILLER); }
bool GridNode::isStdCell() const
{
  if (db_inst_ == nullptr) {
    return false;
  }
  return db_inst_->isCore() || db_inst_->isEndCap();
}
bool GridNode::isBlock() const
{
  return db_inst_ && db_inst_->getMaster()->getType() == dbMasterType::BLOCK;
}
Group* GridNode::getGroup() const { return group_; }
const Rect* GridNode::getRegion() const { return region_; }
Master* GridNode::getMaster() const { return master_; }
bool GridNode::inGroup() const { return group_ != nullptr; }
int GridNode::getNumPins() const { return (int) pins_.size(); }
const std::vector<Pin*>& GridNode::getPins() const { return pins_; }
int GridNode::getGroupId() const { return group_id_; }
void GridNode::setId(int id) { id_ = id; }
void GridNode::setFixed(bool in) { fixed_ = in; }
void GridNode::setDbInst(dbInst* inst) { db_inst_ = inst; }
void GridNode::setLeft(DbuX x) { left_ = x; }
void GridNode::setBottom(DbuY y) { bottom_ = y; }
void GridNode::setOrient(const dbOrientType& in) { orient_ = in; }
void GridNode::setWidth(DbuX width) { width_ = width; }
void GridNode::setHeight(DbuY height) { height_ = height; }
void GridNode::setPlaced(bool in) { placed_ = in; }
void GridNode::setHold(bool in) { hold_ = in; }
void GridNode::setBottomPower(int bot) { powerBot_ = bot; }
void GridNode::setTopPower(int top) { powerTop_ = top; }
void GridNode::setOrigBottom(DbuY bottom) { orig_bottom_ = bottom; }
void GridNode::setOrigLeft(DbuX left) { orig_left_ = left; }
void GridNode::setType(Type type) { type_ = type; }
void GridNode::setGroup(Group* in) { group_ = in; }
void GridNode::setRegion(const Rect* in) { region_ = in; }
void GridNode::setMaster(Master* in) { master_ = in; }
void GridNode::addPin(Pin* pin) { pins_.emplace_back(pin); }
void GridNode::setGroupId(int id) { group_id_ = id; }
bool GridNode::adjustCurrOrient(const dbOrientType& newOri)
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

string Group::getName() { return name_; }
const vector<Rect>& Group::getRects() const { return region_boundaries_; }
const vector<GridNode*> Group::getCells() const { return cells_; }
const Rect& Group::getBBox() const { return boundary_; }
double Group::getUtil() const { return util_; }
int Group::getId() const { return id_; }
void Group::setId(int id) { id_ = id; }
void Group::setName(const string& in) { name_ = in;}
void Group::addRect(const Rect& in) {region_boundaries_.emplace_back(in);}
void Group::addCell(GridNode* cell) { cells_.emplace_back(cell); }
void Group::setBoundary(const Rect& in) { boundary_ = in; }
void Group::setUtil(const double in) { util_ = in; }
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

int Edge::getId() const { return id_; }
void Edge::setId(int id) { id_ = id; }
int Edge::getNumPins() const { return (int) pins_.size(); }
const std::vector<Pin*>& Edge::getPins() const { return pins_; }
void Edge::addPin(Pin* pin) { pins_.emplace_back(pin); }
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

Pin::Pin() = default;
void Pin::setDirection(int dir) { dir_ = dir; }
int Pin::getDirection() const { return dir_; }
void Pin::setNode(GridNode* node) { node_ = node; }
void Pin::setEdge(Edge* ed) { edge_ = ed; }
GridNode* Pin::getNode() const { return node_; }
Edge* Pin::getEdge() const { return edge_; }
void Pin::setOffsetX(DbuX offsetX) { offsetX_ = offsetX; }
DbuX Pin::getOffsetX() const { return offsetX_; }
void Pin::setOffsetY(DbuY offsetY) { offsetY_ = offsetY; }
DbuY Pin::getOffsetY() const { return offsetY_; }
void Pin::setPinLayer(int layer) { pinLayer_ = layer; }
int Pin::getPinLayer() const { return pinLayer_; }
void Pin::setPinWidth(DbuX width) { pinWidth_ = width; }
DbuX Pin::getPinWidth() const { return pinWidth_; }
void Pin::setPinHeight(DbuY height) { pinHeight_ = height; }
DbuY Pin::getPinHeight() const { return pinHeight_; }


}  // namespace dpl
