/////////////////////////////////////////////////////////////////////////////
// Original authors: SangGi Do(sanggido@unist.ac.kr), Mingyu Woo(mwoo@eng.ucsd.edu)
//          (respective Ph.D. advisors: Seokhyeong Kang, Andrew B. Kahng)
// Rewrite by James Cherry, Parallax Software, Inc.

// BSD 3-Clause License
//
// Copyright (c) 2019, James Cherry, Parallax Software, Inc.
// Copyright (c) 2018, SangGi Do and Mingyu Woo
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
///////////////////////////////////////////////////////////////////////////////

#include <cfloat>
#include <fstream>
#include <ostream>
#include <iomanip>
#include <limits>
#include <cmath>
#include <map>
#include "openroad/Error.hh"
#include "opendp/Opendp.h"

namespace opendp {

using std::cerr;
using std::cout;
using std::endl;
using std::fixed;
using std::ifstream;
using std::make_pair;
using std::ofstream;
using std::pair;
using std::setprecision;
using std::string;
using std::to_string;
using std::vector;
using std::round;
using std::max;

using ord::error;

using odb::adsRect;
using odb::dbBox;
using odb::dbITerm;
using odb::dbMasterType;
using odb::dbMPin;
using odb::dbMTerm;
using odb::dbPlacementStatus;

Cell::Cell()
  : hold_(false),
    group_(nullptr),
    region_(nullptr) {}

const char *Cell::name() {
  return db_inst_->getConstName();
}

int64_t
Cell::area()
{
  dbMaster *master = db_inst_->getMaster();
  return master->getWidth() * master->getHeight();
}

////////////////////////////////////////////////////////////////

bool Opendp::isFixed(Cell *cell) {
  return cell == &dummy_cell_ ||
         cell->db_inst_->getPlacementStatus() == dbPlacementStatus::FIRM ||
         cell->db_inst_->getPlacementStatus() == dbPlacementStatus::LOCKED ||
         cell->db_inst_->getPlacementStatus() == dbPlacementStatus::COVER;
}

bool
Opendp::isMultiRow(Cell *cell)
{
  return db_master_map_[cell->db_inst_->getMaster()].is_multi_row_;
}

power
Opendp::topPower(Cell *cell)
{
  return db_master_map_[cell->db_inst_->getMaster()].top_power_;
}

////////////////////////////////////////////////////////////////

Group::Group() : name(""), util(0.0){}

Opendp::Opendp()
  : pad_right_(0),
    pad_left_(0),
    grid_(nullptr)
{
}

Opendp::~Opendp() {
  deleteGrid(grid_);
}

void Opendp::init(dbDatabase *db) { db_ = db; }

void Opendp::clear() {
  // magic number alert
  diamond_search_height_ = 400;
  max_displacement_constraint_ = 0;

  db_master_map_.clear();
  cells_.clear();
  groups_.clear();
  db_master_map_.clear();
  db_inst_map_.clear();
  deleteGrid(grid_);
  grid_ = nullptr;
}

void Opendp::setPaddingGlobal(int left,
			      int right) {
  pad_left_ = left;
  pad_right_ = right;
}

void Opendp::detailedPlacement(int max_displacment) {
  clear();
  max_displacement_constraint_ = max_displacment;
  dbToOpendp();
  initAfterImport();
  reportDesignStats();
  detailedPlacement();
  reportLegalizationStats();
  updateDbInstLocations();
}

void Opendp::initAfterImport() {
  findDesignStats();
  power_mapping();

  // dummy cell generation
  dummy_cell_.is_placed_ = true;

  // Make pixel grid
  grid_ = makeGrid();

  // Fragmented Row Handling
  for(auto db_row : block_->getRows()) {
    int orig_x, orig_y;
    db_row->getOrigin(orig_x, orig_y);

    int x_start = (orig_x - core_.xMin()) / site_width_;
    int y_start = (orig_y - core_.yMin()) / row_height_;

    int x_end = x_start + db_row->getSiteCount();
    int y_end = y_start + 1;

    for(int i = x_start; i < x_end; i++) {
      for(int j = y_start; j < y_end; j++) {
        grid_[j][i].is_valid = true;
      }
    }
  }

  // fixed cell marking
  fixed_cell_assign();
  // group id mapping & x_axis dummycell insertion
  group_pixel_assign2();
  // y axis dummycell insertion
  group_pixel_assign();
}

Grid *
Opendp::makeGrid()
{
  Grid *grid = new Pixel*[row_count_];
  for(int i = 0; i < row_count_; i++) {
    grid[i] = new Pixel[row_site_count_];

    for(int j = 0; j < row_site_count_; j++) {
      Pixel &pixel = grid[i][j];
      pixel.grid_y_ = i;
      pixel.grid_x_ = j;
      pixel.cell = nullptr;
      pixel.group_ = nullptr;
      pixel.util = 0.0;
      pixel.is_valid = false;
    }
  }
  return grid;
}

void
Opendp::deleteGrid(Grid *grid)
{
  if (grid) {
    for(int i = 0; i < row_count_; i++) {
      delete [] grid[i];
    }
    delete grid;
  }
}

void Opendp::updateDbInstLocations() {
  for (Cell &cell : cells_) {
    int x = core_.xMin() + cell.x_;
    int y = core_.yMin() + cell.y_;
    dbInst *db_inst_ = cell.db_inst_;
    db_inst_->setOrient(cell.orient_);
    db_inst_->setLocation(x, y);
  }
}

void Opendp::findDesignStats() {
  fixed_inst_count_ = 0;
  multi_height_inst_count_ = 0;
  movable_area_ = fixed_area_ = 0;
  max_cell_height_ = 0;

  for(Cell &cell : cells_) {
    dbMaster *master = cell.db_inst_->getMaster();
    int64_t cell_area = cell.area();
    if(isFixed(&cell)) {
      fixed_area_ += cell_area;
      fixed_inst_count_++;
    }
    else {
      movable_area_ += cell_area;
      if(isMultiRow(&cell))
	multi_height_inst_count_++;

      int cell_height = gridNearestHeight(&cell);
      if(cell_height > max_cell_height_)
	max_cell_height_ = cell_height;
    }
  }

  design_area_ = row_count_ * static_cast< int64_t >(row_site_count_)
    * site_width_ * row_height_;

  design_util_ =
      static_cast< double >(movable_area_) / (design_area_ - fixed_area_);

  if(design_util_ > 1.0) {
    error("utilization exceeds 100%.");
  }
}

void Opendp::reportDesignStats() {
  cout.precision(3);
  cout << "-------------------- Design Stats ------------------------------" << endl;
  cout << "core area                  : (" << core_.xMin() << ", " << core_.yMin()
       << ") (" << core_.xMax() << ", " << core_.yMax() << ")" << endl;
  cout << "total cells                : " << block_->getInsts().size() << endl;
  cout << "multi cells                : " << multi_height_inst_count_ << endl;
  cout << "fixed cells                : " << fixed_inst_count_ << endl;
  cout << "nets                       : " << block_->getNets().size() << endl;

  cout << "design area                : " << static_cast< double >(design_area_) << endl;
  cout << "total fixed area           : " << static_cast< double >(fixed_area_) << endl;
  cout << "total movable area         : " << static_cast< double >(movable_area_) << endl;
  cout << "design utilization         : " << design_util_ * 100.00 << "%" << endl;
  cout << "rows                       : " << row_count_ << endl;
  cout << "row height                 : " << row_height_ << endl;
  if(max_cell_height_ > 1)
    cout << "max multi_cell height      : " << max_cell_height_ << endl;
  if(groups_.size() > 0)
  cout << "group count                : " << groups_.size() << endl;
  cout << "----------------------------------------------------------------" << endl;
}

void Opendp::reportLegalizationStats() {
  int64_t avg_displacement, sum_displacement, max_displacement;
  displacementStats(avg_displacement, sum_displacement, max_displacement);

  cout << "-------------------- Placement Analysis ------------------------" << endl;
  cout.precision(3);
  cout << "total displacement         : " << sum_displacement << endl;
  cout << "average displacement       : " << avg_displacement << endl;
  cout << "max displacement           : " << max_displacement << endl;
  double hpwl_orig = hpwl(true);
  cout << "original HPWL              : " << hpwl_orig << endl;
  double hpwl_legal = hpwl(false);
  cout << "legalized HPWL             : " << hpwl_legal << endl;
  double hpwl_delta = (hpwl_legal - hpwl_orig) / hpwl_orig * 100;
  cout.precision(0);
  cout << std::fixed;
  cout << "delta HPWL                 : " << hpwl_delta << "%" << endl;
  cout << "----------------------------------------------------------------" << endl;
}

////////////////////////////////////////////////////////////////

power
Opendp::rowTopPower(int row)
{
  return ((row0_top_power_is_vdd_ ? row : row+1) % 2 == 0) ? VDD : VSS;
}

dbOrientType
Opendp::rowOrient(int row)
{
  // orient flips. e.g. R0 -> MX -> R0 -> MX -> ...
  return ((row0_orient_is_r0_ ? row : row + 1) % 2 == 0)
    ? dbOrientType::R0
    : dbOrientType::MX;
}

////////////////////////////////////////////////////////////////

void
Opendp::initLocation(Cell *cell,
		     // Return values.
		     int &x,
		     int &y)
{
  initLocation(cell->db_inst_, x, y);
}

void
Opendp::initLocation(dbInst* inst,
		     // Return values.
		     int &x,
		     int &y)
{
  int loc_x, loc_y;
  inst->getLocation(loc_x, loc_y);
  x = loc_x - core_.xMin();
  y = loc_y - core_.yMin();
}

void
Opendp::initPaddedLoc(Cell *cell,
		      // Return values.
		      int &x,
		      int &y)
{
  initLocation(cell, x, y);
  if (isPadded(cell))
    x -= pad_left_ * site_width_;
}

int Opendp::disp(Cell *cell) {
  int init_x, init_y;
  initLocation(cell, init_x, init_y);
  return abs(init_x - cell->x_) +
         abs(init_y - cell->y_);
}

int Opendp::gridEndX() {
  return divCeil(core_.dx(), site_width_);
}

int Opendp::gridEndY() {
  return divCeil(core_.dy(), row_height_);
}

int Opendp::paddedWidth(Cell *cell) {
  if (isPadded(cell))
    return cell->width_ + (pad_left_ + pad_right_) * site_width_;
  else
    return cell->width_;
}

// Cells only exist for CLASS BLOCK and CORE_* instances.
// CORE_* instances are padded
// BLOCK instances are not padded
bool Opendp::isPadded(Cell *cell) {
  return isClassCore(cell)
    && (pad_left_  > 0 || pad_right_ > 0);
}

bool Opendp::isClassBlock(Cell *cell) {
  return cell->db_inst_->getMaster()->getType() == dbMasterType::BLOCK;
}

// Cells are only created for CLASS BLOCK and CORE instances.
bool Opendp::isClassCore(Cell *cell) {
  return !isClassBlock(cell);
}

int Opendp::gridPaddedWidth(Cell *cell) {
  return divCeil(paddedWidth(cell), site_width_);
}

int Opendp::gridHeight(Cell *cell) {
  return divCeil(cell->height_, row_height_);
}

// Callers should probably be using gridPaddedWidth.
int Opendp::gridNearestWidth(Cell *cell) {
  return divRound(paddedWidth(cell), site_width_);
}

// Callers should probably be using gridHeight.
int Opendp::gridNearestHeight(Cell *cell) {
  return divRound(cell->height_, row_height_);
}

int Opendp::gridX(int x) {
  return x / site_width_;
}

int Opendp::gridY(int y) {
  return y / row_height_;
}

int Opendp::gridX(Cell *cell) {
  return gridX(cell->x_);
}

int Opendp::gridPaddedX(Cell *cell) {
  if (isPadded(cell))
    return gridX(cell->x_ - pad_left_);
  else
    return gridX(cell->x_);
}

int Opendp::gridY(Cell *cell) {
  return gridY(cell->y_);
}

void Opendp::setGridPaddedLoc(Cell *cell,
			      int x,
			      int y) {
  cell->x_ = (x + (isPadded(cell) ? pad_left_ : 0)) * site_width_;
  cell->y_ = y * row_height_;
}

int Opendp::gridPaddedEndX(Cell *cell) {
  return divCeil(cell->x_ + cell->width_
		 + (isPadded(cell) ? pad_right_ * site_width_ : 0),
		 site_width_);
}

int Opendp::gridEndX(Cell *cell) {
  return divCeil(cell->x_ + cell->width_, site_width_);
}

int Opendp::gridEndY(Cell *cell) {
  return divCeil(cell->y_ + row_height_, row_height_);
}

int Opendp::coreGridMaxX() {
  return divRound(core_.xMax(), site_width_);
}

int Opendp::coreGridMaxY() {
  return divRound(core_.yMax(), row_height_);
}

double Opendp::dbuToMicrons(int64_t dbu) {
  return static_cast< double >(dbu) / db_->getTech()->getDbUnitsPerMicron();
}

int divRound(int dividend, int divisor) {
  return round(static_cast<double>(dividend) / divisor);
}

int divCeil(int dividend, int divisor) {
  return ceil(static_cast<double>(dividend) / divisor);
}

int divFloor(int dividend, int divisor) {
  return dividend / divisor;
}

void Opendp::reportGrid() {
  clear();
  dbToOpendp();
  Grid *grid = makeCellGrid();
  reportGrid(grid);
}

void Opendp::reportGrid(Grid *grid)
{
  std::map<Cell*, int> cell_index;
  int i = 0;
  for(Cell& cell : cells_) {
    cell_index[&cell] = i;
    i++;
  }

  // column header
  printf("   ");
  for(int j = 0; j < row_site_count_; j++) {
    printf("|%3d", j);
  }
  printf("|\n");
  printf("   ");
  for(int j = 0; j < row_site_count_; j++) {
    printf("|---");
  }
  printf("|\n");

  for(int i = row_count_ - 1; i >= 0; i--) {
    printf("%3d", i);
    for(int j = 0; j < row_site_count_; j++) {
      Cell* cell = grid[i][j].cell;
      if (cell)
	printf("|%3d", cell_index[cell]);
      else
	printf("|   ");
    }
    printf("|\n");
  }
  printf("\n");

  i = 0;
  for(Cell& cell : cells_) {
    printf("%3d %s\n", i, cell.name());
    i++;
  }
}

}  // namespace opendp
