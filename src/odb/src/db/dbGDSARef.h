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
#include "odb/odb.h"

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

  Point _origin;
  Point _lr;
  Point _ul;
  std::vector<std::pair<std::int16_t, std::string>> _propattr;
  dbGDSSTrans _transform;
  int16_t _num_rows;
  int16_t _num_columns;
  dbId<_dbGDSStructure> _structure;
};
dbIStream& operator>>(dbIStream& stream, _dbGDSARef& obj);
dbOStream& operator<<(dbOStream& stream, const _dbGDSARef& obj);
}  // namespace odb
   // Generator Code End Header
