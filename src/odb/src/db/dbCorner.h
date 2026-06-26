// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// Generator Code Begin Header
#pragma once

#include <cstdint>
#include <string>

#include "dbCore.h"

namespace odb {
class dbIStream;
class dbOStream;
class _dbDatabase;

class _dbCorner : public _dbObject
{
 public:
  _dbCorner(_dbDatabase*);

  bool operator==(const _dbCorner& rhs) const;
  bool operator!=(const _dbCorner& rhs) const { return !operator==(rhs); }
  bool operator<(const _dbCorner& rhs) const;
  void collectMemInfo(MemInfo& info);

  std::string name_;
};
dbIStream& operator>>(dbIStream& stream, _dbCorner& obj);
dbOStream& operator<<(dbOStream& stream, const _dbCorner& obj);
}  // namespace odb
// Generator Code End Header