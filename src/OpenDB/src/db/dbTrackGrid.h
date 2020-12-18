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

#pragma once

#include "dbCore.h"
#include "dbId.h"
#include "dbTypes.h"
#include "dbVector.h"
#include "odb.h"

namespace odb {

class _dbTechLayer;
class _dbDatabase;
class dbIStream;
class dbOStream;
class dbDiff;

class _dbTrackGrid : public _dbObject
{
 public:
  dbId<_dbTechLayer> _layer;
  dbVector<int>      _x_origin;
  dbVector<int>      _x_count;
  dbVector<int>      _x_step;
  dbVector<int>      _y_origin;
  dbVector<int>      _y_count;
  dbVector<int>      _y_step;
  dbId<_dbTechLayer> _next_grid;

  _dbTrackGrid(_dbDatabase*, const _dbTrackGrid& g);
  _dbTrackGrid(_dbDatabase*);
  ~_dbTrackGrid();

  bool operator==(const _dbTrackGrid& rhs) const;
  bool operator!=(const _dbTrackGrid& rhs) const { return !operator==(rhs); }

  bool operator<(const _dbTrackGrid& rhs) const
  {
    if (_layer < rhs._layer)
      return true;

    if (_layer > rhs._layer)
      return false;

    return false;
  }

  void differences(dbDiff&             diff,
                   const char*         field,
                   const _dbTrackGrid& rhs) const;
  void out(dbDiff& diff, char side, const char* field) const;
};

inline _dbTrackGrid::_dbTrackGrid(_dbDatabase*, const _dbTrackGrid& g)
    : _layer(g._layer),
      _x_origin(g._x_origin),
      _x_count(g._x_count),
      _x_step(g._x_step),
      _y_origin(g._y_origin),
      _y_count(g._y_count),
      _y_step(g._y_step),
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
  stream << grid._next_grid;
  return stream;
}

inline dbIStream& operator>>(dbIStream& stream, _dbTrackGrid& grid)
{
  stream >> grid._layer;
  stream >> grid._x_origin;
  stream >> grid._x_count;
  stream >> grid._x_step;
  stream >> grid._y_origin;
  stream >> grid._y_count;
  stream >> grid._y_step;
  stream >> grid._next_grid;
  return stream;
}

}  // namespace odb
