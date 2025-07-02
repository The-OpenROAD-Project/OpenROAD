// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbPowerState.h"

#include "dbBlock.h"
#include "dbDatabase.h"
#include "dbHashTable.hpp"
#include "dbMTerm.h"
#include "dbMaster.h"
#include "dbNet.h"
#include "dbTable.h"
#include "dbTable.hpp"
#include "odb/db.h"
#include "odb/dbMap.h"
#include "utl/Logger.h"
namespace odb {
template class dbTable<_dbPowerState>;

bool _dbPowerState::operator==(const _dbPowerState& rhs) const
{
  if (_name != rhs._name) {
    return false;
  }
  if (_next_entry != rhs._next_entry) {
    return false;
  }

  return true;
}

bool _dbPowerState::operator<(const _dbPowerState& rhs) const
{
  return true;
}

_dbPowerState::_dbPowerState(_dbDatabase* db)
{
  _name = nullptr;
}

dbIStream& operator>>(dbIStream& stream, dbPowerState::Supply& obj)
{
  stream >> obj._mode;
  stream >> obj._voltages;
  return stream;
}
dbIStream& operator>>(dbIStream& stream, dbPowerState::SupplyExpr& obj)
{
  stream >> obj._power;
  stream >> obj._ground;
  stream >> obj._nwell;
  stream >> obj._pwell;
  return stream;
}
dbIStream& operator>>(dbIStream& stream, dbPowerState::PowerStateEntry& obj)
{
  stream >> obj._name;
  stream >> obj._supply_expr;
  stream >> obj._simstate;
  return stream;
}
dbIStream& operator>>(dbIStream& stream, _dbPowerState& obj)
{
  stream >> obj._name;
  stream >> obj._states;
  stream >> obj._next_entry;
  return stream;
}

dbOStream& operator<<(dbOStream& stream, const dbPowerState::Supply& obj)
{
  stream << obj._mode;
  stream << obj._voltages;
  return stream;
}
dbOStream& operator<<(dbOStream& stream, const dbPowerState::SupplyExpr& obj)
{
  stream << obj._power;
  stream << obj._ground;
  stream << obj._nwell;
  stream << obj._pwell;
  return stream;
}
dbOStream& operator<<(dbOStream& stream,
                      const dbPowerState::PowerStateEntry& obj)
{
  stream << obj._name;
  stream << obj._supply_expr;
  stream << obj._simstate;
  return stream;
}
dbOStream& operator<<(dbOStream& stream, const _dbPowerState& obj)
{
  stream << obj._name;
  stream << obj._states;
  stream << obj._next_entry;
  return stream;
}

void _dbPowerState::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);
}

_dbPowerState::~_dbPowerState()
{
  if (_name) {
    free((void*) _name);
  }
}

////////////////////////////////////////////////////////////////////
//
// dbPowerState - Methods
//
////////////////////////////////////////////////////////////////////

const char* dbPowerState::getName() const
{
  _dbPowerState* obj = (_dbPowerState*) this;
  return obj->_name;
}

void dbPowerState::getStates(
    std::vector<dbPowerState::PowerStateEntry>& tbl) const
{
  _dbPowerState* obj = (_dbPowerState*) this;
  tbl = obj->_states;
}

// User Code Begin dbPowerStatePublicMethods

dbPowerState* dbPowerState::create(dbBlock* block, const char* name)
{
  _dbBlock* _block = (_dbBlock*) block;
  if (_block->_powerstate_hash.hasMember(name)) {
    return nullptr;
  }
  _dbPowerState* obj = _block->_powerstate_tbl->create();
  obj->_name = strdup(name);
  ZALLOCATED(obj->_name);

  _block->_powerstate_hash.insert(obj);
  return (dbPowerState*) obj;
}

bool dbPowerState::addState(const std::string& state,
                            const std::string& supply,
                            const std::string& mode,
                            const std::vector<float> voltages,
                            const std::string& simstate)
{
  _dbPowerState* obj = (_dbPowerState*) this;

  auto setSupply = [&](odb::dbPowerState::SupplyExpr& expr) {
    if (supply == "power") {
      expr._power._mode = mode;
      expr._power._voltages = voltages;
    } else if (supply == "ground") {
      expr._ground._mode = mode;
      expr._ground._voltages = voltages;
    } else if (supply == "nwell") {
      expr._nwell._mode = mode;
      expr._nwell._voltages = voltages;
    } else if (supply == "pwell") {
      expr._pwell._mode = mode;
      expr._pwell._voltages = voltages;
    } else {
      return false;  // Unknown supply
    }
    return true;
  };

  // Try updating existing state
  for (auto& entry : obj->_states) {
    if (entry._name == state) {
      return setSupply(entry._supply_expr);
    }
  }

  // Create new state
  PowerStateEntry new_entry;
  new_entry._name = state;
  new_entry._simstate = simstate;
  if (!setSupply(new_entry._supply_expr)) {
    return false;
  }

  obj->_states.push_back(new_entry);
  return true;
}

// User Code End dbPowerStatePublicMethods
}  // namespace odb
   // Generator Code End Cpp