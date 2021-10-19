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

#include "dbTrackGrid.h"

#include <algorithm>

#include "db.h"
#include "dbBlock.h"
#include "dbChip.h"
#include "dbDatabase.h"
#include "dbDiff.hpp"
#include "dbSet.h"
#include "dbTable.h"
#include "dbTable.hpp"
#include "dbTech.h"
#include "dbTechLayer.h"

namespace odb {

template class dbTable<_dbTrackGrid>;

bool _dbTrackGrid::operator==(const _dbTrackGrid& rhs) const
{
  if (_layer != rhs._layer)
    return false;

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

  if (_next_grid != rhs._next_grid)
    return false;

  return true;
}

void _dbTrackGrid::differences(dbDiff& diff,
                               const char* field,
                               const _dbTrackGrid& rhs) const
{
  DIFF_BEGIN
  DIFF_FIELD(_layer);
  DIFF_VECTOR(_x_origin);
  DIFF_VECTOR(_x_count);
  DIFF_VECTOR(_x_step);
  DIFF_VECTOR(_y_origin);
  DIFF_VECTOR(_y_count);
  DIFF_VECTOR(_y_step);
  DIFF_FIELD_NO_DEEP(_next_grid);
  DIFF_END
}

void _dbTrackGrid::out(dbDiff& diff, char side, const char* field) const
{
  DIFF_OUT_BEGIN
  DIFF_OUT_FIELD(_layer);
  DIFF_OUT_VECTOR(_x_origin);
  DIFF_OUT_VECTOR(_x_count);
  DIFF_OUT_VECTOR(_x_step);
  DIFF_OUT_VECTOR(_y_origin);
  DIFF_OUT_VECTOR(_y_count);
  DIFF_OUT_VECTOR(_y_step);
  DIFF_OUT_FIELD_NO_DEEP(_next_grid);
  DIFF_END
}

////////////////////////////////////////////////////////////////////
//
// dbTrackGrid - Methods
//
////////////////////////////////////////////////////////////////////
dbTechLayer* dbTrackGrid::getTechLayer()
{
  _dbTrackGrid* grid = (_dbTrackGrid*) this;
  _dbDatabase* db = grid->getDatabase();
  _dbTech* tech = db->_tech_tbl->getPtr(db->_tech);
  return (dbTechLayer*) tech->_layer_tbl->getPtr(grid->_layer);
}

void dbTrackGrid::getGridX(std::vector<int>& x_grid)
{
  x_grid.clear();

  _dbTrackGrid* grid = (_dbTrackGrid*) this;

  uint i;

  for (i = 0; i < grid->_x_origin.size(); ++i) {
    int j;

    int x = grid->_x_origin[i];
    int count = grid->_x_count[i];
    int step = grid->_x_step[i];

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

void dbTrackGrid::getGridY(std::vector<int>& y_grid)
{
  y_grid.clear();

  _dbTrackGrid* grid = (_dbTrackGrid*) this;

  uint i;

  for (i = 0; i < grid->_y_origin.size(); ++i) {
    int j;

    int y = grid->_y_origin[i];
    int count = grid->_y_count[i];
    int step = grid->_y_step[i];

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

dbBlock* dbTrackGrid::getBlock()
{
  return (dbBlock*) getImpl()->getOwner();
}

void dbTrackGrid::addGridPatternX(int origin_x, int line_count, int step)
{
  _dbTrackGrid* grid = (_dbTrackGrid*) this;
  grid->_x_origin.push_back(origin_x);
  grid->_x_count.push_back(line_count);
  grid->_x_step.push_back(step);
}

void dbTrackGrid::addGridPatternY(int origin_y, int line_count, int step)
{
  _dbTrackGrid* grid = (_dbTrackGrid*) this;
  grid->_y_origin.push_back(origin_y);
  grid->_y_count.push_back(line_count);
  grid->_y_step.push_back(step);
}

int dbTrackGrid::getNumGridPatternsX()
{
  _dbTrackGrid* grid = (_dbTrackGrid*) this;
  return grid->_x_origin.size();
}

int dbTrackGrid::getNumGridPatternsY()
{
  _dbTrackGrid* grid = (_dbTrackGrid*) this;
  return grid->_y_origin.size();
}

void dbTrackGrid::getGridPatternX(int i,
                                  int& origin_x,
                                  int& line_count,
                                  int& step)
{
  _dbTrackGrid* grid = (_dbTrackGrid*) this;
  ZASSERT(i < (int) grid->_x_origin.size());
  origin_x = grid->_x_origin[i];
  line_count = grid->_x_count[i];
  step = grid->_x_step[i];
}

void dbTrackGrid::getGridPatternY(int i,
                                  int& origin_y,
                                  int& line_count,
                                  int& step)
{
  _dbTrackGrid* grid = (_dbTrackGrid*) this;
  ZASSERT(i < (int) grid->_y_origin.size());
  origin_y = grid->_y_origin[i];
  line_count = grid->_y_count[i];
  step = grid->_y_step[i];
}

dbTrackGrid* dbTrackGrid::create(dbBlock* block_, dbTechLayer* layer_)
{
  _dbBlock* block = (_dbBlock*) block_;

  if (block_->findTrackGrid(layer_))
    return NULL;

  _dbTrackGrid* grid = block->_track_grid_tbl->create();
  grid->_layer = layer_->getImpl()->getOID();
  return (dbTrackGrid*) grid;
}

dbTrackGrid* dbTrackGrid::getTrackGrid(dbBlock* block_, uint dbid_)
{
  _dbBlock* block = (_dbBlock*) block_;
  return (dbTrackGrid*) block->_track_grid_tbl->getPtr(dbid_);
}
void dbTrackGrid::destroy(dbTrackGrid* grid_)
{
  _dbTrackGrid* grid = (_dbTrackGrid*) grid_;
  _dbBlock* block = (_dbBlock*) grid->getOwner();
  dbProperty::destroyProperties(grid);
  block->_track_grid_tbl->destroy(grid);
}

}  // namespace odb
