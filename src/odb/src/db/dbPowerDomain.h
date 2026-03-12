// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

// Generator Code Begin Header
#pragma once

#include <cstdint>

#include "dbCore.h"
#include "dbVector.h"
#include "odb/dbId.h"
// User Code Begin Includes
#include <string>

#include "odb/geom.h"
// User Code End Includes

namespace odb {
class dbIStream;
class dbOStream;
class _dbDatabase;
class _dbPowerSwitch;
class _dbIsolation;
class _dbGroup;
class _dbLevelShifter;

class _dbPowerDomain : public _dbObject
{
 public:
  _dbPowerDomain(_dbDatabase*);

  ~_dbPowerDomain();

  bool operator==(const _dbPowerDomain& rhs) const;
  bool operator!=(const _dbPowerDomain& rhs) const { return !operator==(rhs); }
  bool operator<(const _dbPowerDomain& rhs) const;
  void collectMemInfo(MemInfo& info);

  char* name_;
  dbId<_dbPowerDomain> next_entry_;
  dbVector<std::string> elements_;
  dbVector<dbId<_dbPowerSwitch>> power_switch_;
  dbVector<dbId<_dbIsolation>> isolation_;
  dbId<_dbGroup> group_;
  bool top_;
  dbId<_dbPowerDomain> parent_;
  Rect area_;
  dbVector<dbId<_dbLevelShifter>> levelshifters_;
  float voltage_;
};
dbIStream& operator>>(dbIStream& stream, _dbPowerDomain& obj);
dbOStream& operator<<(dbOStream& stream, const _dbPowerDomain& obj);
}  // namespace odb
   // Generator Code End Header
