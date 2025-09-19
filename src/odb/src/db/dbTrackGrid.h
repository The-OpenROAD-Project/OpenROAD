// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include "dbCore.h"
#include "dbDatabase.h"
#include "dbVector.h"
#include "odb/dbId.h"
#include "odb/dbTypes.h"
#include "odb/odb.h"

namespace odb {

class _dbTechLayer;
class _dbDatabase;
class dbIStream;
class dbOStream;

class _dbTrackGrid : public _dbObject
{
 public:
  dbId<_dbTechLayer> _layer;
  dbVector<int> _x_origin;
  dbVector<int> _x_count;
  dbVector<int> _x_step;
  dbVector<int> _y_origin;
  dbVector<int> _y_count;
  dbVector<int> _y_step;
  dbVector<int> _first_mask;
  dbVector<bool> _samemask;
  dbId<_dbTechLayer> _next_grid;

  _dbTrackGrid(_dbDatabase*, const _dbTrackGrid& g);
  _dbTrackGrid(_dbDatabase*);
  ~_dbTrackGrid();

  bool operator==(const _dbTrackGrid& rhs) const;
  bool operator!=(const _dbTrackGrid& rhs) const { return !operator==(rhs); }

  bool operator<(const _dbTrackGrid& rhs) const
  {
    if (_layer < rhs._layer) {
      return true;
    }

    if (_layer > rhs._layer) {
      return false;
    }

    return false;
  }

  void collectMemInfo(MemInfo& info);

  void getAverageTrackPattern(bool is_x,
                              int& track_init,
                              int& num_tracks,
                              int& track_step);
};

inline _dbTrackGrid::_dbTrackGrid(_dbDatabase*, const _dbTrackGrid& g)
    : _layer(g._layer),
      _x_origin(g._x_origin),
      _x_count(g._x_count),
      _x_step(g._x_step),
      _y_origin(g._y_origin),
      _y_count(g._y_count),
      _y_step(g._y_step),
      _first_mask(g._first_mask),
      _samemask(g._samemask),
      _next_grid(g._next_grid)
{
}

inline _dbTrackGrid::_dbTrackGrid(_dbDatabase*)
{
}

inline _dbTrackGrid::~_dbTrackGrid()
{
}

inline dbOStream& operator<<(dbOStream& stream, const _dbTrackGrid& grid)
{
  stream << grid._layer;
  stream << grid._x_origin;
  stream << grid._x_count;
  stream << grid._x_step;
  stream << grid._y_origin;
  stream << grid._y_count;
  stream << grid._y_step;
  stream << grid._first_mask;
  stream << grid._samemask;
  stream << grid._next_grid;
  return stream;
}

inline dbIStream& operator>>(dbIStream& stream, _dbTrackGrid& grid)
{
  _dbDatabase* db = grid.getImpl()->getDatabase();
  stream >> grid._layer;
  stream >> grid._x_origin;
  stream >> grid._x_count;
  stream >> grid._x_step;
  stream >> grid._y_origin;
  stream >> grid._y_count;
  stream >> grid._y_step;
  if (db->isSchema(db_track_mask)) {
    stream >> grid._first_mask;
    stream >> grid._samemask;
  } else {
    grid._first_mask.push_back(0);
    grid._samemask.push_back(false);
  }
  stream >> grid._next_grid;
  return stream;
}

}  // namespace odb
