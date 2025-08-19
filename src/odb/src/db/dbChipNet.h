// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// Generator Code Begin Header
#pragma once

#include <string>
#include <utility>
#include <vector>

#include "dbCore.h"
#include "odb/dbId.h"
#include "odb/odb.h"
// User Code Begin Includes
// User Code End Includes

namespace odb {
// User Code Begin Consts
// User Code End Consts
class dbIStream;
class dbOStream;
class _dbDatabase;
class _dbChip;
class _dbChipInst;
// User Code Begin Classes
class _dbChipBumpInst;
// User Code End Classes

// User Code Begin Structs
// User Code End Structs

class _dbChipNet : public _dbObject
{
 public:
  // User Code Begin Enums
  // User Code End Enums

  _dbChipNet(_dbDatabase*);

  bool operator==(const _dbChipNet& rhs) const;
  bool operator!=(const _dbChipNet& rhs) const { return !operator==(rhs); }
  bool operator<(const _dbChipNet& rhs) const;
  void collectMemInfo(MemInfo& info);
  // User Code Begin Methods
  // User Code End Methods

  std::string name_;
  dbId<_dbChip> chip_;
  dbId<_dbChipNet> chip_net_next_;
  std::vector<std::pair<std::vector<dbId<_dbChipInst>>, dbId<_dbChipBumpInst>>>
      bump_insts_paths_;

  // User Code Begin Fields
  // User Code End Fields
};
dbIStream& operator>>(dbIStream& stream, _dbChipNet& obj);
dbOStream& operator<<(dbOStream& stream, const _dbChipNet& obj);
// User Code Begin General
// User Code End General
}  // namespace odb
// Generator Code End Header