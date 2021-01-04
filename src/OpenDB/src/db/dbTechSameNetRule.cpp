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

#include "dbTechSameNetRule.h"

#include "db.h"
#include "dbDatabase.h"
#include "dbTable.h"
#include "dbTable.hpp"
#include "dbTech.h"
#include "dbTechLayer.h"
#include "dbTechNonDefaultRule.h"

namespace odb {

template class dbTable<_dbTechSameNetRule>;

bool _dbTechSameNetRule::operator==(const _dbTechSameNetRule& rhs) const
{
  if (_flags._stack != rhs._flags._stack)
    return false;

  if (_spacing != rhs._spacing)
    return false;

  if (_layer_1 != rhs._layer_1)
    return false;

  if (_layer_2 != rhs._layer_2)
    return false;

  return true;
}

void _dbTechSameNetRule::differences(dbDiff&                   diff,
                                     const char*               field,
                                     const _dbTechSameNetRule& rhs) const
{
  DIFF_BEGIN
  DIFF_FIELD(_flags._stack);
  DIFF_FIELD(_spacing);
  DIFF_FIELD(_layer_1);
  DIFF_FIELD(_layer_2);
  DIFF_END
}

void _dbTechSameNetRule::out(dbDiff& diff, char side, const char* field) const
{
  DIFF_OUT_BEGIN
  DIFF_OUT_FIELD(_flags._stack);
  DIFF_OUT_FIELD(_spacing);
  DIFF_OUT_FIELD(_layer_1);
  DIFF_OUT_FIELD(_layer_2);
  DIFF_END
}

////////////////////////////////////////////////////////////////////
//
// dbTechSameNetRule - Methods
//
////////////////////////////////////////////////////////////////////

dbTechLayer* dbTechSameNetRule::getLayer1()
{
  _dbTechSameNetRule* rule = (_dbTechSameNetRule*) this;
  _dbTech*            tech = (_dbTech*) rule->getOwner();
  return (dbTechLayer*) tech->_layer_tbl->getPtr(rule->_layer_1);
}

dbTechLayer* dbTechSameNetRule::getLayer2()
{
  _dbTechSameNetRule* rule = (_dbTechSameNetRule*) this;
  _dbTech*            tech = (_dbTech*) rule->getOwner();
  return (dbTechLayer*) tech->_layer_tbl->getPtr(rule->_layer_2);
}

int dbTechSameNetRule::getSpacing()
{
  _dbTechSameNetRule* rule = (_dbTechSameNetRule*) this;
  return rule->_spacing;
}

void dbTechSameNetRule::setSpacing(int spacing)
{
  _dbTechSameNetRule* rule = (_dbTechSameNetRule*) this;
  rule->_spacing           = spacing;
}

void dbTechSameNetRule::setAllowStackedVias(bool value)
{
  _dbTechSameNetRule* rule = (_dbTechSameNetRule*) this;

  if (value)
    rule->_flags._stack = 1;
  else
    rule->_flags._stack = 0;
}

bool dbTechSameNetRule::getAllowStackedVias()
{
  _dbTechSameNetRule* rule = (_dbTechSameNetRule*) this;
  return rule->_flags._stack == 1;
}

dbTechSameNetRule* dbTechSameNetRule::create(dbTechLayer* layer1_,
                                             dbTechLayer* layer2_)
{
  _dbTechLayer* layer1 = (_dbTechLayer*) layer1_;
  _dbTechLayer* layer2 = (_dbTechLayer*) layer2_;
  dbTech*       tech_  = (dbTech*) layer1->getOwner();
  _dbTech*      tech   = (_dbTech*) tech_;
  assert(tech_ == (dbTech*) layer2->getOwner());

  if (tech->_samenet_rules.size() == 0)
    tech->_samenet_matrix.resize(tech->_layer_cnt, tech->_layer_cnt);

  else if (tech_->findSameNetRule(layer1_, layer2_))
    return NULL;

  _dbTechSameNetRule* rule = tech->_samenet_rule_tbl->create();
  rule->_layer_1           = layer1->getOID();
  rule->_layer_2           = layer2->getOID();
  tech->_samenet_matrix(layer1->_number, layer2->_number) = rule->getOID();
  tech->_samenet_matrix(layer2->_number, layer1->_number) = rule->getOID();
  tech->_samenet_rules.push_back(rule->getOID());
  return (dbTechSameNetRule*) rule;
}

dbTechSameNetRule* dbTechSameNetRule::create(dbTechNonDefaultRule* ndrule_,
                                             dbTechLayer*          layer1_,
                                             dbTechLayer*          layer2_)
{
  _dbTechNonDefaultRule* ndrule = (_dbTechNonDefaultRule*) ndrule_;
  _dbTechLayer*          layer1 = (_dbTechLayer*) layer1_;
  _dbTechLayer*          layer2 = (_dbTechLayer*) layer2_;
  dbTech*                tech_  = (dbTech*) layer1->getOwner();
  _dbTech*               tech   = (_dbTech*) tech_;
  assert(tech_ == (dbTech*) layer2->getOwner());
  assert(tech_ == (dbTech*) ndrule->getOwner());

  if (ndrule->_samenet_rules.size() == 0)
    ndrule->_samenet_matrix.resize(tech->_layer_cnt, tech->_layer_cnt);

  else if (ndrule_->findSameNetRule(layer1_, layer2_))
    return NULL;

  _dbTechSameNetRule* rule = tech->_samenet_rule_tbl->create();
  rule->_layer_1           = layer1->getOID();
  rule->_layer_2           = layer2->getOID();
  ndrule->_samenet_matrix(layer1->_number, layer2->_number) = rule->getOID();
  ndrule->_samenet_matrix(layer2->_number, layer1->_number) = rule->getOID();
  ndrule->_samenet_rules.push_back(rule->getOID());
  return (dbTechSameNetRule*) rule;
}

dbTechSameNetRule* dbTechSameNetRule::getTechSameNetRule(dbTech* tech_,
                                                         uint    dbid_)
{
  _dbTech* tech = (_dbTech*) tech_;
  return (dbTechSameNetRule*) tech->_samenet_rule_tbl->getPtr(dbid_);
}

}  // namespace odb
