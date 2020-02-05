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

using odb::adsRect;
using odb::dbBox;
using odb::dbITerm;
using odb::dbMasterType;
using odb::dbMPin;
using odb::dbMTerm;
using odb::dbPlacementStatus;

void density_bin::print() {
  cout << "|=== BEGIN DENSITY_BIN ===|" << endl;
  cout << " area :        " << area << endl;
  cout << " m_util :      " << m_util << endl;
  cout << " f_util :      " << f_util << endl;
  cout << " free_space :  " << free_space << endl;
  cout << " overflow :    " << overflow << endl;
  cout << " density limit:" << density_limit << endl;
  cout << "|===  END  DENSITY_BIN ===|" << endl;
}

Macro::Macro()
    : isMulti(false),
      edgetypeLeft(0),
      edgetypeRight(0),
      top_power(power::undefined) {}

void Macro::print() {
  cout << "|=== BEGIN MACRO ===|" << endl;
  cout << "name:                " << db_master->getConstName() << endl;
  cout << "|=== BEGIN MACRO ===|" << endl;
}

Cell::Cell()
    : cell_macro(nullptr),
      x_coord(0),
      y_coord(0),
      init_x_coord(0),
      init_y_coord(0),
      x_pos(INT_MAX),
      y_pos(INT_MAX),
      width(0.0),
      height(0.0),
      is_placed(false),
      hold(false),
      region(nullptr),
      cell_group(nullptr),
      dense_factor(0.0),
      dense_factor_count(0) {}

const char *Cell::name() {
  return db_inst->getConstName();
}

void Cell::print() {
  cout << "|=== BEGIN CELL ===|" << endl;
  cout << "name:               " << db_inst->getConstName() << endl;
  cout << "type:               " << cell_macro->db_master->getConstName()
       << endl;
  cout << "(init_x,  init_y):  " << init_x_coord << ", " << init_y_coord
       << endl;
  cout << "(x_coord,y_coord):  " << x_coord << ", " << y_coord << endl;
  cout << "[width,height]:      " << width << ", " << height << endl;
  cout << "|===  END  CELL ===|" << endl;
}

int Cell::disp() {
  return abs(init_x_coord - x_coord) +
         abs(init_y_coord - y_coord);
}

////////////////////////////////////////////////////////////////

bool Opendp::isFixed(Cell *cell) {
  return cell == &dummy_cell_ ||
         cell->db_inst->getPlacementStatus() == dbPlacementStatus::FIRM ||
         cell->db_inst->getPlacementStatus() == dbPlacementStatus::LOCKED ||
         cell->db_inst->getPlacementStatus() == dbPlacementStatus::COVER;
}

Pixel::Pixel()
    : util(0.0),
      x_pos(0.0),
      y_pos(0.0),
      pixel_group(nullptr),
      linked_cell(NULL),
      is_valid(true) {}

Row::Row()
    : origX(0),
      origY(0),
      orient(dbOrientType::R0) {}

void Row::print() {
  cout << "|=== BEGIN ROW ===|" << endl;
  cout << "(origX,origY):     " << origX << ", " << origY << endl;
  cout << "orientation:       " << orient << endl;
  cout << "|===  END  ROW ===|" << endl;
}

////////////////////////////////////////////////////////////////

Group::Group() : name(""), util(0.0){}

sub_region::sub_region() : x_pos(0), y_pos(0), width(0), height(0) {}

Opendp::Opendp()
    : initial_power_(power::undefined),
      diamond_search_height_(400),
      max_displacement_constraint_(0),
      site_width_(0),
      max_cell_height_(1) {
}

Opendp::~Opendp() {}

void Opendp::init(dbDatabase *db) { db_ = db; }

void Opendp::clear() {
  macros_.clear();
  rows_.clear();
  cells_.clear();
}

bool Opendp::legalizePlacement(bool verbose) {
  dbToOpendp();
  initAfterImport();
  reportDesignStats();
  simplePlacement(verbose);
  updateDbInstLocations();
  bool legal = checkLegality(verbose);
  reportLegalizationStats();
  return legal;
}

void Opendp::initAfterImport() {
  findDesignStats();
  power_mapping();

  // dummy cell generation
  dummy_cell_.is_placed = true;

  // calc row / site offset
  int row_offset = rows_[0].origY;
  int site_offset = rows_[0].origX;

  // construct pixel grid
  int row_num = gridHeight();
  int col = gridWidth();
  grid_ = new Pixel *[row_num];
  for(int i = 0; i < row_num; i++) {
    grid_[i] = new Pixel[col];
  }

  for(int i = 0; i < row_num; i++) {
    for(int j = 0; j < col; j++) {
      grid_[i][j].y_pos = i;
      grid_[i][j].x_pos = j;
      grid_[i][j].linked_cell = NULL;
      grid_[i][j].is_valid = false;
    }
  }

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

void Opendp::updateDbInstLocations() {
  for (Cell &cell : cells_) {
    int x = cell.x_coord + core_.xMin();
    int y = cell.y_coord + core_.yMin();
    dbInst *db_inst = cell.db_inst;
    db_inst->setLocation(x, y);
    // Orientation is already set.
  }
}

void Opendp::findDesignStats() {
  fixed_inst_count_ = 0;
  multi_height_inst_count_ = 0;
  movable_area_ = fixed_area_ = 0;

  for(Cell &cell : cells_) {
    int cell_area = cell.width * cell.height;
    if(isFixed(&cell)) {
      fixed_area_ += cell_area;
      fixed_inst_count_++;
    }
    else
      movable_area_ += cell_area;
    Macro *macro = cell.cell_macro;
    if(macro->isMulti)
      multi_height_inst_count_++;
  }

  design_area_ = 0;
  for(Row &row : rows_)
    design_area_ += static_cast< int64_t >(site_width_) * row_site_count_ * row_height_;

  for(Cell &cell : cells_) {
    Macro *macro = cell.cell_macro;
    dbMaster *master = macro->db_master;
    if(!isFixed(&cell) && macro->isMulti &&
       master->getType() == dbMasterType::CORE) {
      int cell_height = gridNearestHeight(&cell);
      if(max_cell_height_ < cell_height) max_cell_height_ = cell_height;
    }
  }

  design_util_ =
      static_cast< double >(movable_area_) / (design_area_ - fixed_area_);

  // design_utilization error handling.
  if(design_util_ >= 1.001) {
    cout << "Error: utilization exceeds 100%. (" << fixed << setprecision(2)
         << design_util_ * 100.00 << "%)" << endl;
    exit(1);
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
  cout << "design utilization         : " << design_util_ * 100.00 << endl;
  cout << "rows                       : " << rows_.size() << endl;
  cout << "row height                 : " << row_height_ << endl;
  if(max_cell_height_ > 1)
    cout << "max multi_cell height      : " << max_cell_height_ << endl;
  if(groups_.size() > 0)
  cout << "group count                : " << groups_.size() << endl;
  cout << "----------------------------------------------------------------" << endl;
}

void Opendp::reportLegalizationStats() {
  int avg_displacement, sum_displacement, max_displacement;
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

bool Opendp::readConstraints(const string input) {
  //    cout << " .constraints file : " << input << endl;
  ifstream dot_constraints(input.c_str());
  if(!dot_constraints.good()) {
    cerr << "readConstraints:: cannot open '" << input << "' for reading"
         << endl;
    return true;
  }

  string context;

  while(!dot_constraints.eof()) {
    dot_constraints >> context;
    if(dot_constraints.eof()) break;
    if(strncmp(context.c_str(), "maximum_movement", 16) == 0) {
      string temp = context.substr(0, context.find_last_of("rows"));
      string max_move = temp.substr(temp.find_last_of("=") + 1);
      diamond_search_height_ = atoi(max_move.c_str()) * 20;
      max_displacement_constraint_ = atoi(max_move.c_str());
    }
    else {
      cerr << "readConstraints:: unsupported keyword " << endl;
      return true;
    }
  }

  if(max_displacement_constraint_ == 0)
    max_displacement_constraint_ = rows_.size();

  dot_constraints.close();
  return false;
}

int Opendp::gridWidth() {
  return core_.dx() / site_width_;
}

int Opendp::gridHeight() {
  return core_.dy() / row_height_;
}

int Opendp::gridWidth(Cell *cell) {
  return ceil(cell->width / static_cast<double>(site_width_));
}

int Opendp::gridHeight(Cell *cell) {
  return ceil(cell->height / static_cast<double>(row_height_));
}

// Callers should probably be using gridWidth.
int Opendp::gridNearestWidth(Cell *cell) {
  return divRound(cell->width, site_width_);
}

// Callers should probably be using gridHeight.
int Opendp::gridNearestHeight(Cell *cell) {
  return divRound(cell->height, row_height_);
}

int Opendp::gridX(int x) {
  return x / site_width_;
}

int Opendp::gridY(int y) {
  return y / row_height_;
}

int Opendp::gridNearestX(int x) {
  return divRound(x, site_width_);
}

int Opendp::gridNearestY(int y) {
  return divRound(y, row_height_);
}

int Opendp::gridX(Cell *cell) {
  return cell->x_coord / site_width_;
}

int Opendp::gridY(Cell *cell) {
  return cell->y_coord / row_height_;
}

// Callers should probably be using gridX.
int Opendp::gridNearestX(Cell *cell) {
  return gridNearestX(cell->x_coord);
}

// Callers should probably be using gridY.
int Opendp::gridNearestY(Cell *cell) {
  return gridNearestY(cell->y_coord);
}

int Opendp::coreGridWidth() {
  return divRound(core_.dx(), site_width_);
}

int Opendp::coreGridHeight() {
  return divRound(core_.dy(),row_height_);
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

}  // namespace opendp
