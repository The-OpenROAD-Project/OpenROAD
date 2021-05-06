///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2019, Nefelus Inc
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

// Generator Code Begin Cpp
#include "dbGCellGrid.h"

#include "db.h"
#include "dbDatabase.h"
#include "dbDiff.hpp"
#include "dbHashTable.h"
#include "dbTable.h"
#include "dbTable.hpp"
#include "dbTechLayer.h"
// User Code Begin Includes
#include <algorithm>

#include "dbBlock.h"
#include "dbSet.h"
// User Code End Includes
namespace odb {

template class dbTable<_dbGCellGrid>;

bool _dbGCellGrid::operator==(const _dbGCellGrid& rhs) const
{
  if (flags_.x_grid_valid_ != rhs.flags_.x_grid_valid_)
    return false;

  if (flags_.y_grid_valid_ != rhs.flags_.y_grid_valid_)
    return false;

  // User Code Begin ==
  if (x_origin_ != rhs.x_origin_)
    return false;

  if (x_count_ != rhs.x_count_)
    return false;

  if (x_step_ != rhs.x_step_)
    return false;

  if (y_origin_ != rhs.y_origin_)
    return false;

  if (y_count_ != rhs.y_count_)
    return false;

  if (y_step_ != rhs.y_step_)
    return false;
  // User Code End ==
  return true;
}
bool _dbGCellGrid::operator<(const _dbGCellGrid& rhs) const
{
  // User Code Begin <
  if (getOID() >= rhs.getOID())
    return false;
  // User Code End <
  return true;
}
void _dbGCellGrid::differences(dbDiff& diff,
                               const char* field,
                               const _dbGCellGrid& rhs) const
{
  DIFF_BEGIN

  DIFF_FIELD(flags_.x_grid_valid_);
  DIFF_FIELD(flags_.y_grid_valid_);
  // User Code Begin Differences
  DIFF_VECTOR(x_origin_);
  DIFF_VECTOR(x_count_);
  DIFF_VECTOR(x_step_);
  DIFF_VECTOR(y_origin_);
  DIFF_VECTOR(y_count_);
  DIFF_VECTOR(y_step_);
  // User Code End Differences
  DIFF_END
}
void _dbGCellGrid::out(dbDiff& diff, char side, const char* field) const
{
  DIFF_OUT_BEGIN
  DIFF_OUT_FIELD(flags_.x_grid_valid_);
  DIFF_OUT_FIELD(flags_.y_grid_valid_);

  // User Code Begin Out
  DIFF_OUT_VECTOR(x_origin_);
  DIFF_OUT_VECTOR(x_count_);
  DIFF_OUT_VECTOR(x_step_);
  DIFF_OUT_VECTOR(y_origin_);
  DIFF_OUT_VECTOR(y_count_);
  DIFF_OUT_VECTOR(y_step_);
  // User Code End Out
  DIFF_END
}
_dbGCellGrid::_dbGCellGrid(_dbDatabase* db)
{
  uint32_t* flags__bit_field = (uint32_t*) &flags_;
  *flags__bit_field = 0;
  // User Code Begin Constructor
  // User Code End Constructor
}
_dbGCellGrid::_dbGCellGrid(_dbDatabase* db, const _dbGCellGrid& r)
{
  flags_.x_grid_valid_ = r.flags_.x_grid_valid_;
  flags_.y_grid_valid_ = r.flags_.y_grid_valid_;
  flags_.spare_bits_ = r.flags_.spare_bits_;
  // User Code Begin CopyConstructor
  x_origin_ = r.x_origin_;
  x_count_ = r.x_count_;
  x_step_ = r.x_step_;
  y_origin_ = r.y_origin_;
  y_count_ = r.y_count_;
  y_step_ = r.y_step_;
  // User Code End CopyConstructor
}

dbIStream& operator>>(dbIStream& stream, _dbGCellGrid& obj)
{
  uint32_t* flags__bit_field = (uint32_t*) &obj.flags_;
  stream >> *flags__bit_field;
  stream >> obj.x_origin_;
  stream >> obj.x_count_;
  stream >> obj.x_step_;
  stream >> obj.y_origin_;
  stream >> obj.y_count_;
  stream >> obj.y_step_;
  stream >> obj.x_grid_;
  stream >> obj.y_grid_;
  stream >> obj.congestion_map_;
  // User Code Begin >>
  // User Code End >>
  return stream;
}
dbOStream& operator<<(dbOStream& stream, const _dbGCellGrid& obj)
{
  uint32_t* flags__bit_field = (uint32_t*) &obj.flags_;
  stream << *flags__bit_field;
  stream << obj.x_origin_;
  stream << obj.x_count_;
  stream << obj.x_step_;
  stream << obj.y_origin_;
  stream << obj.y_count_;
  stream << obj.y_step_;
  stream << obj.x_grid_;
  stream << obj.y_grid_;
  stream << obj.congestion_map_;
  // User Code Begin <<
  // User Code End <<
  return stream;
}

_dbGCellGrid::~_dbGCellGrid()
{
  // User Code Begin Destructor
  // User Code End Destructor
}

// User Code Begin PrivateMethods

dbIStream& operator>>(dbIStream& stream, dbGCellGrid::GCellData& obj)
{
  stream >> obj.horizontal_usage;
  stream >> obj.vertical_usage;
  stream >> obj.up_usage;
  stream >> obj.horizontal_capacity;
  stream >> obj.vertical_capacity;
  stream >> obj.up_capacity;
  return stream;
}

dbOStream& operator<<(dbOStream& stream, const dbGCellGrid::GCellData& obj)
{
  stream << obj.horizontal_usage;
  stream << obj.vertical_usage;
  stream << obj.up_usage;
  stream << obj.horizontal_capacity;
  stream << obj.vertical_capacity;
  stream << obj.up_capacity;
  return stream;
}

bool _dbGCellGrid::gcellExists(dbId<_dbTechLayer> lid,
                               uint x_idx,
                               uint y_idx) const
{
  return congestion_map_.find(lid) != congestion_map_.end()
         && congestion_map_.at(lid).find({x_idx, y_idx})
                != congestion_map_.at(lid).end();
}

// User Code End PrivateMethods

////////////////////////////////////////////////////////////////////
//
// dbGCellGrid - Methods
//
////////////////////////////////////////////////////////////////////

// User Code Begin dbGCellGridPublicMethods

void dbGCellGrid::getGridX(std::vector<int>& x_grid)
{
  _dbGCellGrid* grid = (_dbGCellGrid*) this;
  if (grid->flags_.x_grid_valid_) {
    x_grid = grid->x_grid_;
    return;
  }
  grid->x_grid_.clear();
  uint i;
  for (i = 0; i < grid->x_origin_.size(); ++i) {
    int j;

    int x = grid->x_origin_[i];
    int count = grid->x_count_[i];
    int step = grid->x_step_[i];

    for (j = 0; j < count; ++j) {
      grid->x_grid_.push_back(x);
      x += step;
    }
  }
  grid->flags_.x_grid_valid_ = true;
  // empty grid
  if (grid->x_grid_.begin() == grid->x_grid_.end()) {
    x_grid = grid->x_grid_;
    return;
  }

  // sort coords in asscending order
  std::sort(grid->x_grid_.begin(), grid->x_grid_.end());

  // remove any duplicates
  std::vector<int>::iterator new_end;
  new_end = std::unique(grid->x_grid_.begin(), grid->x_grid_.end());
  grid->x_grid_.erase(new_end, grid->x_grid_.end());
  x_grid = grid->x_grid_;
}

void dbGCellGrid::getGridY(std::vector<int>& y_grid)
{
  _dbGCellGrid* grid = (_dbGCellGrid*) this;
  if (grid->flags_.y_grid_valid_) {
    y_grid = grid->y_grid_;
    return;
  }
  grid->y_grid_.clear();

  uint i;

  for (i = 0; i < grid->y_origin_.size(); ++i) {
    int j;

    int y = grid->y_origin_[i];
    int count = grid->y_count_[i];
    int step = grid->y_step_[i];

    for (j = 0; j < count; ++j) {
      grid->y_grid_.push_back(y);
      y += step;
    }
  }
  grid->flags_.y_grid_valid_ = true;
  // empty grid
  if (grid->y_grid_.begin() == grid->y_grid_.end()) {
    y_grid = grid->y_grid_;
    return;
  }

  // sort coords in asscending order
  std::sort(grid->y_grid_.begin(), grid->y_grid_.end());

  // remove any duplicates
  std::vector<int>::iterator new_end;
  new_end = std::unique(grid->y_grid_.begin(), grid->y_grid_.end());
  grid->y_grid_.erase(new_end, grid->y_grid_.end());
  y_grid = grid->y_grid_;
}

dbBlock* dbGCellGrid::getBlock()
{
  return (dbBlock*) getImpl()->getOwner();
}

void dbGCellGrid::addGridPatternX(int origin_x, int line_count, int step)
{
  _dbGCellGrid* grid = (_dbGCellGrid*) this;
  grid->x_origin_.push_back(origin_x);
  grid->x_count_.push_back(line_count);
  grid->x_step_.push_back(step);
  grid->flags_.x_grid_valid_ = false;
  resetCongestionMap();
}

void dbGCellGrid::addGridPatternY(int origin_y, int line_count, int step)
{
  _dbGCellGrid* grid = (_dbGCellGrid*) this;
  grid->y_origin_.push_back(origin_y);
  grid->y_count_.push_back(line_count);
  grid->y_step_.push_back(step);
  grid->flags_.y_grid_valid_ = false;
  resetCongestionMap();
}

int dbGCellGrid::getNumGridPatternsX()
{
  _dbGCellGrid* grid = (_dbGCellGrid*) this;
  return grid->x_origin_.size();
}

int dbGCellGrid::getNumGridPatternsY()
{
  _dbGCellGrid* grid = (_dbGCellGrid*) this;
  return grid->y_origin_.size();
}

void dbGCellGrid::getGridPatternX(int i,
                                  int& origin_x,
                                  int& line_count,
                                  int& step)
{
  _dbGCellGrid* grid = (_dbGCellGrid*) this;
  ZASSERT(i < (int) grid->x_origin_.size());
  origin_x = grid->x_origin_[i];
  line_count = grid->x_count_[i];
  step = grid->x_step_[i];
}

void dbGCellGrid::getGridPatternY(int i,
                                  int& origin_y,
                                  int& line_count,
                                  int& step)
{
  _dbGCellGrid* grid = (_dbGCellGrid*) this;
  ZASSERT(i < (int) grid->y_origin_.size());
  origin_y = grid->y_origin_[i];
  line_count = grid->y_count_[i];
  step = grid->y_step_[i];
}

dbGCellGrid* dbGCellGrid::create(dbBlock* block_)
{
  _dbBlock* block = (_dbBlock*) block_;

  if (block->_gcell_grid != 0)
    return NULL;

  _dbGCellGrid* grid = block->_gcell_grid_tbl->create();
  block->_gcell_grid = grid->getOID();
  return (dbGCellGrid*) grid;
}

dbGCellGrid* dbGCellGrid::getGCellGrid(dbBlock* block_, uint dbid_)
{
  _dbBlock* block = (_dbBlock*) block_;
  return (dbGCellGrid*) block->_gcell_grid_tbl->getPtr(dbid_);
}

uint dbGCellGrid::getXIdx(int x)
{
  std::vector<int> grid;
  getGridX(grid);
  if (grid.empty() || grid[0] > x)
    return 0;
  auto pos = --(std::upper_bound(grid.begin(), grid.end(), x));
  return (int) std::distance(grid.begin(), pos);
}

uint dbGCellGrid::getYIdx(int y)
{
  std::vector<int> grid;
  getGridY(grid);
  if (grid.empty() || grid[0] > y)
    return 0;
  auto pos = --(std::upper_bound(grid.begin(), grid.end(), y));
  return (int) std::distance(grid.begin(), pos);
}

uint dbGCellGrid::getHorizontalCapacity(dbTechLayer* layer,
                                        uint x_idx,
                                        uint y_idx) const
{
  _dbGCellGrid* _grid = (_dbGCellGrid*) this;
  uint lid = layer->getId();
  if (_grid->gcellExists(lid, x_idx, y_idx))
    return _grid->congestion_map_[lid][{x_idx, y_idx}].horizontal_capacity;
  return 0;
}

uint dbGCellGrid::getVerticalCapacity(dbTechLayer* layer,
                                      uint x_idx,
                                      uint y_idx) const
{
  _dbGCellGrid* _grid = (_dbGCellGrid*) this;
  uint lid = layer->getId();
  if (_grid->gcellExists(lid, x_idx, y_idx))
    return _grid->congestion_map_[lid][{x_idx, y_idx}].vertical_capacity;
  return 0;
}

uint dbGCellGrid::getUpCapacity(dbTechLayer* layer,
                                uint x_idx,
                                uint y_idx) const
{
  _dbGCellGrid* _grid = (_dbGCellGrid*) this;
  uint lid = layer->getId();
  if (_grid->gcellExists(lid, x_idx, y_idx))
    return _grid->congestion_map_[lid][{x_idx, y_idx}].up_capacity;
  return 0;
}

uint dbGCellGrid::getHorizontalUsage(dbTechLayer* layer,
                                     uint x_idx,
                                     uint y_idx) const
{
  _dbGCellGrid* _grid = (_dbGCellGrid*) this;
  uint lid = layer->getId();
  if (_grid->gcellExists(lid, x_idx, y_idx))
    return _grid->congestion_map_[lid][{x_idx, y_idx}].horizontal_usage;
  return 0;
}

uint dbGCellGrid::getVerticalUsage(dbTechLayer* layer,
                                   uint x_idx,
                                   uint y_idx) const
{
  _dbGCellGrid* _grid = (_dbGCellGrid*) this;
  uint lid = layer->getId();
  if (_grid->gcellExists(lid, x_idx, y_idx))
    return _grid->congestion_map_[lid][{x_idx, y_idx}].vertical_usage;
  return 0;
}

uint dbGCellGrid::getUpUsage(dbTechLayer* layer, uint x_idx, uint y_idx) const
{
  _dbGCellGrid* _grid = (_dbGCellGrid*) this;
  uint lid = layer->getId();
  if (_grid->gcellExists(lid, x_idx, y_idx))
    return _grid->congestion_map_[lid][{x_idx, y_idx}].up_usage;
  return 0;
}

uint dbGCellGrid::getHorizontalBlockage(dbTechLayer* layer,
                                        uint x_idx,
                                        uint y_idx) const
{
  _dbGCellGrid* _grid = (_dbGCellGrid*) this;
  uint lid = layer->getId();
  if (_grid->gcellExists(lid, x_idx, y_idx))
    return _grid->congestion_map_[lid][{x_idx, y_idx}].horizontal_blockage;
  return 0;
}

uint dbGCellGrid::getVerticalBlockage(dbTechLayer* layer,
                                      uint x_idx,
                                      uint y_idx) const
{
  _dbGCellGrid* _grid = (_dbGCellGrid*) this;
  uint lid = layer->getId();
  if (_grid->gcellExists(lid, x_idx, y_idx))
    return _grid->congestion_map_[lid][{x_idx, y_idx}].vertical_blockage;
  return 0;
}

uint dbGCellGrid::getUpBlockage(dbTechLayer* layer,
                                uint x_idx,
                                uint y_idx) const
{
  _dbGCellGrid* _grid = (_dbGCellGrid*) this;
  uint lid = layer->getId();
  if (_grid->gcellExists(lid, x_idx, y_idx))
    return _grid->congestion_map_[lid][{x_idx, y_idx}].up_blockage;
  return 0;
}

void dbGCellGrid::setHorizontalCapacity(dbTechLayer* layer,
                                        uint x_idx,
                                        uint y_idx,
                                        uint capacity)
{
  _dbGCellGrid* _grid = (_dbGCellGrid*) this;
  uint lid = layer->getId();
  if (capacity == 0 && !_grid->gcellExists(lid, x_idx, y_idx))
    return;
  _grid->congestion_map_[lid][{x_idx, y_idx}].horizontal_capacity = capacity;
}

void dbGCellGrid::setVerticalCapacity(dbTechLayer* layer,
                                      uint x_idx,
                                      uint y_idx,
                                      uint capacity)
{
  _dbGCellGrid* _grid = (_dbGCellGrid*) this;
  uint lid = layer->getId();
  if (capacity == 0 && !_grid->gcellExists(lid, x_idx, y_idx))
    return;
  _grid->congestion_map_[lid][{x_idx, y_idx}].vertical_capacity = capacity;
}

void dbGCellGrid::setUpCapacity(dbTechLayer* layer,
                                uint x_idx,
                                uint y_idx,
                                uint capacity)
{
  _dbGCellGrid* _grid = (_dbGCellGrid*) this;
  uint lid = layer->getId();

  if (capacity == 0 && !_grid->gcellExists(lid, x_idx, y_idx))
    return;
  _grid->congestion_map_[lid][{x_idx, y_idx}].up_capacity = capacity;
}

void dbGCellGrid::setHorizontalUsage(dbTechLayer* layer,
                                     uint x_idx,
                                     uint y_idx,
                                     uint use)
{
  _dbGCellGrid* _grid = (_dbGCellGrid*) this;
  uint lid = layer->getId();
  if (use == 0 && !_grid->gcellExists(lid, x_idx, y_idx))
    return;
  _grid->congestion_map_[lid][{x_idx, y_idx}].horizontal_usage = use;
}

void dbGCellGrid::setVerticalUsage(dbTechLayer* layer,
                                   uint x_idx,
                                   uint y_idx,
                                   uint use)
{
  _dbGCellGrid* _grid = (_dbGCellGrid*) this;
  uint lid = layer->getId();
  if (use == 0 && !_grid->gcellExists(lid, x_idx, y_idx))
    return;
  _grid->congestion_map_[lid][{x_idx, y_idx}].vertical_usage = use;
}

void dbGCellGrid::setUpUsage(dbTechLayer* layer,
                             uint x_idx,
                             uint y_idx,
                             uint use)
{
  _dbGCellGrid* _grid = (_dbGCellGrid*) this;
  uint lid = layer->getId();
  if (use == 0 && !_grid->gcellExists(lid, x_idx, y_idx))
    return;
  _grid->congestion_map_[lid][{x_idx, y_idx}].up_usage = use;
}

void dbGCellGrid::setHorizontalBlockage(dbTechLayer* layer,
                                        uint x_idx,
                                        uint y_idx,
                                        uint blockage)
{
  _dbGCellGrid* _grid = (_dbGCellGrid*) this;
  uint lid = layer->getId();
  if (blockage == 0 && !_grid->gcellExists(lid, x_idx, y_idx))
    return;
  _grid->congestion_map_[lid][{x_idx, y_idx}].horizontal_blockage = blockage;
}

void dbGCellGrid::setVerticalBlockage(dbTechLayer* layer,
                                      uint x_idx,
                                      uint y_idx,
                                      uint blockage)
{
  _dbGCellGrid* _grid = (_dbGCellGrid*) this;
  uint lid = layer->getId();
  if (blockage == 0 && !_grid->gcellExists(lid, x_idx, y_idx))
    return;
  _grid->congestion_map_[lid][{x_idx, y_idx}].vertical_blockage = blockage;
}

void dbGCellGrid::setUpBlockage(dbTechLayer* layer,
                                uint x_idx,
                                uint y_idx,
                                uint blockage)
{
  _dbGCellGrid* _grid = (_dbGCellGrid*) this;
  uint lid = layer->getId();

  if (blockage == 0 && !_grid->gcellExists(lid, x_idx, y_idx))
    return;
  _grid->congestion_map_[lid][{x_idx, y_idx}].up_blockage = blockage;
}

void dbGCellGrid::setCapacity(dbTechLayer* layer,
                              uint x_idx,
                              uint y_idx,
                              uint horizontal,
                              uint vertical,
                              uint up)
{
  _dbGCellGrid* _grid = (_dbGCellGrid*) this;
  uint lid = layer->getId();
  if (horizontal == 0 && vertical == 0 && up == 0
      && !_grid->gcellExists(lid, x_idx, y_idx))
    return;
  _grid->congestion_map_[lid][{x_idx, y_idx}].horizontal_capacity = horizontal;
  _grid->congestion_map_[lid][{x_idx, y_idx}].vertical_capacity = vertical;
  _grid->congestion_map_[lid][{x_idx, y_idx}].up_capacity = up;
}

void dbGCellGrid::setUsage(dbTechLayer* layer,
                           uint x_idx,
                           uint y_idx,
                           uint horizontal,
                           uint vertical,
                           uint up)
{
  _dbGCellGrid* _grid = (_dbGCellGrid*) this;
  uint lid = layer->getId();
  if (horizontal == 0 && vertical == 0 && up == 0
      && !_grid->gcellExists(lid, x_idx, y_idx))
    return;
  _grid->congestion_map_[lid][{x_idx, y_idx}].horizontal_usage = horizontal;
  _grid->congestion_map_[lid][{x_idx, y_idx}].vertical_usage = vertical;
  _grid->congestion_map_[lid][{x_idx, y_idx}].up_usage = up;
}

void dbGCellGrid::setBlockage(dbTechLayer* layer,
                              uint x_idx,
                              uint y_idx,
                              uint horizontal,
                              uint vertical,
                              uint up)
{
  _dbGCellGrid* _grid = (_dbGCellGrid*) this;
  uint lid = layer->getId();
  if (horizontal == 0 && vertical == 0 && up == 0
      && !_grid->gcellExists(lid, x_idx, y_idx))
    return;
  _grid->congestion_map_[lid][{x_idx, y_idx}].horizontal_blockage = horizontal;
  _grid->congestion_map_[lid][{x_idx, y_idx}].vertical_blockage = vertical;
  _grid->congestion_map_[lid][{x_idx, y_idx}].up_blockage = up;
}

void dbGCellGrid::getCapacity(dbTechLayer* layer,
                              uint x_idx,
                              uint y_idx,
                              uint& horizontal,
                              uint& vertical,
                              uint& up) const
{
  _dbGCellGrid* _grid = (_dbGCellGrid*) this;
  uint lid = layer->getId();
  if (_grid->gcellExists(lid, x_idx, y_idx)) {
    auto data = _grid->congestion_map_[lid][{x_idx, y_idx}];
    horizontal = data.horizontal_capacity;
    vertical = data.vertical_capacity;
    up = data.up_capacity;
  } else {
    horizontal = 0;
    vertical = 0;
    up = 0;
  }
}

void dbGCellGrid::getUsage(dbTechLayer* layer,
                           uint x_idx,
                           uint y_idx,
                           uint& horizontal,
                           uint& vertical,
                           uint& up) const
{
  _dbGCellGrid* _grid = (_dbGCellGrid*) this;
  uint lid = layer->getId();
  if (_grid->gcellExists(lid, x_idx, y_idx)) {
    auto data = _grid->congestion_map_[lid][{x_idx, y_idx}];

    horizontal = data.horizontal_usage;
    vertical = data.vertical_usage;
    up = data.up_usage;
  } else {
    horizontal = 0;
    vertical = 0;
    up = 0;
  }
}

void dbGCellGrid::getBlockage(dbTechLayer* layer,
                              uint x_idx,
                              uint y_idx,
                              uint& horizontal,
                              uint& vertical,
                              uint& up) const
{
  _dbGCellGrid* _grid = (_dbGCellGrid*) this;
  uint lid = layer->getId();
  if (_grid->gcellExists(lid, x_idx, y_idx)) {
    auto data = _grid->congestion_map_[lid][{x_idx, y_idx}];

    horizontal = data.horizontal_blockage;
    vertical = data.vertical_blockage;
    up = data.up_blockage;
  } else {
    horizontal = 0;
    vertical = 0;
    up = 0;
  }
}

void dbGCellGrid::resetCongestionMap()
{
  _dbGCellGrid* _grid = (_dbGCellGrid*) this;
  _grid->congestion_map_.clear();
}

void dbGCellGrid::resetGrid()
{
  _dbGCellGrid* _grid = (_dbGCellGrid*) this;
  _grid->x_origin_.clear();
  _grid->x_count_.clear();
  _grid->x_step_.clear();
  _grid->y_origin_.clear();
  _grid->y_count_.clear();
  _grid->y_step_.clear();
  _grid->x_grid_.clear();
  _grid->y_grid_.clear();
  _grid->congestion_map_.clear();
  _grid->flags_.x_grid_valid_ = true;
  _grid->flags_.y_grid_valid_ = true;
}

std::map<std::pair<uint, uint>, dbGCellGrid::GCellData>
dbGCellGrid::getCongestionMap(dbTechLayer* layer)
{
  _dbGCellGrid* _grid = (_dbGCellGrid*) this;
  if (layer == nullptr) {
    std::map<std::pair<uint, uint>, dbGCellGrid::GCellData> congestion;
    for (auto& [lid, layer_map] : _grid->congestion_map_)
      for (auto& [key, val] : layer_map) {
        congestion[key].horizontal_usage += val.horizontal_usage;
        congestion[key].vertical_usage += val.vertical_usage;
        congestion[key].up_usage += val.up_usage;
        congestion[key].horizontal_capacity += val.horizontal_capacity;
        congestion[key].vertical_capacity += val.vertical_capacity;
        congestion[key].up_capacity += val.up_capacity;
      }
    return congestion;
  } else {
    if (_grid->congestion_map_.find(layer->getId())
        != _grid->congestion_map_.end())
      return _grid->congestion_map_[layer->getId()];
    else
      return std::map<std::pair<uint, uint>, dbGCellGrid::GCellData>();
  }
}
// User Code End dbGCellGridPublicMethods
}  // namespace odb
   // Generator Code End Cpp