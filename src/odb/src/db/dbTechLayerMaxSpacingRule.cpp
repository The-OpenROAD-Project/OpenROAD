// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbTechLayerMaxSpacingRule.h"

#include <string>

#include "dbCore.h"
#include "dbDatabase.h"
#include "dbProperty.h"
#include "dbTable.h"
#include "dbTechLayer.h"
#include "odb/db.h"
namespace odb {
template class dbTable<_dbTechLayerMaxSpacingRule>;

bool _dbTechLayerMaxSpacingRule::operator==(
    const _dbTechLayerMaxSpacingRule& rhs) const
{
  // NOLINTBEGIN(readability-simplify-boolean-expr)
  if (cut_class_ != rhs.cut_class_) {
    return false;
  }
  if (max_spacing_ != rhs.max_spacing_) {
    return false;
  }

  return true;
  // NOLINTEND(readability-simplify-boolean-expr)
}

bool _dbTechLayerMaxSpacingRule::operator<(
    const _dbTechLayerMaxSpacingRule& rhs) const
{
  return true;
}

_dbTechLayerMaxSpacingRule::_dbTechLayerMaxSpacingRule(_dbDatabase* db)
{
  max_spacing_ = 0;
}

dbIStream& operator>>(dbIStream& stream, _dbTechLayerMaxSpacingRule& obj)
{
  stream >> obj.cut_class_;
  stream >> obj.max_spacing_;
  return stream;
}

dbOStream& operator<<(dbOStream& stream, const _dbTechLayerMaxSpacingRule& obj)
{
  stream << obj.cut_class_;
  stream << obj.max_spacing_;
  return stream;
}

void _dbTechLayerMaxSpacingRule::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);

  info.children["cut_class"].add(cut_class_);
}

////////////////////////////////////////////////////////////////////
//
// dbTechLayerMaxSpacingRule - Methods
//
////////////////////////////////////////////////////////////////////

void dbTechLayerMaxSpacingRule::setCutClass(const std::string& cut_class)
{
  _dbTechLayerMaxSpacingRule* obj = (_dbTechLayerMaxSpacingRule*) this;

  obj->cut_class_ = cut_class;
}

const std::string& dbTechLayerMaxSpacingRule::getCutClass() const
{
  _dbTechLayerMaxSpacingRule* obj = (_dbTechLayerMaxSpacingRule*) this;
  return obj->cut_class_;
}

void dbTechLayerMaxSpacingRule::setMaxSpacing(int max_spacing)
{
  _dbTechLayerMaxSpacingRule* obj = (_dbTechLayerMaxSpacingRule*) this;

  obj->max_spacing_ = max_spacing;
}

int dbTechLayerMaxSpacingRule::getMaxSpacing() const
{
  _dbTechLayerMaxSpacingRule* obj = (_dbTechLayerMaxSpacingRule*) this;
  return obj->max_spacing_;
}

dbTechLayerMaxSpacingRule* dbTechLayerMaxSpacingRule::create(
    dbTechLayer* parent)
{
  _dbTechLayer* _parent = (_dbTechLayer*) parent;
  return (dbTechLayerMaxSpacingRule*) _parent->max_spacing_rules_tbl_->create();
}
void dbTechLayerMaxSpacingRule::destroy(dbTechLayerMaxSpacingRule* obj)
{
  _dbTechLayer* _parent = (_dbTechLayer*) obj->getImpl()->getOwner();
  dbProperty::destroyProperties(obj);
  _parent->max_spacing_rules_tbl_->destroy((_dbTechLayerMaxSpacingRule*) obj);
}
// User Code Begin dbTechLayerMaxSpacingRulePublicMethods

bool dbTechLayerMaxSpacingRule::hasCutClass() const
{
  _dbTechLayerMaxSpacingRule* obj = (_dbTechLayerMaxSpacingRule*) this;
  return (!obj->cut_class_.empty());
}
// User Code End dbTechLayerMaxSpacingRulePublicMethods
}  // namespace odb
   // Generator Code End Cpp
