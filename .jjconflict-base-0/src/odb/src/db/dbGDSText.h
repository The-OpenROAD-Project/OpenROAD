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

class _dbGDSText : public _dbObject
{
 public:
  _dbGDSText(_dbDatabase*);

  bool operator==(const _dbGDSText& rhs) const;
  bool operator!=(const _dbGDSText& rhs) const { return !operator==(rhs); }
  bool operator<(const _dbGDSText& rhs) const;
  void collectMemInfo(MemInfo& info);

  int16_t _layer;
  int16_t _datatype;
  Point _origin;
  std::vector<std::pair<std::int16_t, std::string>> _propattr;
  dbGDSTextPres _presentation;
  dbGDSSTrans _transform;
  std::string _text;
};
dbIStream& operator>>(dbIStream& stream, _dbGDSText& obj);
dbOStream& operator<<(dbOStream& stream, const _dbGDSText& obj);
}  // namespace odb
   // Generator Code End Header
