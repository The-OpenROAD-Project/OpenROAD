// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbTechLayerCutSpacingTableOrthRule.h"

#include "dbCore.h"
#include "dbDatabase.h"
#include "dbTable.h"
#include "dbTechLayerCutSpacingRule.h"
#include "odb/db.h"
// User Code Begin Includes
#include <cstdint>
#include <utility>
#include <vector>

#include "dbTechLayer.h"
// User Code End Includes
namespace odb {
template class dbTable<_dbTechLayerCutSpacingTableOrthRule>;

bool _dbTechLayerCutSpacingTableOrthRule::operator==(
    const _dbTechLayerCutSpacingTableOrthRule& rhs) const
{
  return true;
}

bool _dbTechLayerCutSpacingTableOrthRule::operator<(
    const _dbTechLayerCutSpacingTableOrthRule& rhs) const
{
  return true;
}

_dbTechLayerCutSpacingTableOrthRule::_dbTechLayerCutSpacingTableOrthRule(
    _dbDatabase* db)
{
}

dbIStream& operator>>(dbIStream& stream,
                      _dbTechLayerCutSpacingTableOrthRule& obj)
{
  stream >> obj.spacing_tbl_;
  return stream;
}

dbOStream& operator<<(dbOStream& stream,
                      const _dbTechLayerCutSpacingTableOrthRule& obj)
{
  stream << obj.spacing_tbl_;
  return stream;
}

void _dbTechLayerCutSpacingTableOrthRule::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);

  // User Code Begin collectMemInfo
  info.children["spacing_tbl"].add(spacing_tbl_);
  // User Code End collectMemInfo
}

////////////////////////////////////////////////////////////////////
//
// dbTechLayerCutSpacingTableOrthRule - Methods
//
////////////////////////////////////////////////////////////////////

void dbTechLayerCutSpacingTableOrthRule::getSpacingTable(
    std::vector<std::pair<int, int>>& tbl) const
{
  _dbTechLayerCutSpacingTableOrthRule* obj
      = (_dbTechLayerCutSpacingTableOrthRule*) this;
  tbl = obj->spacing_tbl_;
}

// User Code Begin dbTechLayerCutSpacingTableOrthRulePublicMethods
void dbTechLayerCutSpacingTableOrthRule::setSpacingTable(
    const std::vector<std::pair<int, int>>& tbl)
{
  _dbTechLayerCutSpacingTableOrthRule* obj
      = (_dbTechLayerCutSpacingTableOrthRule*) this;
  obj->spacing_tbl_ = tbl;
}

dbTechLayerCutSpacingTableOrthRule* dbTechLayerCutSpacingTableOrthRule::create(
    dbTechLayer* parent)
{
  _dbTechLayer* _parent = (_dbTechLayer*) parent;
  _dbTechLayerCutSpacingTableOrthRule* newrule
      = _parent->cut_spacing_table_orth_tbl_->create();
  return ((dbTechLayerCutSpacingTableOrthRule*) newrule);
}

dbTechLayerCutSpacingTableOrthRule*
dbTechLayerCutSpacingTableOrthRule::getTechLayerCutSpacingTableOrthSubRule(
    dbTechLayer* parent,
    uint32_t dbid)
{
  _dbTechLayer* _parent = (_dbTechLayer*) parent;
  return (dbTechLayerCutSpacingTableOrthRule*)
      _parent->cut_spacing_table_orth_tbl_->getPtr(dbid);
}
void dbTechLayerCutSpacingTableOrthRule::destroy(
    dbTechLayerCutSpacingTableOrthRule* rule)
{
  _dbTechLayer* _parent = (_dbTechLayer*) rule->getImpl()->getOwner();
  dbProperty::destroyProperties(rule);
  _parent->cut_spacing_table_orth_tbl_->destroy(
      (_dbTechLayerCutSpacingTableOrthRule*) rule);
}

// User Code End dbTechLayerCutSpacingTableOrthRulePublicMethods
}  // namespace odb
// Generator Code End Cpp
