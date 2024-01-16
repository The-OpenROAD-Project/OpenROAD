///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2022, The Regents of the University of California
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

// Generator Code Begin Cpp
#include "dbPowerSwitch.h"

#include "db.h"
#include "dbBlock.h"
#include "dbDatabase.h"
#include "dbDiff.hpp"
#include "dbHashTable.hpp"
#include "dbMTerm.h"
#include "dbMaster.h"
#include "dbNet.h"
#include "dbPowerDomain.h"
#include "dbTable.h"
#include "dbTable.hpp"
#include "utl/Logger.h"
namespace odb {
template class dbTable<_dbPowerSwitch>;

bool _dbPowerSwitch::operator==(const _dbPowerSwitch& rhs) const
{
  if (_name != rhs._name) {
    return false;
  }
  if (_next_entry != rhs._next_entry) {
    return false;
  }
  if (_control_net != rhs._control_net) {
    return false;
  }
  if (_lib_cell != rhs._lib_cell) {
    return false;
  }
  if (_lib != rhs._lib) {
    return false;
  }
  if (_power_domain != rhs._power_domain) {
    return false;
  }

  return true;
}

bool _dbPowerSwitch::operator<(const _dbPowerSwitch& rhs) const
{
  return true;
}

void _dbPowerSwitch::differences(dbDiff& diff,
                                 const char* field,
                                 const _dbPowerSwitch& rhs) const
{
  DIFF_BEGIN
  DIFF_FIELD(_name);
  DIFF_FIELD(_next_entry);
  DIFF_FIELD(_control_net);
  DIFF_FIELD(_lib_cell);
  DIFF_FIELD(_lib);
  DIFF_FIELD(_power_domain);
  DIFF_END
}

void _dbPowerSwitch::out(dbDiff& diff, char side, const char* field) const
{
  DIFF_OUT_BEGIN
  DIFF_OUT_FIELD(_name);
  DIFF_OUT_FIELD(_next_entry);
  DIFF_OUT_FIELD(_control_net);
  DIFF_OUT_FIELD(_lib_cell);
  DIFF_OUT_FIELD(_lib);
  DIFF_OUT_FIELD(_power_domain);

  DIFF_END
}

_dbPowerSwitch::_dbPowerSwitch(_dbDatabase* db)
{
}

_dbPowerSwitch::_dbPowerSwitch(_dbDatabase* db, const _dbPowerSwitch& r)
{
  _name = r._name;
  _next_entry = r._next_entry;
  _control_net = r._control_net;
  _lib_cell = r._lib_cell;
  _lib = r._lib;
  _power_domain = r._power_domain;
}

dbIStream& operator>>(dbIStream& stream, _dbPowerSwitch& obj)
{
  stream >> obj._name;
  stream >> obj._next_entry;
  stream >> obj._in_supply_port;
  stream >> obj._out_supply_port;
  stream >> obj._control_port;
  stream >> obj._on_state;
  stream >> obj._control_net;
  stream >> obj._power_domain;
  // User Code Begin >>
  if (obj.getDatabase()->isSchema(db_schema_upf_power_switch_mapping)) {
    stream >> obj._lib_cell;
    stream >> obj._lib;
    stream >> obj._port_map;
  }
  // User Code End >>
  return stream;
}

dbOStream& operator<<(dbOStream& stream, const _dbPowerSwitch& obj)
{
  stream << obj._name;
  stream << obj._next_entry;
  stream << obj._in_supply_port;
  stream << obj._out_supply_port;
  stream << obj._control_port;
  stream << obj._on_state;
  stream << obj._control_net;
  stream << obj._power_domain;
  // User Code Begin <<
  stream << obj._lib_cell;
  stream << obj._lib;
  stream << obj._port_map;
  // User Code End <<
  return stream;
}

_dbPowerSwitch::~_dbPowerSwitch()
{
  if (_name) {
    free((void*) _name);
  }
}

////////////////////////////////////////////////////////////////////
//
// dbPowerSwitch - Methods
//
////////////////////////////////////////////////////////////////////

const char* dbPowerSwitch::getName() const
{
  _dbPowerSwitch* obj = (_dbPowerSwitch*) this;
  return obj->_name;
}

void dbPowerSwitch::setControlNet(dbNet* control_net)
{
  _dbPowerSwitch* obj = (_dbPowerSwitch*) this;

  obj->_control_net = control_net->getImpl()->getOID();
}

dbNet* dbPowerSwitch::getControlNet() const
{
  _dbPowerSwitch* obj = (_dbPowerSwitch*) this;
  if (obj->_control_net == 0) {
    return nullptr;
  }
  _dbBlock* par = (_dbBlock*) obj->getOwner();
  return (dbNet*) par->_net_tbl->getPtr(obj->_control_net);
}

void dbPowerSwitch::setPowerDomain(dbPowerDomain* power_domain)
{
  _dbPowerSwitch* obj = (_dbPowerSwitch*) this;

  obj->_power_domain = power_domain->getImpl()->getOID();
}

dbPowerDomain* dbPowerSwitch::getPowerDomain() const
{
  _dbPowerSwitch* obj = (_dbPowerSwitch*) this;
  if (obj->_power_domain == 0) {
    return nullptr;
  }
  _dbBlock* par = (_dbBlock*) obj->getOwner();
  return (dbPowerDomain*) par->_powerdomain_tbl->getPtr(obj->_power_domain);
}

// User Code Begin dbPowerSwitchPublicMethods
dbPowerSwitch* dbPowerSwitch::create(dbBlock* block, const char* name)
{
  _dbBlock* _block = (_dbBlock*) block;
  if (_block->_powerswitch_hash.hasMember(name))
    return nullptr;
  _dbPowerSwitch* ps = _block->_powerswitch_tbl->create();
  ps->_name = strdup(name);
  ZALLOCATED(ps->_name);

  _block->_powerswitch_hash.insert(ps);
  return (dbPowerSwitch*) ps;
}

void dbPowerSwitch::destroy(dbPowerSwitch* ps)
{
  // TODO
}

void dbPowerSwitch::addInSupplyPort(const std::string& in_port)
{
  _dbPowerSwitch* obj = (_dbPowerSwitch*) this;
  obj->_in_supply_port.push_back(in_port);
}

void dbPowerSwitch::addOutSupplyPort(const std::string& out_port)
{
  _dbPowerSwitch* obj = (_dbPowerSwitch*) this;
  obj->_out_supply_port.push_back(out_port);
}

void dbPowerSwitch::addControlPort(const std::string& control_port)
{
  _dbPowerSwitch* obj = (_dbPowerSwitch*) this;
  obj->_control_port.push_back(control_port);
}

void dbPowerSwitch::addOnState(const std::string& on_state)
{
  _dbPowerSwitch* obj = (_dbPowerSwitch*) this;
  obj->_on_state.push_back(on_state);
}

void dbPowerSwitch::setLibCell(dbMaster* master)
{
  _dbPowerSwitch* obj = (_dbPowerSwitch*) this;
  obj->_lib_cell = master->getImpl()->getOID();
  obj->_lib = master->getLib()->getImpl()->getOID();
}

void dbPowerSwitch::addPortMap(const std::string& model_port,
                               const std::string& switch_port)
{
  _dbPowerSwitch* obj = (_dbPowerSwitch*) this;
  dbMaster* master = getLibCell();

  auto logger = obj->getImpl()->getLogger();
  if (!master) {
    logger->error(utl::ODB,
                  153,
                  "Cannot map port {} to {} because no lib cell is added",
                  model_port,
                  switch_port);
    return;
  }

  dbMTerm* mterm = master->findMTerm(switch_port.c_str());
  if (mterm == nullptr) {
    logger->error(utl::ODB,
                  154,
                  "Cannot map port {} to {} because the mterm is not found",
                  model_port,
                  switch_port);
    return;
  }
  obj->_port_map[model_port] = mterm->getImpl()->getOID();
}

void dbPowerSwitch::addPortMap(const std::string& model_port, dbMTerm* mterm)
{
  _dbPowerSwitch* obj = (_dbPowerSwitch*) this;

  dbMaster* master = getLibCell();

  if (!master) {
    obj->getImpl()->getLogger()->error(
        utl::ODB,
        219,
        "Cannot map port {} to {} because no lib cell is added",
        model_port,
        mterm->getName());
    return;
  }

  if (master != mterm->getMaster()) {
    obj->getImpl()->getLogger()->error(
        utl::ODB,
        220,
        "Cannot map port {} to {} because the mterm is not in the same master",
        model_port,
        mterm->getName());
    return;
  }

  obj->_port_map[model_port] = mterm->getImpl()->getOID();
}

std::vector<std::string> dbPowerSwitch::getControlPorts()
{
  _dbPowerSwitch* obj = (_dbPowerSwitch*) this;
  return obj->_control_port;
}

std::vector<std::string> dbPowerSwitch::getInputSupplyPorts()
{
  _dbPowerSwitch* obj = (_dbPowerSwitch*) this;
  return obj->_in_supply_port;
}

std::vector<std::string> dbPowerSwitch::getOutputSupplyPorts()
{
  _dbPowerSwitch* obj = (_dbPowerSwitch*) this;
  return obj->_out_supply_port;
}

std::vector<std::string> dbPowerSwitch::getOnStates()
{
  _dbPowerSwitch* obj = (_dbPowerSwitch*) this;
  return obj->_on_state;
}

dbMaster* dbPowerSwitch::getLibCell()
{
  _dbPowerSwitch* obj = (_dbPowerSwitch*) this;
  return (dbMaster*) dbMaster::getMaster(
      dbLib::getLib((dbDatabase*) obj->getImpl()->getDatabase(), obj->_lib),
      obj->_lib_cell);
}

std::map<std::string, dbMTerm*> dbPowerSwitch::getPortMap()
{
  _dbPowerSwitch* obj = (_dbPowerSwitch*) this;
  std::map<std::string, dbMTerm*> port_mapping;

  dbMaster* cell = getLibCell();
  if (!cell) {
    return port_mapping;
  }

  for (auto const& [key, val] : obj->_port_map) {
    dbMTerm* mterm = (dbMTerm*) dbMTerm::getMTerm(cell, val);
    port_mapping[key] = mterm;
  }

  return port_mapping;
}
// User Code End dbPowerSwitchPublicMethods
}  // namespace odb
   // Generator Code End Cpp