// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

// Generator Code Begin Header
#pragma once

#include <map>
#include <string>

#include "dbCore.h"
#include "dbVector.h"
#include "odb/db.h"
#include "odb/dbSet.h"
#include "odb/odb.h"

namespace odb {
class dbIStream;
class dbOStream;
class _dbDatabase;
class _dbMaster;
class _dbLib;
class _dbMTerm;
class _dbPowerDomain;

class _dbPowerSwitch : public _dbObject
{
 public:
  _dbPowerSwitch(_dbDatabase*);

  ~_dbPowerSwitch();

  bool operator==(const _dbPowerSwitch& rhs) const;
  bool operator!=(const _dbPowerSwitch& rhs) const { return !operator==(rhs); }
  bool operator<(const _dbPowerSwitch& rhs) const;
  void collectMemInfo(MemInfo& info);

  char* _name;
  dbId<_dbPowerSwitch> _next_entry;
  dbVector<dbPowerSwitch::UPFIOSupplyPort> _in_supply_port;
  dbPowerSwitch::UPFIOSupplyPort _out_supply_port;
  dbVector<dbPowerSwitch::UPFControlPort> _control_port;
  dbVector<dbPowerSwitch::UPFAcknowledgePort> _acknowledge_port;
  dbVector<dbPowerSwitch::UPFOnState> _on_state;
  dbId<_dbMaster> _lib_cell;
  dbId<_dbLib> _lib;
  std::map<std::string, dbId<_dbMTerm>> _port_map;
  dbId<_dbPowerDomain> _power_domain;
};
dbIStream& operator>>(dbIStream& stream, _dbPowerSwitch& obj);
dbOStream& operator<<(dbOStream& stream, const _dbPowerSwitch& obj);
}  // namespace odb
   // Generator Code End Header
