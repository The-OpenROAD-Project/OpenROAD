// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// Generator Code Begin Header
#pragma once

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include "dbCore.h"
#include "odb/dbId.h"

namespace odb {
class dbIStream;
class dbOStream;
class _dbDatabase;
class _dbChip;
class _dbChipInst;
// User Code Begin Classes
class _dbChipBumpInst;
// User Code End Classes

class _dbChipNet : public _dbObject
{
 public:
  _dbChipNet(_dbDatabase*);

  bool operator==(const _dbChipNet& rhs) const;
  bool operator!=(const _dbChipNet& rhs) const { return !operator==(rhs); }
  bool operator<(const _dbChipNet& rhs) const;
  void collectMemInfo(MemInfo& info);

  std::string name_;
  dbId<_dbChip> chip_;
  dbId<_dbChipNet> chip_net_next_;
  std::vector<std::pair<std::vector<dbId<_dbChipInst>>, dbId<_dbChipBumpInst>>>
      bump_insts_paths_;
};
dbIStream& operator>>(dbIStream& stream, _dbChipNet& obj);
dbOStream& operator<<(dbOStream& stream, const _dbChipNet& obj);
}  // namespace odb
// Generator Code End Header