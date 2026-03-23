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
#include "odb/dbTypes.h"
#include "odb/geom.h"
// User Code End Includes

namespace odb {
class dbIStream;
class dbOStream;
class _dbDatabase;

class _dbGDSText : public _dbObject
{
 public:
  _dbGDSText(_dbDatabase*);

  bool operator==(const _dbGDSText& rhs) const;
  bool operator!=(const _dbGDSText& rhs) const { return !operator==(rhs); }
  bool operator<(const _dbGDSText& rhs) const;
  void collectMemInfo(MemInfo& info);

  int16_t layer_;
  int16_t datatype_;
  Point origin_;
  std::vector<std::pair<std::int16_t, std::string>> propattr_;
  dbGDSTextPres presentation_;
  dbGDSSTrans transform_;
  std::string text_;
};
dbIStream& operator>>(dbIStream& stream, _dbGDSText& obj);
dbOStream& operator<<(dbOStream& stream, const _dbGDSText& obj);
}  // namespace odb
   // Generator Code End Header
