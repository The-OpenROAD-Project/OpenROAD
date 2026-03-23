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
// User Code Begin Includes
#include "odb/geom.h"
// User Code End Includes

namespace odb {
class dbIStream;
class dbOStream;
class _dbDatabase;

class _dbGDSPath : public _dbObject
{
 public:
  _dbGDSPath(_dbDatabase*);

  bool operator==(const _dbGDSPath& rhs) const;
  bool operator!=(const _dbGDSPath& rhs) const { return !operator==(rhs); }
  bool operator<(const _dbGDSPath& rhs) const;
  void collectMemInfo(MemInfo& info);

  int16_t layer_;
  int16_t datatype_;
  std::vector<Point> xy_;
  std::vector<std::pair<std::int16_t, std::string>> propattr_;
  int width_;
  int16_t path_type_;
};
dbIStream& operator>>(dbIStream& stream, _dbGDSPath& obj);
dbOStream& operator<<(dbOStream& stream, const _dbGDSPath& obj);
}  // namespace odb
   // Generator Code End Header
