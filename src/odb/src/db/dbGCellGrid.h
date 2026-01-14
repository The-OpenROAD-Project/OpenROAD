// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// Generator Code Begin Header
#pragma once

#include <cstdint>
#include <map>

#include "dbCore.h"
#include "dbVector.h"
#include "odb/db.h"
#include "odb/dbId.h"
// User Code Begin Includes
#include "odb/dbMatrix.h"
// User Code End Includes

namespace odb {
class dbIStream;
class dbOStream;
class _dbDatabase;
// User Code Begin Classes
class _dbTechLayer;

// User Code End Classes

struct dbGCellGridFlags
{
  bool x_grid_valid : 1;
  bool y_grid_valid : 1;
  uint32_t spare_bits : 30;
};

class _dbGCellGrid : public _dbObject
{
 public:
  _dbGCellGrid(_dbDatabase*);

  bool operator==(const _dbGCellGrid& rhs) const;
  bool operator!=(const _dbGCellGrid& rhs) const { return !operator==(rhs); }
  bool operator<(const _dbGCellGrid& rhs) const;
  void collectMemInfo(MemInfo& info);
  // User Code Begin Methods
  dbMatrix<dbGCellGrid::GCellData>& get(const dbId<_dbTechLayer>& lid);
  dbTechLayer* getLayer(const dbId<_dbTechLayer>& lid) const;
  // User Code End Methods

  dbGCellGridFlags flags_;
  dbVector<int> x_origin_;
  dbVector<int> x_count_;
  dbVector<int> x_step_;
  dbVector<int> y_origin_;
  dbVector<int> y_count_;
  dbVector<int> y_step_;
  dbVector<int> x_grid_;
  dbVector<int> y_grid_;
  std::map<dbId<_dbTechLayer>, dbMatrix<dbGCellGrid::GCellData>>
      congestion_map_;
};
dbIStream& operator>>(dbIStream& stream, _dbGCellGrid& obj);
dbOStream& operator<<(dbOStream& stream, const _dbGCellGrid& obj);
// User Code Begin General
dbIStream& operator>>(dbIStream& stream, dbGCellGrid::GCellData& obj);
// User Code End General
}  // namespace odb
// Generator Code End Header
