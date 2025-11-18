// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "dbTrackGrid.h"

#include <algorithm>
#include <cmath>
#include <vector>

#include "dbBlock.h"
#include "dbChip.h"
#include "dbCore.h"
#include "dbDatabase.h"
#include "dbTable.h"
#include "dbTable.hpp"
#include "dbTech.h"
#include "dbTechLayer.h"
#include "odb/ZException.h"
#include "odb/db.h"
#include "odb/dbSet.h"
#include "odb/dbTypes.h"
#include "odb/odb.h"
#include "utl/Logger.h"

namespace odb {

template class dbTable<_dbTrackGrid>;

bool _dbTrackGrid::operator==(const _dbTrackGrid& rhs) const
{
  if (_layer != rhs._layer) {
    return false;
  }

  if (_x_origin != rhs._x_origin) {
    return false;
  }

  if (_x_count != rhs._x_count) {
    return false;
  }

  if (_x_step != rhs._x_step) {
    return false;
  }

  if (_y_origin != rhs._y_origin) {
    return false;
  }

  if (_y_count != rhs._y_count) {
    return false;
  }

  if (_y_step != rhs._y_step) {
    return false;
  }

  if (_first_mask != rhs._first_mask) {
    return false;
  }

  if (_samemask != rhs._samemask) {
    return false;
  }

  if (_next_grid != rhs._next_grid) {
    return false;
  }

  return true;
}

////////////////////////////////////////////////////////////////////
//
// dbTrackGrid - Methods
//
////////////////////////////////////////////////////////////////////
dbTechLayer* dbTrackGrid::getTechLayer()
{
  _dbTrackGrid* grid = (_dbTrackGrid*) this;
  _dbBlock* block = (_dbBlock*) grid->getOwner();
  _dbTech* tech = block->getTech();
  return (dbTechLayer*) tech->_layer_tbl->getPtr(grid->_layer);
}

const std::vector<int>& dbTrackGrid::getGridX()
{
  _dbTrackGrid* grid = (_dbTrackGrid*) this;

  if (grid->grid_x_.empty()) {
    uint i;

    for (i = 0; i < grid->_x_origin.size(); ++i) {
      int j;

      int x = grid->_x_origin[i];
      int count = grid->_x_count[i];
      int step = grid->_x_step[i];

      for (j = 0; j < count; ++j) {
        grid->grid_x_.push_back(x);
        x += step;
      }
    }

    // empty grid
    if (grid->grid_x_.begin() == grid->grid_x_.end()) {
      return grid->grid_x_;
    }

    // sort coords in asscending order
    std::sort(grid->grid_x_.begin(), grid->grid_x_.end());

    // remove any duplicates
    auto new_end = std::unique(grid->grid_x_.begin(), grid->grid_x_.end());
    grid->grid_x_.erase(new_end, grid->grid_x_.end());
  }

  return grid->grid_x_;
}

void dbTrackGrid::getGridX(std::vector<int>& x_grid)
{
  const std::vector<int>& grid = getGridX();
  x_grid.clear();
  x_grid.reserve(grid.size());
  x_grid.insert(x_grid.end(), grid.begin(), grid.end());
}

const std::vector<int>& dbTrackGrid::getGridY()
{
  _dbTrackGrid* grid = (_dbTrackGrid*) this;

  if (grid->grid_y_.empty()) {
    uint i;

    for (i = 0; i < grid->_y_origin.size(); ++i) {
      int j;

      int y = grid->_y_origin[i];
      int count = grid->_y_count[i];
      int step = grid->_y_step[i];

      for (j = 0; j < count; ++j) {
        grid->grid_y_.push_back(y);
        y += step;
      }
    }

    // empty grid
    if (grid->grid_y_.begin() == grid->grid_y_.end()) {
      return grid->grid_y_;
    }

    // sort coords in asscending order
    std::sort(grid->grid_y_.begin(), grid->grid_y_.end());

    // remove any duplicates
    auto new_end = std::unique(grid->grid_y_.begin(), grid->grid_y_.end());
    grid->grid_y_.erase(new_end, grid->grid_y_.end());
  }

  return grid->grid_y_;
}

void dbTrackGrid::getGridY(std::vector<int>& y_grid)
{
  const std::vector<int>& grid = getGridY();
  y_grid.clear();
  y_grid.reserve(grid.size());
  y_grid.insert(y_grid.end(), grid.begin(), grid.end());
}

dbBlock* dbTrackGrid::getBlock()
{
  return (dbBlock*) getImpl()->getOwner();
}

void dbTrackGrid::addGridPatternX(int origin_x,
                                  int line_count,
                                  int step,
                                  int first_mask,
                                  bool samemask)
{
  _dbTrackGrid* grid = (_dbTrackGrid*) this;
  grid->_x_origin.push_back(origin_x);
  grid->_x_count.push_back(line_count);
  grid->_x_step.push_back(step);
  grid->_first_mask.push_back(first_mask);
  grid->_samemask.push_back(samemask);

  grid->grid_x_.clear();
}

void dbTrackGrid::addGridPatternY(int origin_y,
                                  int line_count,
                                  int step,
                                  int first_mask,
                                  bool samemask)
{
  _dbTrackGrid* grid = (_dbTrackGrid*) this;
  grid->_y_origin.push_back(origin_y);
  grid->_y_count.push_back(line_count);
  grid->_y_step.push_back(step);
  grid->_first_mask.push_back(first_mask);
  grid->_samemask.push_back(samemask);

  grid->grid_y_.clear();
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

void dbTrackGrid::getGridPatternX(int i,
                                  int& origin_x,
                                  int& line_count,
                                  int& step,
                                  int& first_mask,
                                  bool& samemask)
{
  _dbTrackGrid* grid = (_dbTrackGrid*) this;
  ZASSERT(i < (int) grid->_x_origin.size());
  origin_x = grid->_x_origin[i];
  line_count = grid->_x_count[i];
  step = grid->_x_step[i];
  first_mask = grid->_first_mask[i];
  samemask = grid->_samemask[i];
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

void dbTrackGrid::getGridPatternY(int i,
                                  int& origin_y,
                                  int& line_count,
                                  int& step,
                                  int& first_mask,
                                  bool& samemask)
{
  _dbTrackGrid* grid = (_dbTrackGrid*) this;
  ZASSERT(i < (int) grid->_y_origin.size());
  origin_y = grid->_y_origin[i];
  line_count = grid->_y_count[i];
  step = grid->_y_step[i];
  first_mask = grid->_first_mask[i];
  samemask = grid->_samemask[i];
}

void dbTrackGrid::getAverageTrackSpacing(int& track_step,
                                         int& track_init,
                                         int& num_tracks)
{
  auto layer = getTechLayer();
  if (layer == nullptr) {
    getImpl()->getLogger()->error(utl::ODB, 418, "Layer is empty.");
    return;
  }
  if (layer->getDirection() == odb::dbTechLayerDir::HORIZONTAL) {
    if (getNumGridPatternsY() == 1) {
      getGridPatternY(0, track_init, num_tracks, track_step);
    } else if (getNumGridPatternsY() > 1) {
      _dbTrackGrid* track_grid = (_dbTrackGrid*) this;
      track_grid->getAverageTrackPattern(
          false, track_init, num_tracks, track_step);
    } else {
      getImpl()->getLogger()->error(utl::ODB,
                                    414,
                                    "Horizontal tracks for layer {} not found.",
                                    layer->getName());
    }
  } else if (layer->getDirection() == odb::dbTechLayerDir::VERTICAL) {
    if (getNumGridPatternsX() == 1) {
      getGridPatternX(0, track_init, num_tracks, track_step);
    } else if (getNumGridPatternsX() > 1) {
      _dbTrackGrid* track_grid = (_dbTrackGrid*) this;
      track_grid->getAverageTrackPattern(
          true, track_init, num_tracks, track_step);
    } else {
      getImpl()->getLogger()->error(utl::ODB,
                                    415,
                                    "Vertical tracks for layer {} not found.",
                                    layer->getName());
    }
  } else {
    getImpl()->getLogger()->error(
        utl::ODB, 416, "Layer {} has invalid direction.", layer->getName());
  }
}

dbTrackGrid* dbTrackGrid::create(dbBlock* block_, dbTechLayer* layer_)
{
  _dbBlock* block = (_dbBlock*) block_;

  if (block_->findTrackGrid(layer_)) {
    return nullptr;
  }

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

void _dbTrackGrid::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);

  info.children_["x_origin"].add(_x_origin);
  info.children_["x_count"].add(_x_count);
  info.children_["x_step"].add(_x_step);
  info.children_["y_origin"].add(_y_origin);
  info.children_["y_count"].add(_y_count);
  info.children_["y_step"].add(_y_step);
  info.children_["first_mask"].add(_first_mask);
  info.children_["samemask"].add(_samemask);
}

void _dbTrackGrid::getAverageTrackPattern(bool is_x,
                                          int& track_init,
                                          int& num_tracks,
                                          int& track_step)
{
  std::vector<int> coordinates;
  dbTrackGrid* track_grid = (dbTrackGrid*) this;
  if (is_x) {
    track_grid->getGridX(coordinates);
  } else {
    track_grid->getGridY(coordinates);
  }
  const int span = coordinates.back() - coordinates.front();
  track_init = coordinates.front();
  track_step = std::ceil((float) span / coordinates.size());
  num_tracks = coordinates.size();
}

}  // namespace odb
