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

#include "dbTechViaGenerateRule.h"

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

template class dbTable<_dbTechViaGenerateRule>;

////////////////////////////////////////////////////////////////////
//
// _dbTechViaGenerateRule - Methods
//
////////////////////////////////////////////////////////////////////

bool _dbTechViaGenerateRule::operator==(const _dbTechViaGenerateRule& rhs) const
{
  if (_flags._default != rhs._flags._default)
    return false;

  if (_name && rhs._name) {
    if (strcmp(_name, rhs._name) != 0)
      return false;
  } else if (_name || rhs._name)
    return false;

  if (_layer_rules != rhs._layer_rules)
    return false;

  return true;
}

void _dbTechViaGenerateRule::differences(
    dbDiff&                       diff,
    const char*                   field,
    const _dbTechViaGenerateRule& rhs) const
{
  DIFF_BEGIN
  DIFF_FIELD(_name);
  DIFF_FIELD(_flags._default);
  DIFF_VECTOR(_layer_rules);
  DIFF_END
}

void _dbTechViaGenerateRule::out(dbDiff&     diff,
                                 char        side,
                                 const char* field) const
{
  DIFF_OUT_BEGIN
  DIFF_OUT_FIELD(_name);
  DIFF_OUT_FIELD(_flags._default);
  DIFF_OUT_VECTOR(_layer_rules);
  DIFF_END
}

_dbTechViaGenerateRule::_dbTechViaGenerateRule(_dbDatabase*,
                                               const _dbTechViaGenerateRule& v)
    : _flags(v._flags), _name(NULL), _layer_rules(v._layer_rules)
{
  if (v._name) {
    _name = strdup(v._name);
    ZALLOCATED(_name);
  }
}

_dbTechViaGenerateRule::_dbTechViaGenerateRule(_dbDatabase*)
{
  _name              = 0;
  _flags._default    = 0;
  _flags._spare_bits = 0;
}

_dbTechViaGenerateRule::~_dbTechViaGenerateRule()
{
  if (_name)
    free((void*) _name);
}

dbOStream& operator<<(dbOStream& stream, const _dbTechViaGenerateRule& v)
{
  uint* bit_field = (uint*) &v._flags;
  stream << *bit_field;
  stream << v._name;
  stream << v._layer_rules;
  return stream;
}

dbIStream& operator>>(dbIStream& stream, _dbTechViaGenerateRule& v)
{
  uint* bit_field = (uint*) &v._flags;
  stream >> *bit_field;
  stream >> v._name;
  stream >> v._layer_rules;
  return stream;
}

////////////////////////////////////////////////////////////////////
//
// dbTechViaGenerateRule - Methods
//
////////////////////////////////////////////////////////////////////

std::string dbTechViaGenerateRule::getName()
{
  _dbTechViaGenerateRule* via = (_dbTechViaGenerateRule*) this;
  return via->_name;
}

bool dbTechViaGenerateRule::isDefault()
{
  _dbTechViaGenerateRule* rule = (_dbTechViaGenerateRule*) this;
  return rule->_flags._default == 1;
}

uint dbTechViaGenerateRule::getViaLayerRuleCount()
{
  _dbTechViaGenerateRule* rule = (_dbTechViaGenerateRule*) this;
  return rule->_layer_rules.size();
}

dbTechViaLayerRule* dbTechViaGenerateRule::getViaLayerRule(uint idx)
{
  _dbTechViaGenerateRule* rule = (_dbTechViaGenerateRule*) this;
  dbTech*                 tech = (dbTech*) rule->getOwner();

  if (idx >= rule->_layer_rules.size())
    return NULL;

  dbId<dbTechViaLayerRule> id = rule->_layer_rules[idx];
  return dbTechViaLayerRule::getTechViaLayerRule(tech, id);
}

dbTechViaGenerateRule* dbTechViaGenerateRule::create(dbTech*     tech_,
                                                     const char* name,
                                                     bool        is_default)
{
  if (tech_->findViaGenerateRule(name))
    return NULL;

  _dbTech*                tech = (_dbTech*) tech_;
  _dbTechViaGenerateRule* rule = tech->_via_generate_rule_tbl->create();
  rule->_name                  = strdup(name);
  ZALLOCATED(rule->_name);
  rule->_flags._default = is_default;
  return (dbTechViaGenerateRule*) rule;
}

dbTechViaGenerateRule* dbTechViaGenerateRule::getTechViaGenerateRule(
    dbTech* tech_,
    uint    dbid_)
{
  _dbTech* tech = (_dbTech*) tech_;
  return (dbTechViaGenerateRule*) tech->_via_generate_rule_tbl->getPtr(dbid_);
}

}  // namespace odb
