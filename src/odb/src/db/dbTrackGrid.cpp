// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "dbTrackGrid.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <vector>

#include "dbBlock.h"
#include "dbChip.h"
#include "dbCore.h"
#include "dbDatabase.h"
#include "dbTable.h"
#include "dbTech.h"
#include "dbTechLayer.h"
#include "odb/db.h"
#include "odb/dbSet.h"
#include "odb/dbTypes.h"
#include "utl/Logger.h"
#include "utl/algorithms.h"

namespace odb {

template class dbTable<_dbTrackGrid>;

bool _dbTrackGrid::operator==(const _dbTrackGrid& rhs) const
{
  if (layer_ != rhs.layer_) {
    return false;
  }

  if (x_origin_ != rhs.x_origin_) {
    return false;
  }

  if (x_count_ != rhs.x_count_) {
    return false;
  }

  if (x_step_ != rhs.x_step_) {
    return false;
  }

  if (y_origin_ != rhs.y_origin_) {
    return false;
  }

  if (y_count_ != rhs.y_count_) {
    return false;
  }

  if (y_step_ != rhs.y_step_) {
    return false;
  }

  if (first_mask_ != rhs.first_mask_) {
    return false;
  }

  if (samemask_ != rhs.samemask_) {
    return false;
  }

  if (next_grid_ != rhs.next_grid_) {
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
  return (dbTechLayer*) tech->layer_tbl_->getPtr(grid->layer_);
}

const std::vector<int>& dbTrackGrid::getGridX()
{
  _dbTrackGrid* grid = (_dbTrackGrid*) this;

  if (grid->grid_x_.empty()) {
    uint32_t i;

    for (i = 0; i < grid->x_origin_.size(); ++i) {
      int j;

      int x = grid->x_origin_[i];
      int count = grid->x_count_[i];
      int step = grid->x_step_[i];

      for (j = 0; j < count; ++j) {
        grid->grid_x_.push_back(x);
        x += step;
      }
    }

    // empty grid
    if (grid->grid_x_.begin() == grid->grid_x_.end()) {
      return grid->grid_x_;
    }

    utl::sort_and_unique(grid->grid_x_);
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
    uint32_t i;

    for (i = 0; i < grid->y_origin_.size(); ++i) {
      int j;

      int y = grid->y_origin_[i];
      int count = grid->y_count_[i];
      int step = grid->y_step_[i];

      for (j = 0; j < count; ++j) {
        grid->grid_y_.push_back(y);
        y += step;
      }
    }

    // empty grid
    if (grid->grid_y_.begin() == grid->grid_y_.end()) {
      return grid->grid_y_;
    }

    utl::sort_and_unique(grid->grid_y_);
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
  grid->x_origin_.push_back(origin_x);
  grid->x_count_.push_back(line_count);
  grid->x_step_.push_back(step);
  grid->first_mask_.push_back(first_mask);
  grid->samemask_.push_back(samemask);

  grid->grid_x_.clear();
}

void dbTrackGrid::addGridPatternY(int origin_y,
                                  int line_count,
                                  int step,
                                  int first_mask,
                                  bool samemask)
{
  _dbTrackGrid* grid = (_dbTrackGrid*) this;
  grid->y_origin_.push_back(origin_y);
  grid->y_count_.push_back(line_count);
  grid->y_step_.push_back(step);
  grid->first_mask_.push_back(first_mask);
  grid->samemask_.push_back(samemask);

  grid->grid_y_.clear();
}

int dbTrackGrid::getNumGridPatternsX()
{
  _dbTrackGrid* grid = (_dbTrackGrid*) this;
  return grid->x_origin_.size();
}

int dbTrackGrid::getNumGridPatternsY()
{
  _dbTrackGrid* grid = (_dbTrackGrid*) this;
  return grid->y_origin_.size();
}

void dbTrackGrid::getGridPatternX(int i,
                                  int& origin_x,
                                  int& line_count,
                                  int& step)
{
  _dbTrackGrid* grid = (_dbTrackGrid*) this;
  assert(i < (int) grid->x_origin_.size());
  origin_x = grid->x_origin_[i];
  line_count = grid->x_count_[i];
  step = grid->x_step_[i];
}

void dbTrackGrid::getGridPatternX(int i,
                                  int& origin_x,
                                  int& line_count,
                                  int& step,
                                  int& first_mask,
                                  bool& samemask)
{
  _dbTrackGrid* grid = (_dbTrackGrid*) this;
  assert(i < (int) grid->x_origin_.size());
  origin_x = grid->x_origin_[i];
  line_count = grid->x_count_[i];
  step = grid->x_step_[i];
  first_mask = grid->first_mask_[i];
  samemask = grid->samemask_[i];
}

void dbTrackGrid::getGridPatternY(int i,
                                  int& origin_y,
                                  int& line_count,
                                  int& step)
{
  _dbTrackGrid* grid = (_dbTrackGrid*) this;
  assert(i < (int) grid->y_origin_.size());
  origin_y = grid->y_origin_[i];
  line_count = grid->y_count_[i];
  step = grid->y_step_[i];
}

void dbTrackGrid::getGridPatternY(int i,
                                  int& origin_y,
                                  int& line_count,
                                  int& step,
                                  int& first_mask,
                                  bool& samemask)
{
  _dbTrackGrid* grid = (_dbTrackGrid*) this;
  assert(i < (int) grid->y_origin_.size());
  origin_y = grid->y_origin_[i];
  line_count = grid->y_count_[i];
  step = grid->y_step_[i];
  first_mask = grid->first_mask_[i];
  samemask = grid->samemask_[i];
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

  _dbTrackGrid* grid = block->track_grid_tbl_->create();
  grid->layer_ = layer_->getImpl()->getOID();
  return (dbTrackGrid*) grid;
}

dbTrackGrid* dbTrackGrid::getTrackGrid(dbBlock* block_, uint32_t dbid_)
{
  _dbBlock* block = (_dbBlock*) block_;
  return (dbTrackGrid*) block->track_grid_tbl_->getPtr(dbid_);
}
void dbTrackGrid::destroy(dbTrackGrid* grid_)
{
  _dbTrackGrid* grid = (_dbTrackGrid*) grid_;
  _dbBlock* block = (_dbBlock*) grid->getOwner();
  dbProperty::destroyProperties(grid);
  block->track_grid_tbl_->destroy(grid);
}

void _dbTrackGrid::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);

  info.children["x_origin"].add(x_origin_);
  info.children["x_count"].add(x_count_);
  info.children["x_step"].add(x_step_);
  info.children["y_origin"].add(y_origin_);
  info.children["y_count"].add(y_count_);
  info.children["y_step"].add(y_step_);
  info.children["first_mask"].add(first_mask_);
  info.children["samemask"].add(samemask_);
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
