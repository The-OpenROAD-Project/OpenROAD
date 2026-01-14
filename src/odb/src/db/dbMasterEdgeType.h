// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

// Generator Code Begin Header
#pragma once

#include <cstdint>
#include <string>

#include "dbCore.h"

namespace odb {
class dbIStream;
class dbOStream;
class _dbDatabase;

class _dbMasterEdgeType : public _dbObject
{
 public:
  _dbMasterEdgeType(_dbDatabase*);

  bool operator==(const _dbMasterEdgeType& rhs) const;
  bool operator!=(const _dbMasterEdgeType& rhs) const
  {
    return !operator==(rhs);
  }
  bool operator<(const _dbMasterEdgeType& rhs) const;
  void collectMemInfo(MemInfo& info);

  uint32_t edge_dir_;
  std::string edge_type_;
  int cell_row_;
  int half_row_;
  int range_begin_;
  int range_end_;
};
dbIStream& operator>>(dbIStream& stream, _dbMasterEdgeType& obj);
dbOStream& operator<<(dbOStream& stream, const _dbMasterEdgeType& obj);
}  // namespace odb
   // Generator Code End Header
