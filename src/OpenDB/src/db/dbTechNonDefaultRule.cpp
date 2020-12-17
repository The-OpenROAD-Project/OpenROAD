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

#include "dbTechNonDefaultRule.h"

#include "db.h"
#include "dbBlock.h"
#include "dbDatabase.h"
#include "dbTable.h"
#include "dbTable.hpp"
#include "dbTech.h"
#include "dbTechLayer.h"
#include "dbTechLayerRule.h"
#include "dbTechSameNetRule.h"
#include "dbTechVia.h"

namespace odb {

template class dbTable<_dbTechNonDefaultRule>;

_dbTechNonDefaultRule::_dbTechNonDefaultRule(_dbDatabase*,
                                             const _dbTechNonDefaultRule& r)
    : _flags(r._flags),
      _name(NULL),
      _layer_rules(r._layer_rules),
      _vias(r._vias),
      _samenet_rules(r._samenet_rules),
      _samenet_matrix(r._samenet_matrix),
      _use_vias(r._use_vias),
      _use_rules(r._use_rules),
      _cut_layers(r._cut_layers),
      _min_cuts(r._min_cuts)
{
  if (r._name) {
    _name = strdup(r._name);
    ZALLOCATED(_name);
  }
}

_dbTechNonDefaultRule::_dbTechNonDefaultRule(_dbDatabase*)
{
  _flags._spare_bits   = 0;
  _flags._hard_spacing = 0;
  _flags._block_rule   = 0;
  _name                = NULL;
}

_dbTechNonDefaultRule::~_dbTechNonDefaultRule()
{
  if (_name)
    free((void*) _name);
}

dbOStream& operator<<(dbOStream& stream, const _dbTechNonDefaultRule& rule)
{
  uint* bit_field = (uint*) &rule._flags;
  stream << *bit_field;
  stream << rule._name;
  stream << rule._layer_rules;
  stream << rule._vias;
  stream << rule._samenet_rules;
  stream << rule._samenet_matrix;
  stream << rule._use_vias;
  stream << rule._use_rules;
  stream << rule._cut_layers;
  stream << rule._min_cuts;
  return stream;
}

dbIStream& operator>>(dbIStream& stream, _dbTechNonDefaultRule& rule)
{
  uint* bit_field = (uint*) &rule._flags;
  stream >> *bit_field;
  stream >> rule._name;
  stream >> rule._layer_rules;
  stream >> rule._vias;
  stream >> rule._samenet_rules;
  stream >> rule._samenet_matrix;
  stream >> rule._use_vias;
  stream >> rule._use_rules;
  stream >> rule._cut_layers;
  stream >> rule._min_cuts;

  return stream;
}

bool _dbTechNonDefaultRule::operator==(const _dbTechNonDefaultRule& rhs) const
{
  if (_flags._hard_spacing != rhs._flags._hard_spacing)
    return false;

  if (_flags._block_rule != rhs._flags._block_rule)
    return false;

  if (_name && rhs._name) {
    if (strcmp(_name, rhs._name) != 0)
      return false;
  } else if (_name || rhs._name)
    return false;

  if (_layer_rules != rhs._layer_rules)
    return false;

  if (_vias != rhs._vias)
    return false;

  if (_samenet_rules != rhs._samenet_rules)
    return false;

  if (_samenet_matrix != rhs._samenet_matrix)
    return false;

  if (_use_vias != rhs._use_vias)
    return false;

  if (_use_rules != rhs._use_rules)
    return false;

  if (_cut_layers != rhs._cut_layers)
    return false;

  if (_min_cuts != rhs._min_cuts)
    return false;

  return true;
}

void _dbTechNonDefaultRule::differences(dbDiff&                      diff,
                                        const char*                  field,
                                        const _dbTechNonDefaultRule& rhs) const
{
  DIFF_BEGIN
  DIFF_FIELD(_flags._hard_spacing);
  DIFF_FIELD(_flags._block_rule);
  DIFF_FIELD(_name);
  DIFF_VECTOR(_layer_rules);
  DIFF_VECTOR(_vias);
  DIFF_VECTOR(_samenet_rules);
  DIFF_MATRIX(_samenet_matrix);
  DIFF_VECTOR(_use_vias);
  DIFF_VECTOR(_use_rules);
  DIFF_VECTOR(_cut_layers);
  DIFF_VECTOR(_min_cuts);
  DIFF_END
}

void _dbTechNonDefaultRule::out(dbDiff&     diff,
                                char        side,
                                const char* field) const
{
  DIFF_OUT_BEGIN
  DIFF_OUT_FIELD(_flags._hard_spacing);
  DIFF_OUT_FIELD(_flags._block_rule);
  DIFF_OUT_FIELD(_name);
  DIFF_OUT_VECTOR(_layer_rules);
  DIFF_OUT_VECTOR(_vias);
  DIFF_OUT_VECTOR(_samenet_rules);
  DIFF_OUT_MATRIX(_samenet_matrix);
  DIFF_OUT_VECTOR(_use_vias);
  DIFF_OUT_VECTOR(_use_rules);
  DIFF_OUT_VECTOR(_cut_layers);
  DIFF_OUT_VECTOR(_min_cuts);
  DIFF_END
}

_dbTech* _dbTechNonDefaultRule::getTech()
{
#if 0  // dead code generates warnings -cherry
    if (_flags._block_rule == 0)
        (_dbTech *) getOwner();
#endif

  return (_dbTech*) getDb()->getTech();
}

_dbBlock* _dbTechNonDefaultRule::getBlock()
{
  assert(_flags._block_rule == 1);
  return (_dbBlock*) getOwner();
}

bool _dbTechNonDefaultRule::operator<(const _dbTechNonDefaultRule& rhs) const
{
  return strcmp(_name, rhs._name) < 0;
}

////////////////////////////////////////////////////////////////////
//
// dbTechNonDefaultRule - Methods
//
////////////////////////////////////////////////////////////////////

std::string dbTechNonDefaultRule::getName()
{
  _dbTechNonDefaultRule* rule = (_dbTechNonDefaultRule*) this;
  return rule->_name;
}

const char* dbTechNonDefaultRule::getConstName()
{
  _dbTechNonDefaultRule* rule = (_dbTechNonDefaultRule*) this;
  return rule->_name;
}

bool dbTechNonDefaultRule::isBlockRule()
{
  _dbTechNonDefaultRule* rule = (_dbTechNonDefaultRule*) this;
  return rule->_flags._block_rule == 1;
}

dbTechLayerRule* dbTechNonDefaultRule::getLayerRule(dbTechLayer* layer_)
{
  _dbTechNonDefaultRule* rule  = (_dbTechNonDefaultRule*) this;
  _dbTechLayer*          layer = (_dbTechLayer*) layer_;
  dbId<_dbTechLayerRule> id    = rule->_layer_rules[layer->_number];

  if (id == 0)
    return NULL;

  if (rule->_flags._block_rule == 0) {
    return (dbTechLayerRule*) rule->getTech()->_layer_rule_tbl->getPtr(id);
  } else {
    return (dbTechLayerRule*) rule->getBlock()->_layer_rule_tbl->getPtr(id);
  }
}

void dbTechNonDefaultRule::getLayerRules(
    std::vector<dbTechLayerRule*>& layer_rules)
{
  _dbTechNonDefaultRule*     rule           = (_dbTechNonDefaultRule*) this;
  dbTable<_dbTechLayerRule>* layer_rule_tbl = NULL;

  if (rule->_flags._block_rule == 0) {
    _dbTech* tech  = rule->getTech();
    layer_rule_tbl = tech->_layer_rule_tbl;
  } else {
    _dbBlock* block = rule->getBlock();
    layer_rule_tbl  = block->_layer_rule_tbl;
  }

  layer_rules.clear();

  dbVector<dbId<_dbTechLayerRule> >::iterator itr;

  for (itr = rule->_layer_rules.begin(); itr != rule->_layer_rules.end();
       ++itr) {
    dbId<_dbTechLayerRule> id = *itr;

    if (id != 0)
      layer_rules.push_back((dbTechLayerRule*) layer_rule_tbl->getPtr(id));
  }
}

void dbTechNonDefaultRule::getVias(std::vector<dbTechVia*>& vias)
{
  _dbTechNonDefaultRule* rule = (_dbTechNonDefaultRule*) this;

  if (rule->_flags._block_rule == 1)  // not supported on block rules
    return;

  _dbTech* tech = rule->getTech();
  vias.clear();

  dbVector<dbId<_dbTechVia> >::iterator itr;

  for (itr = rule->_vias.begin(); itr != rule->_vias.end(); ++itr) {
    dbId<_dbTechVia> id = *itr;
    vias.push_back((dbTechVia*) tech->_via_tbl->getPtr(id));
  }
}

dbTechSameNetRule* dbTechNonDefaultRule::findSameNetRule(dbTechLayer* l1_,
                                                         dbTechLayer* l2_)
{
  _dbTechNonDefaultRule* ndrule = (_dbTechNonDefaultRule*) this;

  if (ndrule->_flags._block_rule == 1)  // not supported on block rules
    return NULL;

  _dbTech*                 tech = ndrule->getTech();
  _dbTechLayer*            l1   = (_dbTechLayer*) l1_;
  _dbTechLayer*            l2   = (_dbTechLayer*) l2_;
  dbId<_dbTechSameNetRule> rule
      = ndrule->_samenet_matrix(l1->_number, l2->_number);

  if (rule == 0)
    return NULL;

  return (dbTechSameNetRule*) tech->_samenet_rule_tbl->getPtr(rule);
}

void dbTechNonDefaultRule::getSameNetRules(
    std::vector<dbTechSameNetRule*>& rules)
{
  _dbTechNonDefaultRule* ndrule = (_dbTechNonDefaultRule*) this;

  if (ndrule->_flags._block_rule == 1)  // not supported on block rules
    return;

  _dbTech* tech = ndrule->getTech();
  rules.clear();
  dbVector<dbId<_dbTechSameNetRule> >::iterator itr;

  for (itr = ndrule->_samenet_rules.begin();
       itr != ndrule->_samenet_rules.end();
       ++itr) {
    dbId<_dbTechSameNetRule> r = *itr;
    rules.push_back((dbTechSameNetRule*) tech->_samenet_rule_tbl->getPtr(r));
  }
}

bool dbTechNonDefaultRule::getHardSpacing()
{
  _dbTechNonDefaultRule* rule = (_dbTechNonDefaultRule*) this;
  return rule->_flags._hard_spacing == 1;
}

void dbTechNonDefaultRule::setHardSpacing(bool value)
{
  _dbTechNonDefaultRule* rule = (_dbTechNonDefaultRule*) this;
  rule->_flags._hard_spacing  = value;
}

void dbTechNonDefaultRule::addUseVia(dbTechVia* via)
{
  _dbTechNonDefaultRule* rule = (_dbTechNonDefaultRule*) this;
  rule->_use_vias.push_back(via->getId());
}

void dbTechNonDefaultRule::getUseVias(std::vector<dbTechVia*>& vias)
{
  _dbTechNonDefaultRule* rule = (_dbTechNonDefaultRule*) this;
  _dbTech*               tech = rule->getTech();

  dbVector<dbId<_dbTechVia> >::iterator itr;

  for (itr = rule->_use_vias.begin(); itr != rule->_use_vias.end(); ++itr) {
    dbId<_dbTechVia> vid = *itr;
    dbTechVia*       via = dbTechVia::getTechVia((dbTech*) tech, vid);
    vias.push_back(via);
  }
}

void dbTechNonDefaultRule::addUseViaRule(dbTechViaGenerateRule* gen_rule)
{
  _dbTechNonDefaultRule* rule = (_dbTechNonDefaultRule*) this;
  rule->_use_rules.push_back(gen_rule->getId());
}

void dbTechNonDefaultRule::getUseViaRules(
    std::vector<dbTechViaGenerateRule*>& rules)
{
  _dbTechNonDefaultRule* rule = (_dbTechNonDefaultRule*) this;
  _dbTech*               tech = rule->getTech();

  dbVector<dbId<_dbTechViaGenerateRule> >::iterator itr;

  for (itr = rule->_use_rules.begin(); itr != rule->_use_rules.end(); ++itr) {
    dbId<_dbTechViaGenerateRule> rid = *itr;
    dbTechViaGenerateRule*       rule
        = dbTechViaGenerateRule::getTechViaGenerateRule((dbTech*) tech, rid);
    rules.push_back(rule);
  }
}

void dbTechNonDefaultRule::setMinCuts(dbTechLayer* cut_layer, int count)
{
  _dbTechNonDefaultRule* rule = (_dbTechNonDefaultRule*) this;

  uint                                    id = cut_layer->getId();
  dbVector<dbId<_dbTechLayer> >::iterator itr;
  uint                                    idx = 0;

  for (itr = rule->_cut_layers.begin(); itr != rule->_cut_layers.end();
       ++itr, ++idx) {
    dbId<_dbTechLayer> lid = *itr;

    if (lid == id) {
      rule->_min_cuts[idx] = count;
      return;
    }
  }

  rule->_cut_layers.push_back(id);
  rule->_min_cuts.push_back(count);
}

bool dbTechNonDefaultRule::getMinCuts(dbTechLayer* cut_layer, int& count)
{
  _dbTechNonDefaultRule* rule = (_dbTechNonDefaultRule*) this;

  uint                                    id = cut_layer->getId();
  dbVector<dbId<_dbTechLayer> >::iterator itr;
  uint                                    idx = 0;

  for (itr = rule->_cut_layers.begin(); itr != rule->_cut_layers.end();
       ++itr, ++idx) {
    dbId<_dbTechLayer> lid = *itr;

    if (lid == id) {
      count = rule->_min_cuts[idx];
      return true;
    }
  }

  return false;
}

dbTechNonDefaultRule* dbTechNonDefaultRule::create(dbTech*     tech_,
                                                   const char* name_)
{
  if (tech_->findNonDefaultRule(name_))
    return NULL;

  _dbTech*               tech = (_dbTech*) tech_;
  _dbTechNonDefaultRule* rule = tech->_non_default_rule_tbl->create();
  rule->_name                 = strdup(name_);
  ZALLOCATED(rule->_name);
  rule->_layer_rules.resize(tech->_layer_cnt);

  int i;
  for (i = 0; i < tech->_layer_cnt; ++i)
    rule->_layer_rules.push_back(0);

  return (dbTechNonDefaultRule*) rule;
}

dbTechNonDefaultRule* dbTechNonDefaultRule::create(dbBlock*    block_,
                                                   const char* name_)
{
  if (block_->findNonDefaultRule(name_))
    return NULL;

  _dbBlock*              block = (_dbBlock*) block_;
  _dbTech*               tech  = (_dbTech*) block->getDb()->getTech();
  _dbTechNonDefaultRule* rule  = block->_non_default_rule_tbl->create();

  rule->_name = strdup(name_);
  ZALLOCATED(rule->_name);
  rule->_flags._block_rule = 1;
  rule->_layer_rules.resize(tech->_layer_cnt);

  int i;
  for (i = 0; i < tech->_layer_cnt; ++i)
    rule->_layer_rules.push_back(0);

  return (dbTechNonDefaultRule*) rule;
}

dbTechNonDefaultRule* dbTechNonDefaultRule::getTechNonDefaultRule(dbTech* tech_,
                                                                  uint    dbid_)
{
  _dbTech* tech = (_dbTech*) tech_;
  return (dbTechNonDefaultRule*) tech->_non_default_rule_tbl->getPtr(dbid_);
}

dbTechNonDefaultRule* dbTechNonDefaultRule::getTechNonDefaultRule(
    dbBlock* block_,
    uint     dbid_)
{
  _dbBlock* block = (_dbBlock*) block_;
  return (dbTechNonDefaultRule*) block->_non_default_rule_tbl->getPtr(dbid_);
}

}  // namespace odb
