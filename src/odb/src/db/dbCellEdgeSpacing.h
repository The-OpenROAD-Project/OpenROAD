// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

// Generator Code Begin Header
#pragma once

#include <cstdint>
#include <string>

#include "dbCore.h"

namespace odb {
class dbIStream;
class dbOStream;
class _dbDatabase;

struct dbCellEdgeSpacingFlags
{
  bool except_abutted : 1;
  bool except_non_filler_in_between : 1;
  bool optional : 1;
  bool soft : 1;
  bool exact : 1;
  uint32_t spare_bits : 27;
};

class _dbCellEdgeSpacing : public _dbObject
{
 public:
  _dbCellEdgeSpacing(_dbDatabase*);

  bool operator==(const _dbCellEdgeSpacing& rhs) const;
  bool operator!=(const _dbCellEdgeSpacing& rhs) const
  {
    return !operator==(rhs);
  }
  bool operator<(const _dbCellEdgeSpacing& rhs) const;
  void collectMemInfo(MemInfo& info);

  dbCellEdgeSpacingFlags flags_;
  std::string first_edge_type_;
  std::string second_edge_type_;
  int spacing_;
};
dbIStream& operator>>(dbIStream& stream, _dbCellEdgeSpacing& obj);
dbOStream& operator<<(dbOStream& stream, const _dbCellEdgeSpacing& obj);
}  // namespace odb
   // Generator Code End Header
