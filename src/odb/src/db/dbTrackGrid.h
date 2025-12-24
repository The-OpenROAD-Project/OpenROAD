// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <vector>

#include "dbCore.h"
#include "dbDatabase.h"
#include "dbVector.h"
#include "odb/dbId.h"
#include "odb/dbTypes.h"

namespace odb {

class _dbTechLayer;
class _dbDatabase;
class dbIStream;
class dbOStream;

class _dbTrackGrid : public _dbObject
{
 public:
  _dbTrackGrid(_dbDatabase*, const _dbTrackGrid& g);
  _dbTrackGrid(_dbDatabase*);

  bool operator==(const _dbTrackGrid& rhs) const;
  bool operator!=(const _dbTrackGrid& rhs) const { return !operator==(rhs); }

  bool operator<(const _dbTrackGrid& rhs) const
  {
    if (layer_ < rhs.layer_) {
      return true;
    }

    if (layer_ > rhs.layer_) {
      return false;
    }

    return false;
  }

  void collectMemInfo(MemInfo& info);

  void getAverageTrackPattern(bool is_x,
                              int& track_init,
                              int& num_tracks,
                              int& track_step);

  dbId<_dbTechLayer> layer_;
  dbVector<int> x_origin_;
  dbVector<int> x_count_;
  dbVector<int> x_step_;
  dbVector<int> y_origin_;
  dbVector<int> y_count_;
  dbVector<int> y_step_;
  dbVector<int> first_mask_;
  dbVector<bool> samemask_;
  dbId<_dbTechLayer> next_grid_;

  // Transient
  std::vector<int> grid_x_;
  std::vector<int> grid_y_;
};

inline _dbTrackGrid::_dbTrackGrid(_dbDatabase*, const _dbTrackGrid& g)
    : layer_(g.layer_),
      x_origin_(g.x_origin_),
      x_count_(g.x_count_),
      x_step_(g.x_step_),
      y_origin_(g.y_origin_),
      y_count_(g.y_count_),
      y_step_(g.y_step_),
      first_mask_(g.first_mask_),
      samemask_(g.samemask_),
      next_grid_(g.next_grid_)
{
}

inline _dbTrackGrid::_dbTrackGrid(_dbDatabase*)
{
}

inline dbOStream& operator<<(dbOStream& stream, const _dbTrackGrid& grid)
{
  stream << grid.layer_;
  stream << grid.x_origin_;
  stream << grid.x_count_;
  stream << grid.x_step_;
  stream << grid.y_origin_;
  stream << grid.y_count_;
  stream << grid.y_step_;
  stream << grid.first_mask_;
  stream << grid.samemask_;
  stream << grid.next_grid_;
  return stream;
}

inline dbIStream& operator>>(dbIStream& stream, _dbTrackGrid& grid)
{
  _dbDatabase* db = grid.getImpl()->getDatabase();
  stream >> grid.layer_;
  stream >> grid.x_origin_;
  stream >> grid.x_count_;
  stream >> grid.x_step_;
  stream >> grid.y_origin_;
  stream >> grid.y_count_;
  stream >> grid.y_step_;
  if (db->isSchema(kSchemaTrackMask)) {
    stream >> grid.first_mask_;
    stream >> grid.samemask_;
  } else {
    grid.first_mask_.push_back(0);
    grid.samemask_.push_back(false);
  }
  stream >> grid.next_grid_;
  return stream;
}

}  // namespace odb
