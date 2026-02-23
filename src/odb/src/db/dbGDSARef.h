// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

// Generator Code Begin Header
#pragma once

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include "dbCore.h"
#include "dbGDSStructure.h"
#include "odb/dbId.h"
// User Code Begin Includes
#include "odb/dbTypes.h"
#include "odb/geom.h"
// User Code End Includes

namespace odb {
class dbIStream;
class dbOStream;
class _dbDatabase;
class _dbGDSStructure;

class _dbGDSARef : public _dbObject
{
 public:
  _dbGDSARef(_dbDatabase*);

  bool operator==(const _dbGDSARef& rhs) const;
  bool operator!=(const _dbGDSARef& rhs) const { return !operator==(rhs); }
  bool operator<(const _dbGDSARef& rhs) const;
  void collectMemInfo(MemInfo& info);

  Point origin_;
  Point lr_;
  Point ul_;
  std::vector<std::pair<std::int16_t, std::string>> propattr_;
  dbGDSSTrans transform_;
  int16_t num_rows_;
  int16_t num_columns_;
  dbId<_dbGDSStructure> structure_;
};
dbIStream& operator>>(dbIStream& stream, _dbGDSARef& obj);
dbOStream& operator<<(dbOStream& stream, const _dbGDSARef& obj);
}  // namespace odb
   // Generator Code End Header
