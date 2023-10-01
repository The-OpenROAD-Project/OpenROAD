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

#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <cfloat>
#include <cmath>
#include <limits>
#include <map>

#include "DplObserver.h"
#include "utl/Logger.h"

namespace dpl {

using std::round;
using std::string;

using utl::DPL;

using odb::dbMasterType;
using odb::dbNet;
using odb::Rect;

const char* Cell::name() const
{
  return db_inst_->getConstName();
}

int64_t Cell::area() const
{
  dbMaster* master = db_inst_->getMaster();
  return int64_t(master->getWidth()) * master->getHeight();
}

////////////////////////////////////////////////////////////////

bool Opendp::isFixed(const Cell* cell) const
{
  return cell == &dummy_cell_ || cell->db_inst_->isFixed();
}

bool Opendp::isMultiRow(const Cell* cell) const
{
  auto iter = db_master_map_.find(cell->db_inst_->getMaster());
  assert(iter != db_master_map_.end());
  return iter->second.is_multi_row;
}

////////////////////////////////////////////////////////////////

Opendp::Opendp()
{
  dummy_cell_.is_placed_ = true;
}

Opendp::~Opendp() = default;

void Opendp::init(dbDatabase* db, Logger* logger)
{
  db_ = db;
  logger_ = logger;
}

void Opendp::initBlock()
{
  block_ = db_->getChip()->getBlock();
  core_ = block_->getCoreArea();
}

void Opendp::setPaddingGlobal(int left, int right)
{
  pad_left_ = left;
  pad_right_ = right;
}

void Opendp::setPadding(dbInst* inst, int left, int right)
{
  inst_padding_map_[inst] = std::make_pair(left, right);
}

void Opendp::setPadding(dbMaster* master, int left, int right)
{
  master_padding_map_[master] = std::make_pair(left, right);
}

bool Opendp::havePadding() const
{
  return pad_left_ > 0 || pad_right_ > 0 || !master_padding_map_.empty()
         || !inst_padding_map_.empty();
}

void Opendp::setDebug(std::unique_ptr<DplObserver>& observer)
{
  debug_observer_ = std::move(observer);
}

void Opendp::detailedPlacement(int max_displacement_x,
                               int max_displacement_y,
                               const std::string& report_file_name,
                               bool disallow_one_site_gaps)
{
  importDb();

  if (have_fillers_) {
    logger_->warn(DPL, 37, "Use remove_fillers before detailed placement.");
  }

  if (max_displacement_x == 0 || max_displacement_y == 0) {
    // defaults
    max_displacement_x_ = 500;
    max_displacement_y_ = 100;
  } else {
    max_displacement_x_ = max_displacement_x;
    max_displacement_y_ = max_displacement_y;
  }
  disallow_one_site_gaps_ = disallow_one_site_gaps;
  if (!have_one_site_cells_) {
    // If 1-site fill cell is not detected && no disallow_one_site_gaps flag:
    // warn the user then continue as normal
    if (!disallow_one_site_gaps_) {
      logger_->warn(DPL,
                    38,
                    "No 1-site fill cells detected.  To remove 1-site gaps use "
                    "the -disallow_one_site_gaps flag.");
    }
  }
  hpwl_before_ = hpwl();
  detailedPlacement();
  // Save displacement stats before updating instance DB locations.
  findDisplacementStats();
  updateDbInstLocations();
  if (!placement_failures_.empty()) {
    logger_->info(DPL,
                  34,
                  "Detailed placement failed on the following {} instances:",
                  placement_failures_.size());
    for (auto cell : placement_failures_) {
      logger_->info(DPL, 35, " {}", cell->name());
    }

    if (!report_file_name.empty()) {
      writeJsonReport(
          report_file_name, {}, {}, {}, {}, {}, {}, placement_failures_);
    }
    logger_->error(DPL, 36, "Detailed placement failed.");
  }
}

void Opendp::updateDbInstLocations()
{
  for (Cell& cell : cells_) {
    if (!isFixed(&cell) && isStdCell(&cell)) {
      dbInst* db_inst_ = cell.db_inst_;
      // Only move the instance if necessary to avoid triggering callbacks.
      if (db_inst_->getOrient() != cell.orient_) {
        db_inst_->setOrient(cell.orient_);
      }
      int x = core_.xMin() + cell.x_;
      int y = core_.yMin() + cell.y_;
      int inst_x, inst_y;
      db_inst_->getLocation(inst_x, inst_y);
      if (x != inst_x || y != inst_y) {
        db_inst_->setLocation(x, y);
      }
    }
  }
}

void Opendp::reportLegalizationStats() const
{
  logger_->report("Placement Analysis");
  logger_->report("---------------------------------");
  logger_->report("total displacement   {:10.1f} u",
                  dbuToMicrons(displacement_sum_));
  logger_->metric("design__instance__displacement__total",
                  dbuToMicrons(displacement_sum_));
  logger_->report("average displacement {:10.1f} u",
                  dbuToMicrons(displacement_avg_));
  logger_->metric("design__instance__displacement__mean",
                  dbuToMicrons(displacement_avg_));
  logger_->report("max displacement     {:10.1f} u",
                  dbuToMicrons(displacement_max_));
  logger_->metric("design__instance__displacement__max",
                  dbuToMicrons(displacement_max_));
  logger_->report("original HPWL        {:10.1f} u",
                  dbuToMicrons(hpwl_before_));
  double hpwl_legal = hpwl();
  logger_->report("legalized HPWL       {:10.1f} u", dbuToMicrons(hpwl_legal));
  logger_->metric("route__wirelength__estimated", dbuToMicrons(hpwl_legal));
  int hpwl_delta
      = (hpwl_before_ == 0.0)
            ? 0.0
            : round((hpwl_legal - hpwl_before_) / hpwl_before_ * 100);
  logger_->report("delta HPWL           {:10} %", hpwl_delta);
  logger_->report("");
}

////////////////////////////////////////////////////////////////

void Opendp::findDisplacementStats()
{
  displacement_avg_ = 0;
  displacement_sum_ = 0;
  displacement_max_ = 0;

  for (const Cell& cell : cells_) {
    int displacement = disp(&cell);
    displacement_sum_ += displacement;
    if (displacement > displacement_max_) {
      displacement_max_ = displacement;
    }
  }
  if (!cells_.empty()) {
    displacement_avg_ = displacement_sum_ / cells_.size();
  } else {
    displacement_avg_ = 0.0;
  }
}

// Note that this does NOT use cell/core coordinates.
int64_t Opendp::hpwl() const
{
  int64_t hpwl_sum = 0;
  for (dbNet* net : block_->getNets()) {
    hpwl_sum += hpwl(net);
  }
  return hpwl_sum;
}

int64_t Opendp::hpwl(dbNet* net) const
{
  if (net->getSigType().isSupply()) {
    return 0;
  }

  Rect bbox = net->getTermBBox();
  return bbox.dx() + bbox.dy();
}

////////////////////////////////////////////////////////////////

Point Opendp::initialLocation(const Cell* cell, bool padded) const
{
  int loc_x, loc_y;
  cell->db_inst_->getLocation(loc_x, loc_y);
  int site_width = getSiteWidth(cell);
  loc_x -= core_.xMin();
  if (padded) {
    loc_x -= padLeft(cell) * site_width;
  }
  loc_y -= core_.yMin();
  return Point(loc_x, loc_y);
}

int Opendp::disp(const Cell* cell) const
{
  Point init = initialLocation(cell, false);
  return abs(init.getX() - cell->x_) + abs(init.getY() - cell->y_);
}

bool Opendp::isPaddedType(dbInst* inst) const
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
    case dbMasterType::ENDCAP_LEF58_RIGHTEDGE:
    case dbMasterType::ENDCAP_LEF58_LEFTEDGE:
      return true;
    case dbMasterType::CORE_SPACER:
    case dbMasterType::BLOCK:
    case dbMasterType::BLOCK_BLACKBOX:
    case dbMasterType::BLOCK_SOFT:
    case dbMasterType::ENDCAP_TOPLEFT:
    case dbMasterType::ENDCAP_TOPRIGHT:
    case dbMasterType::ENDCAP_BOTTOMLEFT:
    case dbMasterType::ENDCAP_BOTTOMRIGHT:
    case dbMasterType::ENDCAP_LEF58_BOTTOMEDGE:
    case dbMasterType::ENDCAP_LEF58_TOPEDGE:
    case dbMasterType::ENDCAP_LEF58_RIGHTBOTTOMEDGE:
    case dbMasterType::ENDCAP_LEF58_LEFTBOTTOMEDGE:
    case dbMasterType::ENDCAP_LEF58_RIGHTTOPEDGE:
    case dbMasterType::ENDCAP_LEF58_LEFTTOPEDGE:
    case dbMasterType::ENDCAP_LEF58_RIGHTBOTTOMCORNER:
    case dbMasterType::ENDCAP_LEF58_LEFTBOTTOMCORNER:
    case dbMasterType::ENDCAP_LEF58_RIGHTTOPCORNER:
    case dbMasterType::ENDCAP_LEF58_LEFTTOPCORNER:
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

bool Opendp::isStdCell(const Cell* cell) const
{
  if (cell->db_inst_ == nullptr) {
    return false;
  }
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
    case dbMasterType::ENDCAP_LEF58_BOTTOMEDGE:
    case dbMasterType::ENDCAP_LEF58_TOPEDGE:
    case dbMasterType::ENDCAP_LEF58_RIGHTEDGE:
    case dbMasterType::ENDCAP_LEF58_LEFTEDGE:
    case dbMasterType::ENDCAP_LEF58_RIGHTBOTTOMEDGE:
    case dbMasterType::ENDCAP_LEF58_LEFTBOTTOMEDGE:
    case dbMasterType::ENDCAP_LEF58_RIGHTTOPEDGE:
    case dbMasterType::ENDCAP_LEF58_LEFTTOPEDGE:
    case dbMasterType::ENDCAP_LEF58_RIGHTBOTTOMCORNER:
    case dbMasterType::ENDCAP_LEF58_LEFTBOTTOMCORNER:
    case dbMasterType::ENDCAP_LEF58_RIGHTTOPCORNER:
    case dbMasterType::ENDCAP_LEF58_LEFTTOPCORNER:
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
bool Opendp::isBlock(const Cell* cell)
{
  dbMasterType type = cell->db_inst_->getMaster()->getType();
  return type == dbMasterType::BLOCK;
}

int Opendp::padLeft(const Cell* cell) const
{
  return padLeft(cell->db_inst_);
}

int Opendp::padLeft(dbInst* inst) const
{
  if (isPaddedType(inst)) {
    auto itr1 = inst_padding_map_.find(inst);
    if (itr1 != inst_padding_map_.end()) {
      return itr1->second.first;
    }
    auto itr2 = master_padding_map_.find(inst->getMaster());
    if (itr2 != master_padding_map_.end()) {
      return itr2->second.first;
    }
    return pad_left_;
  }
  return 0;
}

int Opendp::padRight(const Cell* cell) const
{
  return padRight(cell->db_inst_);
}

int Opendp::padRight(dbInst* inst) const
{
  if (isPaddedType(inst)) {
    auto itr1 = inst_padding_map_.find(inst);
    if (itr1 != inst_padding_map_.end()) {
      return itr1->second.second;
    }
    auto itr2 = master_padding_map_.find(inst->getMaster());
    if (itr2 != master_padding_map_.end()) {
      return itr2->second.second;
    }
    return pad_right_;
  }
  return 0;
}

int Opendp::paddedWidth(const Cell* cell, int site_width) const
{
  return cell->width_ + (padLeft(cell) + padRight(cell)) * site_width;
}

int Opendp::paddedWidth(const Cell* cell) const
{
  int site_width = getSiteWidth(cell);
  return cell->width_ + (padLeft(cell) + padRight(cell)) * site_width;
}

int Opendp::gridPaddedWidth(const Cell* cell, int site_width) const
{
  return divCeil(paddedWidth(cell), site_width);
}

int Opendp::gridPaddedWidth(const Cell* cell) const
{
  int site_width = getSiteWidth(cell);
  return divCeil(paddedWidth(cell), site_width);
}

int Opendp::gridHeight(const Cell* cell, int row_height) const
{
  return std::max(1, divCeil(cell->height_, row_height));
}

int Opendp::gridHeight(const Cell* cell) const
{
  int row_height = getRowHeight(cell);
  // TODO: this is no longer correct for hybrid cells
  return std::max(1, divCeil(cell->height_, row_height));
}

int64_t Opendp::paddedArea(const Cell* cell) const
{
  return int64_t(paddedWidth(cell)) * cell->height_;
}

// Callers should probably be using gridPaddedWidth.
int Opendp::gridNearestWidth(const Cell* cell, int site_width) const
{
  return divRound(paddedWidth(cell), site_width);
}

int Opendp::gridNearestWidth(const Cell* cell) const
{
  int site_width = getSiteWidth(cell);
  return divRound(paddedWidth(cell), site_width);
}

// Callers should probably be using gridHeight.
int Opendp::gridNearestHeight(const Cell* cell, int row_height) const
{
  return divRound(cell->height_, row_height);
}

int Opendp::gridNearestHeight(const Cell* cell) const
{
  int row_height = getRowHeight(cell);
  return divRound(cell->height_, row_height);
}

int Opendp::gridEndX(int x, int site_width) const
{
  return divCeil(x, site_width);
}

int Opendp::gridY(int y, int row_height) const
{
  return y / row_height;
}

int Opendp::gridEndY(int y, int row_height) const
{
  return divCeil(y, row_height);
}

int Opendp::gridX(int x, int site_width) const
{
  return x / site_width;
}

int Opendp::gridX(const Cell* cell, int site_width) const
{
  return gridX(cell->x_, site_width);
}

int Opendp::gridX(const Cell* cell) const
{
  return gridX(cell->x_, getSiteWidth(cell));
}

int Opendp::gridPaddedX(const Cell* cell) const
{
  return gridX(cell->x_ - padLeft(cell) * getSiteWidth(cell),
               getSiteWidth(cell));
}

int Opendp::gridPaddedX(const Cell* cell, int site_width) const
{
  return gridX(cell->x_ - padLeft(cell) * site_width, site_width);
}

int Opendp::getRowCount(const Cell* cell) const
{
  return getRowCount(getRowHeight(cell));
}

int Opendp::getRowCount(int row_height) const
{
  return divFloor(core_.dy(), row_height);
}

int Opendp::getRowHeight(const Cell* cell) const
{
  int row_height = row_height_;
  if (isStdCell(cell) || cell->isHybrid()) {
    row_height = cell->height_;
  }
  return row_height;
}

pair<int, GridInfo> Opendp::getRowInfo(const Cell* cell) const
{
  if (grid_info_map_.empty()) {
    logger_->error(DPL, 43, "No grid layers mapped.");
  }
  Grid_map_key key = getGridMapKey(cell);
  auto layer = grid_info_map_.find(key);
  if (layer == grid_info_map_.end()) {
    // this means the cell is taller than any layer
    logger_->error(DPL,
                   44,
                   "Cell {} with height {} is taller than any row.",
                   cell->name(),
                   cell->height_);
  }
  return std::make_pair(cell->height_, layer->second);
}

Grid_map_key Opendp::getGridMapKey(const dbSite* site) const
{
  Grid_map_key gmk;
  auto grid_idx = site_idx_to_grid_idx.find(site->getId());
  if (grid_idx == site_idx_to_grid_idx.end()) {
    logger_->error(
        DPL, 46, "Site {} is not mapped to a grid.", site->getName());
  }
  gmk.grid_index = grid_idx->second;
  return gmk;
}

Grid_map_key Opendp::getGridMapKey(const Cell* cell) const
{
  if (cell == nullptr) {
    logger_->error(DPL, 5211, "getGridMapKey cell is null");
  }
  auto site = cell->getSite();
  if (!isStdCell(cell)) {
    // non std cells can go to the first grid.
    return smallest_non_hybrid_grid_key;
  }
  if (site == nullptr) {
    logger_->error(DPL, 4219, "Cell {} has no site.", cell->name());
  }
  return this->getGridMapKey(site);
}

GridInfo Opendp::getGridInfo(const Cell* cell) const
{
  return grid_info_map_.at(getGridMapKey(cell));
}

int Opendp::getSiteWidth(const Cell* cell) const
{
  // TODO: this is not complete, it is here so that future changes to the code
  // are easier
  return site_width_;
}

pair<int, int> Opendp::gridY(
    int y,
    const std::vector<std::pair<dbSite*, dbOrientType>>& grid_sites) const
{
  int sum_heights = std::accumulate(
      grid_sites.begin(),
      grid_sites.end(),
      0,
      [](int sum, const std::pair<dbSite*, dbOrientType>& entry) {
        return sum + entry.first->getHeight();
      });

  int base_height_index = divFloor(y, sum_heights);
  int cur_height = base_height_index * sum_heights;
  int index = 0;
  base_height_index *= grid_sites.size();
  while (cur_height < y && index < grid_sites.size()) {
    auto site = grid_sites.at(index);
    if (cur_height + site.first->getHeight() > y)
      break;
    cur_height += site.first->getHeight();
    index++;
  }
  return {base_height_index + index, cur_height};
}

pair<int, int> Opendp::gridEndY(
    int y,
    const std::vector<std::pair<dbSite*, dbOrientType>>& grid_sites) const
{
  int sum_heights = std::accumulate(
      grid_sites.begin(),
      grid_sites.end(),
      0,
      [](int sum, const std::pair<dbSite*, dbOrientType>& entry) {
        return sum + entry.first->getHeight();
      });

  int base_height_index = divFloor(y, sum_heights);
  int cur_height = base_height_index * sum_heights;
  int index = 0;
  base_height_index *= grid_sites.size();
  while (cur_height < y && index < grid_sites.size()) {
    auto site = grid_sites.at(index);
    cur_height += site.first->getHeight();
    index++;
  }
  return {base_height_index + index, cur_height};
}

int Opendp::gridY(const Cell* cell) const
{
  if (cell->isHybrid()) {
    auto grid_info = getGridInfo(cell);
    return gridY(cell->y_, grid_info.getSites()).first;
  }
  int row_height = getRowHeight(cell);

  return cell->y_ / row_height;
}

void Opendp::setGridPaddedLoc(Cell* cell, int x, int y, int site_width) const
{
  cell->x_ = (x + padLeft(cell)) * site_width;
  if (cell->isHybrid()) {
    auto grid_info = grid_info_map_.at(getGridMapKey(cell));
    int total_sites_height = grid_info.getSitesTotalHeight();
    auto sites = grid_info.getSites();
    const int sites_size = sites.size();
    int height = (y / sites_size) * total_sites_height;
    for (int s = 0; s < y % sites_size; s++) {
      height += sites[s].first->getHeight();
    }
    cell->y_ = height;
    return;
  }
  cell->y_ = y * getRowHeight(cell);
}

int Opendp::gridPaddedEndX(const Cell* cell, int site_width) const
{
  return divCeil(cell->x_ + cell->width_ + padRight(cell) * site_width,
                 site_width);
}

int Opendp::gridPaddedEndX(const Cell* cell) const
{
  int site_width = getSiteWidth(cell);
  return divCeil(cell->x_ + cell->width_ + padRight(cell) * site_width,
                 site_width);
}

int Opendp::gridEndX(const Cell* cell, int site_width) const
{
  return divCeil(cell->x_ + cell->width_, site_width);
}

int Opendp::gridEndX(const Cell* cell) const
{
  int site_width = getSiteWidth(cell);
  return divCeil(cell->x_ + cell->width_, site_width);
}

int Opendp::gridEndY(const Cell* cell) const
{
  if (cell->isHybrid()) {
    auto grid_info = getGridInfo(cell);
    auto grid_sites = grid_info.getSites();
    return gridY(cell->y_ + cell->height_, grid_sites).first;
  }
  int row_height = getRowHeight(cell);
  return divCeil(cell->y_ + cell->height_, row_height);
}

double Opendp::dbuToMicrons(int64_t dbu) const
{
  double dbu_micron = db_->getTech()->getDbUnitsPerMicron();
  return dbu / dbu_micron;
}

double Opendp::dbuAreaToMicrons(int64_t dbu_area) const
{
  double dbu_micron = db_->getTech()->getDbUnitsPerMicron();
  return dbu_area / (dbu_micron * dbu_micron);
}

int divRound(int dividend, int divisor)
{
  return round(static_cast<double>(dividend) / divisor);
}

int divCeil(int dividend, int divisor)
{
  return ceil(static_cast<double>(dividend) / divisor);
}

int divFloor(int dividend, int divisor)
{
  return dividend / divisor;
}

}  // namespace dpl
