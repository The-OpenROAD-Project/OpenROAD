// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

// Generator Code Begin Header
#pragma once

#include <cstdint>
#include <string>

#include "dbCore.h"
#include "odb/dbId.h"
// User Code Begin Includes
#include "dbVector.h"
#include "odb/db.h"
// User Code End Includes

namespace odb {
class dbIStream;
class dbOStream;
class _dbDatabase;
class _dbTechLayer;

class _dbMetalWidthViaMap : public _dbObject
{
 public:
  _dbMetalWidthViaMap(_dbDatabase*);

  bool operator==(const _dbMetalWidthViaMap& rhs) const;
  bool operator!=(const _dbMetalWidthViaMap& rhs) const
  {
    return !operator==(rhs);
  }
  bool operator<(const _dbMetalWidthViaMap& rhs) const;
  void collectMemInfo(MemInfo& info);

  bool via_cut_class_;
  dbId<_dbTechLayer> cut_layer_;
  int below_layer_width_low_;
  int below_layer_width_high_;
  int above_layer_width_low_;
  int above_layer_width_high_;
  std::string via_name_;
  bool pg_via_;
};
dbIStream& operator>>(dbIStream& stream, _dbMetalWidthViaMap& obj);
dbOStream& operator<<(dbOStream& stream, const _dbMetalWidthViaMap& obj);
}  // namespace odb
   // Generator Code End Header
