// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

// Generator Code Begin Header
#pragma once

#include "dbCore.h"
#include "dbVector.h"
#include "odb/odb.h"
// User Code Begin Includes
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

  char* _name;
  dbId<_dbPowerDomain> _next_entry;
  dbVector<std::string> _elements;
  dbVector<dbId<_dbPowerSwitch>> _power_switch;
  dbVector<dbId<_dbIsolation>> _isolation;
  dbId<_dbGroup> _group;
  bool _top;
  dbId<_dbPowerDomain> _parent;
  Rect _area;
  dbVector<dbId<_dbLevelShifter>> _levelshifters;
  float _voltage;
};
dbIStream& operator>>(dbIStream& stream, _dbPowerDomain& obj);
dbOStream& operator<<(dbOStream& stream, const _dbPowerDomain& obj);
}  // namespace odb
   // Generator Code End Header
