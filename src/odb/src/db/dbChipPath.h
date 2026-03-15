// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// Generator Code Begin Header
#pragma once

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include "dbCore.h"

namespace odb {
class dbIStream;
class dbOStream;
class _dbDatabase;

class _dbChipPath : public _dbObject
{
 public:
  _dbChipPath(_dbDatabase*);

  bool operator==(const _dbChipPath& rhs) const;
  bool operator!=(const _dbChipPath& rhs) const { return !operator==(rhs); }
  bool operator<(const _dbChipPath& rhs) const;
  void collectMemInfo(MemInfo& info);

  std::string name_;
  std::vector<std::pair<std::string, bool>> entries_;
};
dbIStream& operator>>(dbIStream& stream, _dbChipPath& obj);
dbOStream& operator<<(dbOStream& stream, const _dbChipPath& obj);
}  // namespace odb
// Generator Code End Header
