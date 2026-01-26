// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbPowerSwitch.h"

#include <cstdlib>
#include <map>
#include <string>

#include "dbBlock.h"
#include "dbCore.h"
#include "dbDatabase.h"
#include "dbHashTable.hpp"
#include "dbMTerm.h"
#include "dbMaster.h"
#include "dbNet.h"
#include "dbPowerDomain.h"
#include "dbTable.h"
#include "odb/db.h"
#include "utl/Logger.h"
// User Code Begin Includes
#include "dbCommon.h"
// User Code End Includes
namespace odb {
template class dbTable<_dbPowerSwitch>;

bool _dbPowerSwitch::operator==(const _dbPowerSwitch& rhs) const
{
  if (name_ != rhs.name_) {
    return false;
  }
  if (next_entry_ != rhs.next_entry_) {
    return false;
  }
  if (lib_cell_ != rhs.lib_cell_) {
    return false;
  }
  if (lib_ != rhs.lib_) {
    return false;
  }
  if (power_domain_ != rhs.power_domain_) {
    return false;
  }

  return true;
}

bool _dbPowerSwitch::operator<(const _dbPowerSwitch& rhs) const
{
  return true;
}

_dbPowerSwitch::_dbPowerSwitch(_dbDatabase* db)
{
  name_ = nullptr;
}

static dbIStream& operator>>(dbIStream& stream,
                             dbPowerSwitch::UPFIOSupplyPort& obj)
{
  stream >> obj.port_name;
  stream >> obj.supply_net_name;
  return stream;
}
static dbIStream& operator>>(dbIStream& stream,
                             dbPowerSwitch::UPFControlPort& obj)
{
  stream >> obj.port_name;
  stream >> obj.net_name;
  return stream;
}
static dbIStream& operator>>(dbIStream& stream,
                             dbPowerSwitch::UPFAcknowledgePort& obj)
{
  stream >> obj.port_name;
  stream >> obj.net_name;
  stream >> obj.boolean_expression;
  return stream;
}
static dbIStream& operator>>(dbIStream& stream, dbPowerSwitch::UPFOnState& obj)
{
  stream >> obj.state_name;
  stream >> obj.input_supply_port;
  stream >> obj.boolean_expression;
  return stream;
}
dbIStream& operator>>(dbIStream& stream, _dbPowerSwitch& obj)
{
  stream >> obj.name_;
  stream >> obj.next_entry_;
  // User Code Begin >>
  if (obj.getDatabase()->isSchema(kSchemaUpdateDbPowerSwitch)) {
    stream >> obj.in_supply_port_;
    stream >> obj.out_supply_port_;
    stream >> obj.control_port_;
    stream >> obj.acknowledge_port_;
    stream >> obj.on_state_;
  } else {
    dbVector<std::string> in_supply_port;
    stream >> in_supply_port;
    for (const auto& port : in_supply_port) {
      obj.in_supply_port_.emplace_back(dbPowerSwitch::UPFIOSupplyPort{
          .port_name = port, .supply_net_name = ""});
    }
    dbVector<std::string> out_supply_port;
    stream >> out_supply_port;
    if (!out_supply_port.empty()) {
      obj.out_supply_port_.port_name = out_supply_port[0];
      obj.out_supply_port_.supply_net_name = "";
    }
    dbVector<std::string> control_port;
    stream >> control_port;
    for (const auto& port : control_port) {
      obj.control_port_.emplace_back(
          dbPowerSwitch::UPFControlPort{.port_name = port, .net_name = ""});
    }
    dbVector<std::string> on_state;
    stream >> on_state;
    for (const auto& state : on_state) {
      obj.on_state_.emplace_back(
          dbPowerSwitch::UPFOnState{.state_name = state,
                                    .input_supply_port = "",
                                    .boolean_expression = ""});
    }
    dbId<_dbNet> net;
    stream >> net;  // unused
  }
  stream >> obj.power_domain_;
  if (obj.getDatabase()->isSchema(kSchemaUpfPowerSwitchMapping)) {
    stream >> obj.lib_cell_;
    stream >> obj.lib_;
    stream >> obj.port_map_;
  }
  // User Code End >>
  return stream;
}

static dbOStream& operator<<(dbOStream& stream,
                             const dbPowerSwitch::UPFIOSupplyPort& obj)
{
  stream << obj.port_name;
  stream << obj.supply_net_name;
  return stream;
}
static dbOStream& operator<<(dbOStream& stream,
                             const dbPowerSwitch::UPFControlPort& obj)
{
  stream << obj.port_name;
  stream << obj.net_name;
  return stream;
}
static dbOStream& operator<<(dbOStream& stream,
                             const dbPowerSwitch::UPFAcknowledgePort& obj)
{
  stream << obj.port_name;
  stream << obj.net_name;
  stream << obj.boolean_expression;
  return stream;
}
static dbOStream& operator<<(dbOStream& stream,
                             const dbPowerSwitch::UPFOnState& obj)
{
  stream << obj.state_name;
  stream << obj.input_supply_port;
  stream << obj.boolean_expression;
  return stream;
}
dbOStream& operator<<(dbOStream& stream, const _dbPowerSwitch& obj)
{
  stream << obj.name_;
  stream << obj.next_entry_;
  // User Code Begin <<
  stream << obj.in_supply_port_;
  stream << obj.out_supply_port_;
  stream << obj.control_port_;
  stream << obj.acknowledge_port_;
  stream << obj.on_state_;
  stream << obj.power_domain_;
  stream << obj.lib_cell_;
  stream << obj.lib_;
  stream << obj.port_map_;
  // User Code End <<
  return stream;
}

void _dbPowerSwitch::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);

  // User Code Begin collectMemInfo
  info.children["name"].add(name_);
  info.children["in_supply_port"].add(in_supply_port_);
  info.children["control_port"].add(control_port_);
  info.children["acknowledge_port"].add(acknowledge_port_);
  info.children["on_state"].add(on_state_);
  info.children["port_map"].add(port_map_);
  // User Code End collectMemInfo
}

_dbPowerSwitch::~_dbPowerSwitch()
{
  if (name_) {
    free((void*) name_);
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
  return obj->name_;
}

void dbPowerSwitch::setPowerDomain(dbPowerDomain* power_domain)
{
  _dbPowerSwitch* obj = (_dbPowerSwitch*) this;

  obj->power_domain_ = power_domain->getImpl()->getOID();
}

dbPowerDomain* dbPowerSwitch::getPowerDomain() const
{
  _dbPowerSwitch* obj = (_dbPowerSwitch*) this;
  if (obj->power_domain_ == 0) {
    return nullptr;
  }
  _dbBlock* par = (_dbBlock*) obj->getOwner();
  return (dbPowerDomain*) par->powerdomain_tbl_->getPtr(obj->power_domain_);
}

// User Code Begin dbPowerSwitchPublicMethods
dbPowerSwitch* dbPowerSwitch::create(dbBlock* block, const char* name)
{
  _dbBlock* _block = (_dbBlock*) block;
  if (_block->powerswitch_hash_.hasMember(name)) {
    return nullptr;
  }
  _dbPowerSwitch* ps = _block->powerswitch_tbl_->create();
  ps->name_ = safe_strdup(name);

  _block->powerswitch_hash_.insert(ps);
  return (dbPowerSwitch*) ps;
}

void dbPowerSwitch::destroy(dbPowerSwitch* ps)
{
  // TODO
}

void dbPowerSwitch::addInSupplyPort(const std::string& in_port,
                                    const std::string& net)
{
  _dbPowerSwitch* obj = (_dbPowerSwitch*) this;
  obj->in_supply_port_.emplace_back(
      UPFIOSupplyPort{.port_name = in_port, .supply_net_name = net});
}

void dbPowerSwitch::setOutSupplyPort(const std::string& out_port,
                                     const std::string& net)
{
  _dbPowerSwitch* obj = (_dbPowerSwitch*) this;
  obj->out_supply_port_.port_name = out_port;
  obj->out_supply_port_.supply_net_name = net;
}

void dbPowerSwitch::addControlPort(const std::string& control_port,
                                   const std::string& control_net)
{
  _dbPowerSwitch* obj = (_dbPowerSwitch*) this;
  obj->control_port_.emplace_back(
      UPFControlPort{.port_name = control_port, .net_name = control_net});
}

void dbPowerSwitch::addAcknowledgePort(const std::string& port_name,
                                       const std::string& net_name,
                                       const std::string& boolean_expression)
{
  _dbPowerSwitch* obj = (_dbPowerSwitch*) this;
  obj->acknowledge_port_.emplace_back(
      UPFAcknowledgePort{.port_name = port_name,
                         .net_name = net_name,
                         .boolean_expression = boolean_expression});
}

void dbPowerSwitch::addOnState(const std::string& on_state,
                               const std::string& port_name,
                               const std::string& boolean_expression)
{
  _dbPowerSwitch* obj = (_dbPowerSwitch*) this;
  obj->on_state_.emplace_back(
      UPFOnState{.state_name = on_state,
                 .input_supply_port = port_name,
                 .boolean_expression = boolean_expression});
}

void dbPowerSwitch::setLibCell(dbMaster* master)
{
  _dbPowerSwitch* obj = (_dbPowerSwitch*) this;
  obj->lib_cell_ = master->getImpl()->getOID();
  obj->lib_ = master->getLib()->getImpl()->getOID();
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
  obj->port_map_[model_port] = mterm->getImpl()->getOID();
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

  obj->port_map_[model_port] = mterm->getImpl()->getOID();
}

std::vector<dbPowerSwitch::UPFControlPort> dbPowerSwitch::getControlPorts()
{
  _dbPowerSwitch* obj = (_dbPowerSwitch*) this;
  return obj->control_port_;
}

std::vector<dbPowerSwitch::UPFIOSupplyPort> dbPowerSwitch::getInputSupplyPorts()
{
  _dbPowerSwitch* obj = (_dbPowerSwitch*) this;
  return obj->in_supply_port_;
}

dbPowerSwitch::UPFIOSupplyPort dbPowerSwitch::getOutputSupplyPort()
{
  _dbPowerSwitch* obj = (_dbPowerSwitch*) this;
  return UPFIOSupplyPort{obj->out_supply_port_};
}

std::vector<dbPowerSwitch::UPFAcknowledgePort>
dbPowerSwitch::getAcknowledgePorts()
{
  _dbPowerSwitch* obj = (_dbPowerSwitch*) this;
  return obj->acknowledge_port_;
}

std::vector<dbPowerSwitch::UPFOnState> dbPowerSwitch::getOnStates()
{
  _dbPowerSwitch* obj = (_dbPowerSwitch*) this;
  return obj->on_state_;
}

dbMaster* dbPowerSwitch::getLibCell()
{
  _dbPowerSwitch* obj = (_dbPowerSwitch*) this;
  return dbMaster::getMaster(
      dbLib::getLib((dbDatabase*) obj->getImpl()->getDatabase(), obj->lib_),
      obj->lib_cell_);
}

std::map<std::string, dbMTerm*> dbPowerSwitch::getPortMap()
{
  _dbPowerSwitch* obj = (_dbPowerSwitch*) this;
  std::map<std::string, dbMTerm*> port_mapping;

  dbMaster* cell = getLibCell();
  if (!cell) {
    return port_mapping;
  }

  for (auto const& [key, val] : obj->port_map_) {
    dbMTerm* mterm = dbMTerm::getMTerm(cell, val);
    port_mapping[key] = mterm;
  }

  return port_mapping;
}
// User Code End dbPowerSwitchPublicMethods
}  // namespace odb
   // Generator Code End Cpp
