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

class _dbGDSBox : public _dbObject
{
 public:
  _dbGDSBox(_dbDatabase*);

  bool operator==(const _dbGDSBox& rhs) const;
  bool operator!=(const _dbGDSBox& rhs) const { return !operator==(rhs); }
  bool operator<(const _dbGDSBox& rhs) const;
  void collectMemInfo(MemInfo& info);

  int16_t _layer;
  int16_t _datatype;
  Rect _bounds;
  std::vector<std::pair<std::int16_t, std::string>> _propattr;
};
dbIStream& operator>>(dbIStream& stream, _dbGDSBox& obj);
dbOStream& operator<<(dbOStream& stream, const _dbGDSBox& obj);
}  // namespace odb
   // Generator Code End Header
