/////////////////////////////////////////////////////////////////////////////
// Original authors: SangGi Do(sanggido@unist.ac.kr), Mingyu
// Woo(mwoo@eng.ucsd.edu)
//          (respective Ph.D. advisors: Seokhyeong Kang, Andrew B. Kahng)
// Rewrite by James Cherry, Parallax Software, Inc.
//
// Copyright (c) 2019, The Regents of the University of California
// Copyright (c) 2018, SangGi Do and Mingyu Woo
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

#include "dpl/Opendp.h"

#include <cfloat>
#include <cmath>
#include <limits>
#include <map>

#include "utl/Logger.h"
#include "ord/OpenRoad.hh"  // closestPtInRect

namespace dpl {

using std::round;
using std::string;

using utl::DPL;

using odb::dbBox;
using odb::dbBPin;
using odb::dbBTerm;
using odb::dbITerm;
using odb::dbMasterType;
using odb::dbMPin;
using odb::dbMTerm;
using odb::dbNet;
using odb::dbPlacementStatus;
using odb::Rect;
using odb::dbSigType;

Cell::Cell() :
  db_inst_(nullptr),
  x_(0),
  y_(0),
  width_(0),
  height_(0),
  is_placed_(false),
  hold_(false),
  group_(nullptr),
  region_(nullptr)
{
}

const char *
Cell::name() const
{
  return db_inst_->getConstName();
}

int64_t
Cell::area() const
{
  dbMaster *master = db_inst_->getMaster();
  return master->getWidth() * master->getHeight();
}

////////////////////////////////////////////////////////////////

bool
Opendp::isFixed(const Cell *cell) const
{
  return cell == &dummy_cell_ || cell->db_inst_->isFixed();
}

bool
Opendp::isMultiRow(const Cell *cell) const
{
  auto iter = db_master_map_.find(cell->db_inst_->getMaster());
  assert(iter != db_master_map_.end());
  return iter->second.is_multi_row_;
}

Power
Opendp::topPower(const Cell *cell) const
{
  auto iter = db_master_map_.find(cell->db_inst_->getMaster());
  assert(iter != db_master_map_.end());
  return iter->second.top_power_;
}

////////////////////////////////////////////////////////////////

Group::Group() :
  util(0.0)
{
}

Opendp::Opendp() :
  pad_left_(0),
  pad_right_(0),
  max_displacement_x_(0),
  max_displacement_y_(0),
  grid_(nullptr)
{
  dummy_cell_.is_placed_ = true;
}

Opendp::~Opendp()
{
  deleteGrid();
}

void
Opendp::init(dbDatabase *db,
             Logger *logger)
{
  db_ = db;
  logger_ = logger;
}

void
Opendp::setPaddingGlobal(int left,
                         int right)
{
  pad_left_ = left;
  pad_right_ = right;
}

void
Opendp::setPadding(dbInst *inst,
                   int left,
                   int right)
{
  inst_padding_map_[inst] = std::make_pair(left, right);
}

void
Opendp::setPadding(dbMaster *master,
                   int left,
                   int right)
{
  master_padding_map_[master] = std::make_pair(left, right);
}

bool
Opendp::havePadding() const
{
  return pad_left_ > 0 || pad_right_ > 0
    || !master_padding_map_.empty()
    || !inst_padding_map_.empty();
}

void
Opendp::detailedPlacement(int max_displacement_x,
                          int max_displacement_y)
{
  importDb();

  if (max_displacement_x == 0
      || max_displacement_y == 0) {
    // defaults
    max_displacement_x_ = 500;
    max_displacement_y_ = 100;
  }
  else {
    max_displacement_x_ = max_displacement_x;
    max_displacement_y_ = max_displacement_y;
  }

  reportImportWarnings();
  hpwl_before_ = hpwl();
  detailedPlacement();
  // Save displacement stats before updating instance DB locations.
  findDisplacementStats();
  updateDbInstLocations();
}

void
Opendp::updateDbInstLocations()
{
  for (Cell &cell : cells_) {
    if (!isFixed(&cell) && isStdCell(&cell)) {
      dbInst *db_inst_ = cell.db_inst_;
      // Only move the instance if necessary to avoid triggering callbacks.
      if (db_inst_->getOrient() != cell.orient_)
        db_inst_->setOrient(cell.orient_);
      int x = core_.xMin() + cell.x_;
      int y = core_.yMin() + cell.y_;
      int inst_x, inst_y;
      db_inst_->getLocation(inst_x, inst_y);
      if (x != inst_x || y != inst_y)
        db_inst_->setLocation(x, y);
    }
  }
}

void
Opendp::reportLegalizationStats() const
{
  logger_->report("Placement Analysis");
  logger_->report("---------------------------------");
  logger_->report("total displacement   {:10.1f} u", dbuToMicrons(displacement_sum_));
  logger_->report("average displacement {:10.1f} u", dbuToMicrons(displacement_avg_));
  logger_->report("max displacement     {:10.1f} u", dbuToMicrons(displacement_max_));
  logger_->report("original HPWL        {:10.1f} u", dbuToMicrons(hpwl_before_));
  double hpwl_legal = hpwl();
  logger_->report("legalized HPWL       {:10.1f} u", dbuToMicrons(hpwl_legal));
  int hpwl_delta = (hpwl_before_ == 0.0)
    ? 0.0
    : round((hpwl_legal - hpwl_before_) / hpwl_before_ * 100);
  logger_->report("delta HPWL           {:10} %", hpwl_delta);
  logger_->report("");
}

////////////////////////////////////////////////////////////////

void
Opendp::findDisplacementStats()
{
  displacement_avg_ = 0;
  displacement_sum_ = 0;
  displacement_max_ = 0;

  for (const Cell &cell : cells_) {
    int displacement = disp(&cell);
    displacement_sum_ += displacement;
    if (displacement > displacement_max_) {
      displacement_max_ = displacement;
    }
  }
  if (cells_.size())
    displacement_avg_ = displacement_sum_ / cells_.size();
  else
    displacement_avg_ = 0.0;
}

// Note that this does NOT use cell/core coordinates.
int64_t
Opendp::hpwl() const
{
  int64_t hpwl_sum = 0;
  for (dbNet *net : block_->getNets())
    hpwl_sum += hpwl(net);
  return hpwl_sum;
}

int64_t
Opendp::hpwl(dbNet *net) const
{
  if (isSupply(net))
    return 0;
  else {
    Rect bbox = getBox(net);
    return bbox.dx() + bbox.dy();
  }
}

bool
Opendp::isSupply(dbNet *net) const
{
  dbSigType sig_type = net->getSigType();
  return sig_type == dbSigType::POWER
    || sig_type == dbSigType::GROUND;
}

Rect
Opendp::getBox(dbNet *net) const
{
  Rect net_box;
  net_box.mergeInit();

  for (dbITerm *iterm : net->getITerms()) {
    int x, y;
    if (iterm->getAvgXY(&x, &y)) {
      Rect iterm_rect(x, y, x, y);
      net_box.merge(iterm_rect);
    }
    else {
      // This clause is sort of worthless because getAvgXY prints
      // a warning when it fails.
      dbInst *inst = iterm->getInst();
      dbBox *inst_box = inst->getBBox();
      int center_x = (inst_box->xMin() + inst_box->xMax()) / 2;
      int center_y = (inst_box->yMin() + inst_box->yMax()) / 2;
      Rect inst_center(center_x, center_y, center_x, center_y);
      net_box.merge(inst_center);
    }
  }

  for (dbBTerm *bterm : net->getBTerms()) {
    for (dbBPin *bpin : bterm->getBPins()) {
      dbPlacementStatus status = bpin->getPlacementStatus();
      if (status.isPlaced()) {
        Rect pin_bbox = bpin->getBBox();
        int center_x = (pin_bbox.xMin() + pin_bbox.xMax()) / 2;
        int center_y = (pin_bbox.yMin() + pin_bbox.yMax()) / 2;
        Rect pin_center(center_x, center_y, center_x, center_y);
        net_box.merge(pin_center);
      }
    }
  }
  return net_box;
}

////////////////////////////////////////////////////////////////

Power
Opendp::rowTopPower(int row) const
{
  return ((row0_top_power_is_vdd_ ? row : row + 1) % 2 == 0) ? VDD : VSS;
}

dbOrientType
Opendp::rowOrient(int row) const
{
  // Row orient flips R0 -> MX -> R0 -> MX ...
  return ((row0_orient_is_r0_ ? row : row + 1) % 2 == 0) ? dbOrientType::R0
                                                         : dbOrientType::MX;
}

////////////////////////////////////////////////////////////////

Point
Opendp::initialLocation(const Cell *cell,
                        bool padded) const
{
  int loc_x, loc_y;
  cell->db_inst_->getLocation(loc_x, loc_y);
  loc_x -= core_.xMin();
  if (padded)
    loc_x -= padLeft(cell) * site_width_;
  loc_y -= core_.yMin();
  return Point(loc_x, loc_y);
}

int
Opendp::disp(const Cell *cell) const
{
  Point init = initialLocation(cell, false);
  return abs(init.getX() - cell->x_) + abs(init.getY() - cell->y_);
}

bool
Opendp::isPaddedType(dbInst *inst) const
{
  dbMasterType type = inst->getMaster()->getType();
  // Use switch so if new types are added we get a compiler warning.
  switch (type) {
    case dbMasterType::CORE:
    case dbMasterType::CORE_ANTENNACELL:
    case dbMasterType::CORE_FEEDTHRU:
    case dbMasterType::CORE_TIEHIGH:
    case dbMasterType::CORE_TIELOW:
    case dbMasterType::CORE_WELLTAP:
    case dbMasterType::ENDCAP:
    case dbMasterType::ENDCAP_PRE:
    case dbMasterType::ENDCAP_POST:
      return true;
    case dbMasterType::CORE_SPACER:
    case dbMasterType::BLOCK:
    case dbMasterType::BLOCK_BLACKBOX:
    case dbMasterType::BLOCK_SOFT:
    case dbMasterType::ENDCAP_TOPLEFT:
    case dbMasterType::ENDCAP_TOPRIGHT:
    case dbMasterType::ENDCAP_BOTTOMLEFT:
    case dbMasterType::ENDCAP_BOTTOMRIGHT:
      // These classes are completely ignored by the placer.
    case dbMasterType::COVER:
    case dbMasterType::COVER_BUMP:
    case dbMasterType::RING:
    case dbMasterType::PAD:
    case dbMasterType::PAD_AREAIO:
    case dbMasterType::PAD_INPUT:
    case dbMasterType::PAD_OUTPUT:
    case dbMasterType::PAD_INOUT:
    case dbMasterType::PAD_POWER:
    case dbMasterType::PAD_SPACER:
    case dbMasterType::NONE:
      return false;
  }
  // gcc warniing
  return false;
}

bool
Opendp::isStdCell(const Cell *cell) const
{
  dbMasterType type = cell->db_inst_->getMaster()->getType();
  // Use switch so if new types are added we get a compiler warning.
  switch (type) {
    case dbMasterType::CORE:
    case dbMasterType::CORE_ANTENNACELL:
    case dbMasterType::CORE_FEEDTHRU:
    case dbMasterType::CORE_TIEHIGH:
    case dbMasterType::CORE_TIELOW:
    case dbMasterType::CORE_SPACER:
    case dbMasterType::CORE_WELLTAP:
      return true;
    case dbMasterType::BLOCK:
    case dbMasterType::BLOCK_BLACKBOX:
    case dbMasterType::BLOCK_SOFT:
    case dbMasterType::ENDCAP:
    case dbMasterType::ENDCAP_PRE:
    case dbMasterType::ENDCAP_POST:
    case dbMasterType::ENDCAP_TOPLEFT:
    case dbMasterType::ENDCAP_TOPRIGHT:
    case dbMasterType::ENDCAP_BOTTOMLEFT:
    case dbMasterType::ENDCAP_BOTTOMRIGHT:
      // These classes are completely ignored by the placer.
    case dbMasterType::COVER:
    case dbMasterType::COVER_BUMP:
    case dbMasterType::RING:
    case dbMasterType::PAD:
    case dbMasterType::PAD_AREAIO:
    case dbMasterType::PAD_INPUT:
    case dbMasterType::PAD_OUTPUT:
    case dbMasterType::PAD_INOUT:
    case dbMasterType::PAD_POWER:
    case dbMasterType::PAD_SPACER:
    case dbMasterType::NONE:
      return false;
  }
  // gcc warniing
  return false;
}

/* static */
bool
Opendp::isBlock(const Cell *cell)
{
  dbMasterType type = cell->db_inst_->getMaster()->getType();
  return type == dbMasterType::BLOCK;
}

int
Opendp::gridEndX() const
{
  return gridEndX(core_.dx());
}

int
Opendp::gridEndY() const
{
  return gridEndY(core_.dy());
}

int
Opendp::padLeft(const Cell *cell) const
{
  return padLeft(cell->db_inst_);
}

int
Opendp::padLeft(dbInst *inst) const
{
  if (isPaddedType(inst)) {
    auto itr1 = inst_padding_map_.find(inst);
    if (itr1 != inst_padding_map_.end())
      return itr1->second.first;
    auto itr2 = master_padding_map_.find(inst->getMaster());
    if (itr2 != master_padding_map_.end())
      return itr2->second.first;
    else
      return pad_left_;
  }
  else
    return 0;
}

int
Opendp::padRight(const Cell *cell) const
{
  return padRight(cell->db_inst_);
}

int
Opendp::padRight(dbInst *inst) const
{
  if (isPaddedType(inst)) {
    auto itr1 = inst_padding_map_.find(inst);
    if (itr1 != inst_padding_map_.end())
      return itr1->second.second;
    auto itr2 = master_padding_map_.find(inst->getMaster());
    if (itr2 != master_padding_map_.end())
      return itr2->second.second;
    else
      return pad_right_;
  }
  else
    return 0;
}

int
Opendp::paddedWidth(const Cell *cell) const
{
  return cell->width_ + (padLeft(cell) + padRight(cell)) * site_width_;
}

int
Opendp::gridPaddedWidth(const Cell *cell) const
{
  return divCeil(paddedWidth(cell), site_width_);
}

int
Opendp::gridHeight(const Cell *cell) const
{
  return divCeil(cell->height_, row_height_);
}

int64_t
Opendp::paddedArea(const Cell *cell) const
{
  return paddedWidth(cell) * cell->height_;
}

// Callers should probably be using gridPaddedWidth.
int
Opendp::gridNearestWidth(const Cell *cell) const
{
  return divRound(paddedWidth(cell), site_width_);
}

// Callers should probably be using gridHeight.
int
Opendp::gridNearestHeight(const Cell *cell) const
{
  return divRound(cell->height_, row_height_);
}

int
Opendp::gridX(int x) const
{
  return x / site_width_;
}

int
Opendp::gridEndX(int x) const
{
  return divCeil(x, site_width_);
}

int
Opendp::gridY(int y) const
{
  return y / row_height_;
}

int
Opendp::gridEndY(int y) const
{
  return divCeil(y, row_height_);
}

int
Opendp::gridX(const Cell *cell) const
{
  return gridX(cell->x_);
}

int
Opendp::gridPaddedX(const Cell *cell) const
{
  return gridX(cell->x_ - padLeft(cell) * site_width_);
}

int
Opendp::gridY(const Cell *cell) const
{
  return gridY(cell->y_);
}

void
Opendp::setGridPaddedLoc(Cell *cell, int x, int y) const
{
  cell->x_ = (x + padLeft(cell)) * site_width_;
  cell->y_ = y * row_height_;
}

int
Opendp::gridPaddedEndX(const Cell *cell) const
{
  return divCeil(cell->x_ + cell->width_ + padRight(cell) * site_width_,
                 site_width_);
}

int
Opendp::gridEndX(const Cell *cell) const
{
  return divCeil(cell->x_ + cell->width_, site_width_);
}

int
Opendp::gridEndY(const Cell *cell) const
{
  return divCeil(cell->y_ + cell->height_, row_height_);
}

double
Opendp::dbuToMicrons(int64_t dbu) const
{
  double dbu_micron = db_->getTech()->getDbUnitsPerMicron();
  return dbu / dbu_micron;
}

double
Opendp::dbuAreaToMicrons(int64_t dbu_area) const
{
  double dbu_micron = db_->getTech()->getDbUnitsPerMicron();
  return dbu_area / (dbu_micron * dbu_micron);
}

int
divRound(int dividend, int divisor)
{
  return round(static_cast<double>(dividend) / divisor);
}

int
divCeil(int dividend, int divisor)
{
  return ceil(static_cast<double>(dividend) / divisor);
}

int
divFloor(int dividend, int divisor)
{
  return dividend / divisor;
}

}  // namespace opendp
