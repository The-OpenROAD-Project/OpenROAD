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

class _dbGDSSRef : public _dbObject
{
 public:
  _dbGDSSRef(_dbDatabase*);

  bool operator==(const _dbGDSSRef& rhs) const;
  bool operator!=(const _dbGDSSRef& rhs) const { return !operator==(rhs); }
  bool operator<(const _dbGDSSRef& rhs) const;
  void collectMemInfo(MemInfo& info);

  Point _origin;
  std::vector<std::pair<std::int16_t, std::string>> _propattr;
  dbGDSSTrans _transform;
  dbId<_dbGDSStructure> _structure;
};
dbIStream& operator>>(dbIStream& stream, _dbGDSSRef& obj);
dbOStream& operator<<(dbOStream& stream, const _dbGDSSRef& obj);
}  // namespace odb
   // Generator Code End Header
