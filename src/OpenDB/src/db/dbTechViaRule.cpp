///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2019, Nefelus Inc
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

#include "dbTechViaRule.h"

#include "db.h"
#include "dbDatabase.h"
#include "dbDiff.hpp"
#include "dbSet.h"
#include "dbTable.h"
#include "dbTable.hpp"
#include "dbTech.h"
#include "dbTechLayer.h"
#include "dbTechVia.h"

namespace odb {

template class dbTable<_dbTechViaRule>;

////////////////////////////////////////////////////////////////////
//
// _dbTechViaRule - Methods
//
////////////////////////////////////////////////////////////////////

bool _dbTechViaRule::operator==(const _dbTechViaRule& rhs) const
{
  if (_name && rhs._name) {
    if (strcmp(_name, rhs._name) != 0)
      return false;
  } else if (_name || rhs._name)
    return false;

  if (_layer_rules != rhs._layer_rules)
    return false;

  if (_vias != rhs._vias)
    return false;

  return true;
}

void _dbTechViaRule::differences(dbDiff&               diff,
                                 const char*           field,
                                 const _dbTechViaRule& rhs) const
{
  DIFF_BEGIN
  DIFF_FIELD(_name);
  DIFF_VECTOR(_layer_rules);
  DIFF_VECTOR(_vias);
  DIFF_END
}

void _dbTechViaRule::out(dbDiff& diff, char side, const char* field) const
{
  DIFF_OUT_BEGIN
  DIFF_OUT_FIELD(_name);
  DIFF_OUT_VECTOR(_layer_rules);
  DIFF_OUT_VECTOR(_vias);
  DIFF_END
}

_dbTechViaRule::_dbTechViaRule(_dbDatabase*, const _dbTechViaRule& v)
    : _flags(v._flags),
      _name(NULL),
      _layer_rules(v._layer_rules),
      _vias(v._vias)
{
  if (v._name) {
    _name = strdup(v._name);
    ZALLOCATED(_name);
  }
}

_dbTechViaRule::_dbTechViaRule(_dbDatabase*)
{
  _name              = 0;
  _flags._spare_bits = 0;
}

_dbTechViaRule::~_dbTechViaRule()
{
  if (_name)
    free((void*) _name);
}

dbOStream& operator<<(dbOStream& stream, const _dbTechViaRule& v)
{
  uint* bit_field = (uint*) &v._flags;
  stream << *bit_field;
  stream << v._name;
  stream << v._layer_rules;
  stream << v._vias;
  return stream;
}

dbIStream& operator>>(dbIStream& stream, _dbTechViaRule& v)
{
  uint* bit_field = (uint*) &v._flags;
  stream >> *bit_field;
  stream >> v._name;
  stream >> v._layer_rules;
  stream >> v._vias;
  return stream;
}

////////////////////////////////////////////////////////////////////
//
// dbTechViaRule - Methods
//
////////////////////////////////////////////////////////////////////

std::string dbTechViaRule::getName()
{
  _dbTechViaRule* via = (_dbTechViaRule*) this;
  return via->_name;
}

void dbTechViaRule::addVia(dbTechVia* via)
{
  _dbTechViaRule* rule = (_dbTechViaRule*) this;
  rule->_vias.push_back(via->getImpl()->getOID());
}

uint dbTechViaRule::getViaCount()
{
  _dbTechViaRule* rule = (_dbTechViaRule*) this;
  return rule->_vias.size();
}

dbTechVia* dbTechViaRule::getVia(uint idx)
{
  _dbTechViaRule* rule = (_dbTechViaRule*) this;
  dbTech*         tech = (dbTech*) rule->getOwner();

  if (idx >= rule->_vias.size())
    return NULL;

  dbId<dbTechVia> id = rule->_vias[idx];
  return dbTechVia::getTechVia(tech, id);
}

uint dbTechViaRule::getViaLayerRuleCount()
{
  _dbTechViaRule* rule = (_dbTechViaRule*) this;
  return rule->_layer_rules.size();
}

dbTechViaLayerRule* dbTechViaRule::getViaLayerRule(uint idx)
{
  _dbTechViaRule* rule = (_dbTechViaRule*) this;
  dbTech*         tech = (dbTech*) rule->getOwner();

  if (idx >= rule->_layer_rules.size())
    return NULL;

  dbId<dbTechViaLayerRule> id = rule->_layer_rules[idx];
  return dbTechViaLayerRule::getTechViaLayerRule(tech, id);
}

dbTechViaRule* dbTechViaRule::create(dbTech* tech_, const char* name)
{
  if (tech_->findViaRule(name))
    return NULL;

  _dbTech*        tech = (_dbTech*) tech_;
  _dbTechViaRule* rule = tech->_via_rule_tbl->create();
  rule->_name          = strdup(name);
  ZALLOCATED(rule->_name);
  return (dbTechViaRule*) rule;
}

dbTechViaRule* dbTechViaRule::getTechViaRule(dbTech* tech_, uint dbid_)
{
  _dbTech* tech = (_dbTech*) tech_;
  return (dbTechViaRule*) tech->_via_rule_tbl->getPtr(dbid_);
}

}  // namespace odb
