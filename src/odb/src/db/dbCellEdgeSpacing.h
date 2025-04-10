// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

// Generator Code Begin Header
#pragma once

#include <string>

#include "dbCore.h"
#include "odb/odb.h"

namespace odb {
class dbIStream;
class dbOStream;
class _dbDatabase;

struct dbCellEdgeSpacingFlags
{
  bool except_abutted_ : 1;
  bool except_non_filler_in_between_ : 1;
  bool optional_ : 1;
  bool soft_ : 1;
  bool exact_ : 1;
  uint spare_bits_ : 27;
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
  int spacing;
};
dbIStream& operator>>(dbIStream& stream, _dbCellEdgeSpacing& obj);
dbOStream& operator<<(dbOStream& stream, const _dbCellEdgeSpacing& obj);
}  // namespace odb
   // Generator Code End Header
