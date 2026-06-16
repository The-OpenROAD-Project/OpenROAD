// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// Generator Code Begin Header
#pragma once

#include <cstdint>
#include <string>

#include "dbCore.h"
#include "dbVector.h"
#include "odb/dbId.h"
#include "odb/dbTransform.h"
#include "odb/geom.h"

namespace odb {
class dbIStream;
class dbOStream;
class _dbDatabase;
class _dbChipInst;
class _dbUnfoldedChipRegionInst;

class _dbUnfoldedChipInst : public _dbObject
{
 public:
  _dbUnfoldedChipInst(_dbDatabase*);

  bool operator==(const _dbUnfoldedChipInst& rhs) const;
  bool operator!=(const _dbUnfoldedChipInst& rhs) const
  {
    return !operator==(rhs);
  }
  bool operator<(const _dbUnfoldedChipInst& rhs) const;
  void collectMemInfo(MemInfo& info);

  std::string name_;
  dbVector<dbId<_dbChipInst>> chip_inst_path_;
  dbTransform transform_;
  dbId<_dbUnfoldedChipRegionInst> region_;
};
dbIStream& operator>>(dbIStream& stream, _dbUnfoldedChipInst& obj);
dbOStream& operator<<(dbOStream& stream, const _dbUnfoldedChipInst& obj);
}  // namespace odb
// Generator Code End Header