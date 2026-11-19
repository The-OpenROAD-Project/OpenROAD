// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

// Generator Code Begin Header
#pragma once

#include <cstdint>
#include <map>
#include <string>

#include "dbCore.h"
#include "dbVector.h"
#include "odb/db.h"
#include "odb/dbId.h"
#include "odb/dbSet.h"

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

  char* name_;
  dbId<_dbPowerSwitch> next_entry_;
  dbVector<dbPowerSwitch::UPFIOSupplyPort> in_supply_port_;
  dbPowerSwitch::UPFIOSupplyPort out_supply_port_;
  dbVector<dbPowerSwitch::UPFControlPort> control_port_;
  dbVector<dbPowerSwitch::UPFAcknowledgePort> acknowledge_port_;
  dbVector<dbPowerSwitch::UPFOnState> on_state_;
  dbId<_dbMaster> lib_cell_;
  dbId<_dbLib> lib_;
  std::map<std::string, dbId<_dbMTerm>> port_map_;
  dbId<_dbPowerDomain> power_domain_;
};
dbIStream& operator>>(dbIStream& stream, _dbPowerSwitch& obj);
dbOStream& operator<<(dbOStream& stream, const _dbPowerSwitch& obj);
}  // namespace odb
   // Generator Code End Header
