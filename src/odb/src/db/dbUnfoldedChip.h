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
class _dbUnfoldedRegion;

class _dbUnfoldedChip : public _dbObject
{
 public:
  _dbUnfoldedChip(_dbDatabase*);

  bool operator==(const _dbUnfoldedChip& rhs) const;
  bool operator!=(const _dbUnfoldedChip& rhs) const { return !operator==(rhs); }
  bool operator<(const _dbUnfoldedChip& rhs) const;
  void collectMemInfo(MemInfo& info);

  std::string name_;
  dbVector<dbId<_dbChipInst>> chip_inst_path_;
  dbTransform transform_;
  dbId<_dbUnfoldedRegion> region_;
};
dbIStream& operator>>(dbIStream& stream, _dbUnfoldedChip& obj);
dbOStream& operator<<(dbOStream& stream, const _dbUnfoldedChip& obj);
}  // namespace odb
// Generator Code End Header