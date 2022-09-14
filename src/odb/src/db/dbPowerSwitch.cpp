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
#include "dbNet.h"
#include "dbPowerDomain.h"
#include "dbTable.h"
#include "dbTable.hpp"
// User Code Begin Includes
// User Code End Includes
namespace odb {

template class dbTable<_dbPowerSwitch>;

bool _dbPowerSwitch::operator==(const _dbPowerSwitch& rhs) const
{
  if (_name != rhs._name)
    return false;

  if (_next_entry != rhs._next_entry)
    return false;

  if (_in_supply_port != rhs._in_supply_port)
    return false;

  if (_out_supply_port != rhs._out_supply_port)
    return false;

  if (_control_net != rhs._control_net)
    return false;

  if (_power_domain != rhs._power_domain)
    return false;

  // User Code Begin ==
  // User Code End ==
  return true;
}
bool _dbPowerSwitch::operator<(const _dbPowerSwitch& rhs) const
{
  // User Code Begin <
  // User Code End <
  return true;
}
void _dbPowerSwitch::differences(dbDiff& diff,
                                 const char* field,
                                 const _dbPowerSwitch& rhs) const
{
  DIFF_BEGIN

  DIFF_FIELD(_name);
  DIFF_FIELD(_next_entry);
  DIFF_FIELD(_in_supply_port);
  DIFF_FIELD(_out_supply_port);
  DIFF_FIELD(_control_net);
  DIFF_FIELD(_power_domain);
  // User Code Begin Differences
  // User Code End Differences
  DIFF_END
}
void _dbPowerSwitch::out(dbDiff& diff, char side, const char* field) const
{
  DIFF_OUT_BEGIN
  DIFF_OUT_FIELD(_name);
  DIFF_OUT_FIELD(_next_entry);
  DIFF_OUT_FIELD(_in_supply_port);
  DIFF_OUT_FIELD(_out_supply_port);
  DIFF_OUT_FIELD(_control_net);
  DIFF_OUT_FIELD(_power_domain);

  // User Code Begin Out
  // User Code End Out
  DIFF_END
}
_dbPowerSwitch::_dbPowerSwitch(_dbDatabase* db)
{
  // User Code Begin Constructor
  // User Code End Constructor
}
_dbPowerSwitch::_dbPowerSwitch(_dbDatabase* db, const _dbPowerSwitch& r)
{
  _name = r._name;
  _next_entry = r._next_entry;
  _in_supply_port = r._in_supply_port;
  _out_supply_port = r._out_supply_port;
  _control_net = r._control_net;
  _power_domain = r._power_domain;
  // User Code Begin CopyConstructor
  // User Code End CopyConstructor
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
  // User Code End <<
  return stream;
}

_dbPowerSwitch::~_dbPowerSwitch()
{
  if (_name)
    free((void*) _name);
  // User Code Begin Destructor
  // User Code End Destructor
}

// User Code Begin PrivateMethods
// User Code End PrivateMethods

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

std::string dbPowerSwitch::getInSupplyPort() const
{
  _dbPowerSwitch* obj = (_dbPowerSwitch*) this;
  return obj->_in_supply_port;
}

std::string dbPowerSwitch::getOutSupplyPort() const
{
  _dbPowerSwitch* obj = (_dbPowerSwitch*) this;
  return obj->_out_supply_port;
}

void dbPowerSwitch::setControlNet(dbNet* control_net)
{
  _dbPowerSwitch* obj = (_dbPowerSwitch*) this;

  obj->_control_net = control_net->getImpl()->getOID();
}

dbNet* dbPowerSwitch::getControlNet() const
{
  _dbPowerSwitch* obj = (_dbPowerSwitch*) this;
  if (obj->_control_net == 0)
    return NULL;
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
  if (obj->_power_domain == 0)
    return NULL;
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

void dbPowerSwitch::setInSupplyPort(const std::string& in_port)
{
  _dbPowerSwitch* obj = (_dbPowerSwitch*) this;
  obj->_in_supply_port = in_port;
}

void dbPowerSwitch::setOutSupplyPort(const std::string& out_port)
{
  _dbPowerSwitch* obj = (_dbPowerSwitch*) this;
  obj->_out_supply_port = out_port;
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

std::vector<std::string> dbPowerSwitch::getControlPorts()
{
  _dbPowerSwitch* obj = (_dbPowerSwitch*) this;
  return obj->_control_port;
}
std::vector<std::string> dbPowerSwitch::getOnStates()
{
  _dbPowerSwitch* obj = (_dbPowerSwitch*) this;
  return obj->_on_state;
}
// User Code End dbPowerSwitchPublicMethods
}  // namespace odb
   // Generator Code End Cpp