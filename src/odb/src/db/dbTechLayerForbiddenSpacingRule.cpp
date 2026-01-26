// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbTechLayerForbiddenSpacingRule.h"

#include <utility>

#include "dbCore.h"
#include "dbDatabase.h"
#include "dbTable.h"
#include "dbTechLayer.h"
#include "odb/db.h"
namespace odb {
template class dbTable<_dbTechLayerForbiddenSpacingRule>;

bool _dbTechLayerForbiddenSpacingRule::operator==(
    const _dbTechLayerForbiddenSpacingRule& rhs) const
{
  if (width_ != rhs.width_) {
    return false;
  }
  if (within_ != rhs.within_) {
    return false;
  }
  if (prl_ != rhs.prl_) {
    return false;
  }
  if (two_edges_ != rhs.two_edges_) {
    return false;
  }

  return true;
}

bool _dbTechLayerForbiddenSpacingRule::operator<(
    const _dbTechLayerForbiddenSpacingRule& rhs) const
{
  return true;
}

_dbTechLayerForbiddenSpacingRule::_dbTechLayerForbiddenSpacingRule(
    _dbDatabase* db)
{
  width_ = 0;
  within_ = 0;
  prl_ = 0;
  two_edges_ = 0;
}

dbIStream& operator>>(dbIStream& stream, _dbTechLayerForbiddenSpacingRule& obj)
{
  stream >> obj.forbidden_spacing_;
  stream >> obj.width_;
  stream >> obj.within_;
  stream >> obj.prl_;
  stream >> obj.two_edges_;
  return stream;
}

dbOStream& operator<<(dbOStream& stream,
                      const _dbTechLayerForbiddenSpacingRule& obj)
{
  stream << obj.forbidden_spacing_;
  stream << obj.width_;
  stream << obj.within_;
  stream << obj.prl_;
  stream << obj.two_edges_;
  return stream;
}

void _dbTechLayerForbiddenSpacingRule::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);
}

////////////////////////////////////////////////////////////////////
//
// dbTechLayerForbiddenSpacingRule - Methods
//
////////////////////////////////////////////////////////////////////

void dbTechLayerForbiddenSpacingRule::setForbiddenSpacing(
    const std::pair<int, int>& forbidden_spacing)
{
  _dbTechLayerForbiddenSpacingRule* obj
      = (_dbTechLayerForbiddenSpacingRule*) this;

  obj->forbidden_spacing_ = forbidden_spacing;
}

std::pair<int, int> dbTechLayerForbiddenSpacingRule::getForbiddenSpacing() const
{
  _dbTechLayerForbiddenSpacingRule* obj
      = (_dbTechLayerForbiddenSpacingRule*) this;
  return obj->forbidden_spacing_;
}

void dbTechLayerForbiddenSpacingRule::setWidth(int width)
{
  _dbTechLayerForbiddenSpacingRule* obj
      = (_dbTechLayerForbiddenSpacingRule*) this;

  obj->width_ = width;
}

int dbTechLayerForbiddenSpacingRule::getWidth() const
{
  _dbTechLayerForbiddenSpacingRule* obj
      = (_dbTechLayerForbiddenSpacingRule*) this;
  return obj->width_;
}

void dbTechLayerForbiddenSpacingRule::setWithin(int within)
{
  _dbTechLayerForbiddenSpacingRule* obj
      = (_dbTechLayerForbiddenSpacingRule*) this;

  obj->within_ = within;
}

int dbTechLayerForbiddenSpacingRule::getWithin() const
{
  _dbTechLayerForbiddenSpacingRule* obj
      = (_dbTechLayerForbiddenSpacingRule*) this;
  return obj->within_;
}

void dbTechLayerForbiddenSpacingRule::setPrl(int prl)
{
  _dbTechLayerForbiddenSpacingRule* obj
      = (_dbTechLayerForbiddenSpacingRule*) this;

  obj->prl_ = prl;
}

int dbTechLayerForbiddenSpacingRule::getPrl() const
{
  _dbTechLayerForbiddenSpacingRule* obj
      = (_dbTechLayerForbiddenSpacingRule*) this;
  return obj->prl_;
}

void dbTechLayerForbiddenSpacingRule::setTwoEdges(int two_edges)
{
  _dbTechLayerForbiddenSpacingRule* obj
      = (_dbTechLayerForbiddenSpacingRule*) this;

  obj->two_edges_ = two_edges;
}

int dbTechLayerForbiddenSpacingRule::getTwoEdges() const
{
  _dbTechLayerForbiddenSpacingRule* obj
      = (_dbTechLayerForbiddenSpacingRule*) this;
  return obj->two_edges_;
}

// User Code Begin dbTechLayerForbiddenSpacingRulePublicMethods

bool dbTechLayerForbiddenSpacingRule::hasWidth()
{
  _dbTechLayerForbiddenSpacingRule* obj
      = (_dbTechLayerForbiddenSpacingRule*) this;
  return obj->width_ != 0;
}

bool dbTechLayerForbiddenSpacingRule::hasWithin()
{
  _dbTechLayerForbiddenSpacingRule* obj
      = (_dbTechLayerForbiddenSpacingRule*) this;
  return obj->within_ != 0;
}

bool dbTechLayerForbiddenSpacingRule::hasPrl()
{
  _dbTechLayerForbiddenSpacingRule* obj
      = (_dbTechLayerForbiddenSpacingRule*) this;
  return obj->prl_ != 0;
}

bool dbTechLayerForbiddenSpacingRule::hasTwoEdges()
{
  _dbTechLayerForbiddenSpacingRule* obj
      = (_dbTechLayerForbiddenSpacingRule*) this;
  return obj->two_edges_ != 0;
}

dbTechLayerForbiddenSpacingRule* dbTechLayerForbiddenSpacingRule::create(
    dbTechLayer* _layer)
{
  _dbTechLayer* layer = (_dbTechLayer*) _layer;
  _dbTechLayerForbiddenSpacingRule* newrule
      = layer->forbidden_spacing_rules_tbl_->create();
  return ((dbTechLayerForbiddenSpacingRule*) newrule);
}

void dbTechLayerForbiddenSpacingRule::destroy(
    dbTechLayerForbiddenSpacingRule* rule)
{
  _dbTechLayer* layer = (_dbTechLayer*) rule->getImpl()->getOwner();
  dbProperty::destroyProperties(rule);
  layer->forbidden_spacing_rules_tbl_->destroy(
      (_dbTechLayerForbiddenSpacingRule*) rule);
}

// User Code End dbTechLayerForbiddenSpacingRulePublicMethods
}  // namespace odb
// Generator Code End Cpp
