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

// Generator Code Begin Header
#pragma once

#include "dbCore.h"
#include "odb.h"

// User Code Begin includes
#include <map>

#include "db.h"
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

struct dbGCellGridFlags
{
  bool x_grid_valid_ : 1;
  bool y_grid_valid_ : 1;
  uint spare_bits_ : 30;
};
// User Code Begin structs
// User Code End structs

class _dbGCellGrid : public _dbObject
{
 public:
  // User Code Begin enums
  // User Code End enums

  dbGCellGridFlags flags_;
  dbVector<int>    x_origin_;
  dbVector<int>    x_count_;
  dbVector<int>    x_step_;
  dbVector<int>    y_origin_;
  dbVector<int>    y_count_;
  dbVector<int>    y_step_;
  dbVector<int>    x_grid_;
  dbVector<int>    y_grid_;
  std::map<dbId<_dbTechLayer>,
           std::map<std::pair<uint, uint>, dbGCellGrid::GCellData>>
      congestion_map_;

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
  bool gcellExists(dbId<_dbTechLayer> lid, uint x_idx, uint y_idx) const;
  // User Code End methods
};
dbIStream& operator>>(dbIStream& stream, _dbGCellGrid& obj);
dbOStream& operator<<(dbOStream& stream, const _dbGCellGrid& obj);
// User Code Begin general
dbIStream& operator>>(dbIStream& stream, dbGCellGrid::GCellData& obj);
dbOStream& operator<<(dbOStream& stream, const dbGCellGrid::GCellData& obj);
// User Code End general
}  // namespace odb
// Generator Code End Header
