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
#pragma once

#include "dbCore.h"
#include "odb.h"

// User Code Begin includes
#include <map>

#include "dbVector.h"
// User Code End includes

namespace odb {

class dbIStream;
class dbOStream;
class dbDiff;
class _dbDatabase;
// User Code Begin Classes
class _dbTechLayer;
// User Code End Classes

struct GCellData
{
  uint _horizontal_usage    = 0;
  uint _vertical_usage      = 0;
  uint _up_usage            = 0;
  uint _horizontal_capacity = 0;
  uint _vertical_capacity   = 0;
  uint _up_capacity         = 0;
};
// User Code Begin structs
// User Code End structs

class _dbGCellGrid : public _dbObject
{
 public:
  // User Code Begin enums
  // User Code End enums

  dbVector<int> _x_origin;
  dbVector<int> _x_count;
  dbVector<int> _x_step;
  dbVector<int> _y_origin;
  dbVector<int> _y_count;
  dbVector<int> _y_step;
  std::map<dbId<_dbTechLayer>, std::map<std::pair<int, int>, GCellData>>
      _congestion_map;

  // User Code Begin fields
  // User Code End fields
  _dbGCellGrid(_dbDatabase*, const _dbGCellGrid& r);
  _dbGCellGrid(_dbDatabase*);
  ~_dbGCellGrid();
  bool operator==(const _dbGCellGrid& rhs) const;
  bool operator!=(const _dbGCellGrid& rhs) const { return !operator==(rhs); }
  bool operator<(const _dbGCellGrid& rhs) const;
  void differences(dbDiff&             diff,
                   const char*         field,
                   const _dbGCellGrid& rhs) const;
  void out(dbDiff& diff, char side, const char* field) const;
  // User Code Begin methods
  // User Code End methods
};
dbIStream& operator>>(dbIStream& stream, _dbGCellGrid& obj);
dbOStream& operator<<(dbOStream& stream, const _dbGCellGrid& obj);
// User Code Begin general
dbIStream& operator>>(dbIStream& stream, GCellData& obj);
dbOStream& operator<<(dbOStream& stream, const GCellData& obj);
// User Code End general
}  // namespace odb
// Generator Code End 1
