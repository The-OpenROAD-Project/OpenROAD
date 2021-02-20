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

// Generator Code Begin 1
#include "dbGCellGrid.h"

#include "db.h"
#include "dbDatabase.h"
#include "dbDiff.hpp"
#include "dbHashTable.h"
#include "dbTable.h"
#include "dbTable.hpp"
#include "dbTechLayer.h"
// User Code Begin includes
#include <dbBlock.h>

#include <algorithm>

#include "dbSet.h"
// User Code End includes
namespace odb {

// User Code Begin definitions
// User Code End definitions
template class dbTable<_dbGCellGrid>;

bool _dbGCellGrid::operator==(const _dbGCellGrid& rhs) const
{
  // User Code Begin ==
  if (_x_origin != rhs._x_origin)
    return false;

  if (_x_count != rhs._x_count)
    return false;

  if (_x_step != rhs._x_step)
    return false;

  if (_y_origin != rhs._y_origin)
    return false;

  if (_y_count != rhs._y_count)
    return false;

  if (_y_step != rhs._y_step)
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
void _dbGCellGrid::differences(dbDiff&             diff,
                               const char*         field,
                               const _dbGCellGrid& rhs) const
{
  DIFF_BEGIN

  // User Code Begin differences
  DIFF_VECTOR(_x_origin);
  DIFF_VECTOR(_x_count);
  DIFF_VECTOR(_x_step);
  DIFF_VECTOR(_y_origin);
  DIFF_VECTOR(_y_count);
  DIFF_VECTOR(_y_step);
  // User Code End differences
  DIFF_END
}
void _dbGCellGrid::out(dbDiff& diff, char side, const char* field) const
{
  DIFF_OUT_BEGIN

  // User Code Begin out
  DIFF_OUT_VECTOR(_x_origin);
  DIFF_OUT_VECTOR(_x_count);
  DIFF_OUT_VECTOR(_x_step);
  DIFF_OUT_VECTOR(_y_origin);
  DIFF_OUT_VECTOR(_y_count);
  DIFF_OUT_VECTOR(_y_step);
  // User Code End out
  DIFF_END
}
_dbGCellGrid::_dbGCellGrid(_dbDatabase* db)
{
  // User Code Begin constructor
  // User Code End constructor
}
_dbGCellGrid::_dbGCellGrid(_dbDatabase* db, const _dbGCellGrid& r)
{
  // User Code Begin CopyConstructor
  _x_origin = r._x_origin;
  _x_count  = r._x_count;
  _x_step   = r._x_step;
  _y_origin = r._y_origin;
  _y_count  = r._y_count;
  _y_step   = r._y_step;
  // User Code End CopyConstructor
}

dbIStream& operator>>(dbIStream& stream, _dbGCellGrid& obj)
{
  stream >> obj._x_origin;
  stream >> obj._x_count;
  stream >> obj._x_step;
  stream >> obj._y_origin;
  stream >> obj._y_count;
  stream >> obj._y_step;
  // stream >> obj._usage_map;
  // stream >> obj._capacity_map;
  // User Code Begin >>
  // User Code End >>
  return stream;
}
dbOStream& operator<<(dbOStream& stream, const _dbGCellGrid& obj)
{
  stream << obj._x_origin;
  stream << obj._x_count;
  stream << obj._x_step;
  stream << obj._y_origin;
  stream << obj._y_count;
  stream << obj._y_step;
  // stream << obj._usage_map;
  // stream << obj._capacity_map;
  // User Code Begin <<
  // User Code End <<
  return stream;
}

_dbGCellGrid::~_dbGCellGrid()
{
  // User Code Begin Destructor
  // User Code End Destructor
}
////////////////////////////////////////////////////////////////////
//
// dbGCellGrid - Methods
//
////////////////////////////////////////////////////////////////////

// User Code Begin dbGCellGridPublicMethods
dbIStream& operator>>(dbIStream& stream, GCellData& obj)
{
  stream >> obj._horizontal;
  stream >> obj._vertical;
  stream >> obj._up;
  return stream;
}

dbOStream& operator<<(dbOStream& stream, const GCellData& obj)
{
  stream << obj._horizontal;
  stream << obj._vertical;
  stream << obj._up;
  return stream;
}

void dbGCellGrid::getGridX(std::vector<int>& x_grid) const
{
  x_grid.clear();

  _dbGCellGrid* grid = (_dbGCellGrid*) this;

  uint i;

  for (i = 0; i < grid->_x_origin.size(); ++i) {
    int j;

    int x     = grid->_x_origin[i];
    int count = grid->_x_count[i];
    int step  = grid->_x_step[i];

    for (j = 0; j < count; ++j) {
      x_grid.push_back(x);
      x += step;
    }
  }

  // empty grid
  if (x_grid.begin() == x_grid.end())
    return;

  // sort coords in asscending order
  std::sort(x_grid.begin(), x_grid.end());

  // remove any duplicates
  std::vector<int>::iterator new_end;
  new_end = std::unique(x_grid.begin(), x_grid.end());
  x_grid.erase(new_end, x_grid.end());
}

void dbGCellGrid::getGridY(std::vector<int>& y_grid) const
{
  y_grid.clear();

  _dbGCellGrid* grid = (_dbGCellGrid*) this;

  uint i;

  for (i = 0; i < grid->_y_origin.size(); ++i) {
    int j;

    int y     = grid->_y_origin[i];
    int count = grid->_y_count[i];
    int step  = grid->_y_step[i];

    for (j = 0; j < count; ++j) {
      y_grid.push_back(y);
      y += step;
    }
  }

  // empty grid
  if (y_grid.begin() == y_grid.end())
    return;

  // sort coords in asscending order
  std::sort(y_grid.begin(), y_grid.end());

  // remove any duplicates
  std::vector<int>::iterator new_end;
  new_end = std::unique(y_grid.begin(), y_grid.end());
  y_grid.erase(new_end, y_grid.end());
}

dbBlock* dbGCellGrid::getBlock()
{
  return (dbBlock*) getImpl()->getOwner();
}

void dbGCellGrid::addGridPatternX(int origin_x, int line_count, int step)
{
  _dbGCellGrid* grid = (_dbGCellGrid*) this;
  grid->_x_origin.push_back(origin_x);
  grid->_x_count.push_back(line_count);
  grid->_x_step.push_back(step);
  resetCongestionMap();
}

void dbGCellGrid::addGridPatternY(int origin_y, int line_count, int step)
{
  _dbGCellGrid* grid = (_dbGCellGrid*) this;
  grid->_y_origin.push_back(origin_y);
  grid->_y_count.push_back(line_count);
  grid->_y_step.push_back(step);
  resetCongestionMap();
}

int dbGCellGrid::getNumGridPatternsX()
{
  _dbGCellGrid* grid = (_dbGCellGrid*) this;
  return grid->_x_origin.size();
}

int dbGCellGrid::getNumGridPatternsY()
{
  _dbGCellGrid* grid = (_dbGCellGrid*) this;
  return grid->_y_origin.size();
}

void dbGCellGrid::getGridPatternX(int  i,
                                  int& origin_x,
                                  int& line_count,
                                  int& step)
{
  _dbGCellGrid* grid = (_dbGCellGrid*) this;
  ZASSERT(i < (int) grid->_x_origin.size());
  origin_x   = grid->_x_origin[i];
  line_count = grid->_x_count[i];
  step       = grid->_x_step[i];
}

void dbGCellGrid::getGridPatternY(int  i,
                                  int& origin_y,
                                  int& line_count,
                                  int& step)
{
  _dbGCellGrid* grid = (_dbGCellGrid*) this;
  ZASSERT(i < (int) grid->_y_origin.size());
  origin_y   = grid->_y_origin[i];
  line_count = grid->_y_count[i];
  step       = grid->_y_step[i];
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

uint dbGCellGrid::getXIdx(uint x, std::vector<int> x_grid) const
{
  if (x_grid.empty())
    getGridX(x_grid);
  auto pos = --(std::upper_bound(x_grid.begin(), x_grid.end(), x));
  return std::max(0, (int) std::distance(x_grid.begin(), pos));
}

uint dbGCellGrid::getYIdx(uint y, std::vector<int> y_grid) const
{
  if (y_grid.empty())
    getGridY(y_grid);
  auto pos = --(std::upper_bound(y_grid.begin(), y_grid.end(), y));
  return std::max(0, (int) std::distance(y_grid.begin(), pos));
}

uint dbGCellGrid::getHorizontalCapacity(dbTechLayer* layer,
                                        uint         x_idx,
                                        uint         y_idx) const
{
  _dbGCellGrid* _grid = (_dbGCellGrid*) this;
  uint          lid   = layer->getId();
  if (_grid->_capacity_map.find(lid) != _grid->_capacity_map.end() 
      && _grid->_capacity_map[lid].find(x_idx)  != _grid->_capacity_map[lid].end()
      && _grid->_capacity_map[lid][x_idx].find(y_idx) != _grid->_capacity_map[lid][x_idx].end())
    return _grid->_capacity_map[lid][x_idx][y_idx]._horizontal;
  return 0;
}

uint dbGCellGrid::getVerticalCapacity(dbTechLayer* layer,
                                      uint         x_idx,
                                      uint         y_idx) const
{
  _dbGCellGrid* _grid = (_dbGCellGrid*) this;
  uint          lid   = layer->getId();
  if (_grid->_capacity_map.find(lid) != _grid->_capacity_map.end() 
      && _grid->_capacity_map[lid].find(x_idx)  != _grid->_capacity_map[lid].end()
      && _grid->_capacity_map[lid][x_idx].find(y_idx) != _grid->_capacity_map[lid][x_idx].end())
    return _grid->_capacity_map[lid][x_idx][y_idx]._vertical;
  return 0;
}

uint dbGCellGrid::getUpCapacity(dbTechLayer* layer,
                                uint         x_idx,
                                uint         y_idx) const
{
  _dbGCellGrid* _grid = (_dbGCellGrid*) this;
  uint          lid   = layer->getId();
  if (_grid->_capacity_map.find(lid) != _grid->_capacity_map.end() 
      && _grid->_capacity_map[lid].find(x_idx)  != _grid->_capacity_map[lid].end()
      && _grid->_capacity_map[lid][x_idx].find(y_idx) != _grid->_capacity_map[lid][x_idx].end())
    return _grid->_capacity_map[lid][x_idx][y_idx]._up;
  return 0;
}

uint dbGCellGrid::getHorizontalUsage(dbTechLayer* layer,
                                     uint         x_idx,
                                     uint         y_idx) const
{
  _dbGCellGrid* _grid = (_dbGCellGrid*) this;
  uint          lid   = layer->getId();
  if (_grid->_usage_map.find(lid) != _grid->_usage_map.end() 
      && _grid->_usage_map[lid].find(x_idx)  != _grid->_usage_map[lid].end()
      && _grid->_usage_map[lid][x_idx].find(y_idx) != _grid->_usage_map[lid][x_idx].end())
    return _grid->_usage_map[lid][x_idx][y_idx]._horizontal;
  return 0;
}

uint dbGCellGrid::getVerticalUsage(dbTechLayer* layer,
                                   uint         x_idx,
                                   uint         y_idx) const
{
  _dbGCellGrid* _grid = (_dbGCellGrid*) this;
  uint          lid   = layer->getId();
  if (_grid->_usage_map.find(lid) != _grid->_usage_map.end() 
      && _grid->_usage_map[lid].find(x_idx)  != _grid->_usage_map[lid].end()
      && _grid->_usage_map[lid][x_idx].find(y_idx) != _grid->_usage_map[lid][x_idx].end())
    return _grid->_usage_map[lid][x_idx][y_idx]._vertical;
  return 0;
}

uint dbGCellGrid::getUpUsage(dbTechLayer* layer, uint x_idx, uint y_idx) const
{
  _dbGCellGrid* _grid = (_dbGCellGrid*) this;
  uint          lid   = layer->getId();
  if (_grid->_usage_map.find(lid) != _grid->_usage_map.end() 
      && _grid->_usage_map[lid].find(x_idx)  != _grid->_usage_map[lid].end()
      && _grid->_usage_map[lid][x_idx].find(y_idx) != _grid->_usage_map[lid][x_idx].end())
    return _grid->_usage_map[lid][x_idx][y_idx]._up;
  return 0;
}

void dbGCellGrid::setHorizontalCapacity(dbTechLayer* layer,
                                        uint         x_idx,
                                        uint         y_idx,
                                        uint         capacity)
{
  _dbGCellGrid* _grid = (_dbGCellGrid*) this;
  _grid->_capacity_map[layer->getId()][x_idx][y_idx]._horizontal = capacity;
}

void dbGCellGrid::setVerticalCapacity(dbTechLayer* layer,
                                      uint         x_idx,
                                      uint         y_idx,
                                      uint         capacity)
{
  _dbGCellGrid* _grid = (_dbGCellGrid*) this;
  _grid->_capacity_map[layer->getId()][x_idx][y_idx]._vertical = capacity;
}

void dbGCellGrid::setUpCapacity(dbTechLayer* layer,
                                uint         x_idx,
                                uint         y_idx,
                                uint         capacity)
{
  _dbGCellGrid* _grid                                    = (_dbGCellGrid*) this;
  _grid->_capacity_map[layer->getId()][x_idx][y_idx]._up = capacity;
}

void dbGCellGrid::setHorizontalUsage(dbTechLayer* layer,
                                     uint         x_idx,
                                     uint         y_idx,
                                     uint         use)
{
  _dbGCellGrid* _grid = (_dbGCellGrid*) this;
  _grid->_usage_map[layer->getId()][x_idx][y_idx]._horizontal = use;
}

void dbGCellGrid::setVerticalUsage(dbTechLayer* layer,
                                   uint         x_idx,
                                   uint         y_idx,
                                   uint         use)
{
  _dbGCellGrid* _grid = (_dbGCellGrid*) this;
  _grid->_usage_map[layer->getId()][x_idx][y_idx]._vertical = use;
}

void dbGCellGrid::setUpUsage(dbTechLayer* layer,
                             uint         x_idx,
                             uint         y_idx,
                             uint         use)
{
  _dbGCellGrid* _grid                                 = (_dbGCellGrid*) this;
  _grid->_usage_map[layer->getId()][x_idx][y_idx]._up = use;
}

void dbGCellGrid::setCapacity(dbTechLayer* layer,
                              uint         x_idx,
                              uint         y_idx,
                              uint         horizontal,
                              uint         vertical,
                              uint         up)
{
  _dbGCellGrid* _grid = (_dbGCellGrid*) this;
  GCellData     data;
  data._horizontal                                   = horizontal;
  data._vertical                                     = vertical;
  data._up                                           = up;
  _grid->_capacity_map[layer->getId()][x_idx][y_idx] = data;
}

void dbGCellGrid::setUsage(dbTechLayer* layer,
                           uint         x_idx,
                           uint         y_idx,
                           uint         horizontal,
                           uint         vertical,
                           uint         up)
{
  _dbGCellGrid* _grid = (_dbGCellGrid*) this;
  GCellData     data;
  data._horizontal                                = horizontal;
  data._vertical                                  = vertical;
  data._up                                        = up;
  _grid->_usage_map[layer->getId()][x_idx][y_idx] = data;
}

void dbGCellGrid::getCapacity(dbTechLayer* layer,
                              uint         x_idx,
                              uint         y_idx,
                              uint&        horizontal,
                              uint&        vertical,
                              uint&        up) const
{
  _dbGCellGrid* _grid = (_dbGCellGrid*) this;
  uint          lid   = layer->getId();
  if (_grid->_capacity_map.find(lid) != _grid->_capacity_map.end() 
      && _grid->_capacity_map[lid].find(x_idx)  != _grid->_capacity_map[lid].end()
      && _grid->_capacity_map[lid][x_idx].find(y_idx) != _grid->_capacity_map[lid][x_idx].end()) {
    auto data  = _grid->_capacity_map[lid][x_idx][y_idx];
    horizontal = data._horizontal;
    vertical   = data._vertical;
    up         = data._up;
  } else {
    horizontal = 0;
    vertical   = 0;
    up         = 0;
  }
}

void dbGCellGrid::getUsage(dbTechLayer* layer,
                           uint         x_idx,
                           uint         y_idx,
                           uint&        horizontal,
                           uint&        vertical,
                           uint&        up) const
{
  _dbGCellGrid* _grid = (_dbGCellGrid*) this;
  uint          lid   = layer->getId();
  if (_grid->_usage_map.find(lid) != _grid->_usage_map.end() 
      && _grid->_usage_map[lid].find(x_idx)  != _grid->_usage_map[lid].end()
      && _grid->_usage_map[lid][x_idx].find(y_idx) != _grid->_usage_map[lid][x_idx].end()) {
    auto data = _grid->_usage_map[lid][x_idx][y_idx];

    horizontal = data._horizontal;
    vertical   = data._vertical;
    up         = data._up;
  } else {
    horizontal = 0;
    vertical   = 0;
    up         = 0;
  }
}

void dbGCellGrid::resetCongestionMap()
{
  _dbGCellGrid* _grid = (_dbGCellGrid*) this;
  if (!_grid->_capacity_map.empty())
    _grid->_capacity_map.clear();
  if (!_grid->_usage_map.empty())
    _grid->_usage_map.clear();
}
// User Code End dbGCellGridPublicMethods
}  // namespace odb
// Generator Code End 1
