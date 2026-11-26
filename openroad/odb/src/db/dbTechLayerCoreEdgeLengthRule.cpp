// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbTechLayerCoreEdgeLengthRule.h"

#include "dbCore.h"
#include "dbDatabase.h"
#include "dbProperty.h"
#include "dbTable.h"
#include "dbTechLayer.h"
#include "odb/db.h"
// User Code Begin Includes
#include <vector>
// User Code End Includes
namespace odb {
template class dbTable<_dbTechLayerCoreEdgeLengthRule>;

bool _dbTechLayerCoreEdgeLengthRule::operator==(
    const _dbTechLayerCoreEdgeLengthRule& rhs) const
{
  // NOLINTBEGIN(readability-simplify-boolean-expr)
  if (min_length_ != rhs.min_length_) {
    return false;
  }
  if (max_length_ != rhs.max_length_) {
    return false;
  }

  // User Code Begin ==
  if (except_adjacent_edge_lengths_ != rhs.except_adjacent_edge_lengths_) {
    return false;
  }
  if (except_adjacent_lengths_ != rhs.except_adjacent_lengths_) {
    return false;
  }
  if (except_adjacent_exact_ != rhs.except_adjacent_exact_) {
    return false;
  }
  if (within_layers_ != rhs.within_layers_) {
    return false;
  }
  if (except_layers_ != rhs.except_layers_) {
    return false;
  }
  // User Code End ==
  return true;
  // NOLINTEND(readability-simplify-boolean-expr)
}

bool _dbTechLayerCoreEdgeLengthRule::operator<(
    const _dbTechLayerCoreEdgeLengthRule& rhs) const
{
  return true;
}

_dbTechLayerCoreEdgeLengthRule::_dbTechLayerCoreEdgeLengthRule(_dbDatabase* db)
{
  min_length_ = 0;
  max_length_ = 0;
}

dbIStream& operator>>(dbIStream& stream, _dbTechLayerCoreEdgeLengthRule& obj)
{
  stream >> obj.min_length_;
  stream >> obj.max_length_;
  stream >> obj.except_adjacent_edge_lengths_;
  stream >> obj.except_adjacent_lengths_;
  stream >> obj.except_adjacent_exact_;
  stream >> obj.within_layers_;
  stream >> obj.except_layers_;
  return stream;
}

dbOStream& operator<<(dbOStream& stream,
                      const _dbTechLayerCoreEdgeLengthRule& obj)
{
  stream << obj.min_length_;
  stream << obj.max_length_;
  stream << obj.except_adjacent_edge_lengths_;
  stream << obj.except_adjacent_lengths_;
  stream << obj.except_adjacent_exact_;
  stream << obj.within_layers_;
  stream << obj.except_layers_;
  return stream;
}

void _dbTechLayerCoreEdgeLengthRule::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);

  info.children["except_adjacent_edge_lengths"].add(
      except_adjacent_edge_lengths_);
  info.children["except_adjacent_lengths"].add(except_adjacent_lengths_);
  info.children["except_adjacent_exact"].add(except_adjacent_exact_);
  info.children["within_layers"].add(within_layers_);
  info.children["except_layers"].add(except_layers_);
}

////////////////////////////////////////////////////////////////////
//
// dbTechLayerCoreEdgeLengthRule - Methods
//
////////////////////////////////////////////////////////////////////

void dbTechLayerCoreEdgeLengthRule::setMinLength(int min_length)
{
  _dbTechLayerCoreEdgeLengthRule* obj
      = reinterpret_cast<_dbTechLayerCoreEdgeLengthRule*>(this);

  obj->min_length_ = min_length;
}

int dbTechLayerCoreEdgeLengthRule::getMinLength() const
{
  const _dbTechLayerCoreEdgeLengthRule* obj
      = reinterpret_cast<const _dbTechLayerCoreEdgeLengthRule*>(this);
  return obj->min_length_;
}

void dbTechLayerCoreEdgeLengthRule::setMaxLength(int max_length)
{
  _dbTechLayerCoreEdgeLengthRule* obj
      = reinterpret_cast<_dbTechLayerCoreEdgeLengthRule*>(this);

  obj->max_length_ = max_length;
}

int dbTechLayerCoreEdgeLengthRule::getMaxLength() const
{
  const _dbTechLayerCoreEdgeLengthRule* obj
      = reinterpret_cast<const _dbTechLayerCoreEdgeLengthRule*>(this);
  return obj->max_length_;
}

void dbTechLayerCoreEdgeLengthRule::getExceptAdjacentEdgeLengths(
    std::vector<int>& tbl) const
{
  const _dbTechLayerCoreEdgeLengthRule* obj
      = reinterpret_cast<const _dbTechLayerCoreEdgeLengthRule*>(this);
  tbl = obj->except_adjacent_edge_lengths_;
}

void dbTechLayerCoreEdgeLengthRule::getExceptAdjacentLengths(
    std::vector<int>& tbl) const
{
  const _dbTechLayerCoreEdgeLengthRule* obj
      = reinterpret_cast<const _dbTechLayerCoreEdgeLengthRule*>(this);
  tbl = obj->except_adjacent_lengths_;
}

void dbTechLayerCoreEdgeLengthRule::getExceptAdjacentExact(
    std::vector<int>& tbl) const
{
  const _dbTechLayerCoreEdgeLengthRule* obj
      = reinterpret_cast<const _dbTechLayerCoreEdgeLengthRule*>(this);
  tbl = obj->except_adjacent_exact_;
}

dbTechLayerCoreEdgeLengthRule* dbTechLayerCoreEdgeLengthRule::create(
    dbTechLayer* parent)
{
  _dbTechLayer* _parent = reinterpret_cast<_dbTechLayer*>(parent);
  return reinterpret_cast<dbTechLayerCoreEdgeLengthRule*>(
      _parent->core_edge_length_rules_tbl_->create());
}
void dbTechLayerCoreEdgeLengthRule::destroy(dbTechLayerCoreEdgeLengthRule* obj)
{
  _dbTechLayer* _parent
      = static_cast<_dbTechLayer*>(obj->getImpl()->getOwner());
  dbProperty::destroyProperties(obj);
  _parent->core_edge_length_rules_tbl_->destroy(
      reinterpret_cast<_dbTechLayerCoreEdgeLengthRule*>(obj));
}
// User Code Begin dbTechLayerCoreEdgeLengthRulePublicMethods
void dbTechLayerCoreEdgeLengthRule::addExceptAdjacentLength(
    int edge_length,
    int adj_length,
    bool exact_adjacent_length)
{
  auto* obj = reinterpret_cast<_dbTechLayerCoreEdgeLengthRule*>(this);
  obj->except_adjacent_edge_lengths_.push_back(edge_length);
  obj->except_adjacent_lengths_.push_back(adj_length);
  obj->except_adjacent_exact_.push_back(exact_adjacent_length ? 1 : 0);
}

void dbTechLayerCoreEdgeLengthRule::addWithinLayer(dbTechLayer* layer)
{
  if (layer == nullptr) {
    return;
  }
  auto* obj = reinterpret_cast<_dbTechLayerCoreEdgeLengthRule*>(this);
  obj->within_layers_.push_back(layer->getImpl()->getOID());
}

std::vector<dbTechLayer*> dbTechLayerCoreEdgeLengthRule::getWithinLayers() const
{
  auto* obj = reinterpret_cast<const _dbTechLayerCoreEdgeLengthRule*>(this);
  std::vector<dbTechLayer*> tbl;
  odb::dbTech* tech
      = reinterpret_cast<dbTechLayer*>(obj->getOwner())->getTech();
  for (const auto& lid : obj->within_layers_) {
    if (lid.isValid()) {
      tbl.push_back(odb::dbTechLayer::getTechLayer(tech, lid));
    }
  }
  return tbl;
}

void dbTechLayerCoreEdgeLengthRule::addExceptLayer(dbTechLayer* layer)
{
  if (layer == nullptr) {
    return;
  }
  auto* obj = reinterpret_cast<_dbTechLayerCoreEdgeLengthRule*>(this);
  obj->except_layers_.push_back(layer->getImpl()->getOID());
}

std::vector<dbTechLayer*> dbTechLayerCoreEdgeLengthRule::getExceptLayers() const
{
  auto* obj = reinterpret_cast<const _dbTechLayerCoreEdgeLengthRule*>(this);
  std::vector<dbTechLayer*> tbl;
  odb::dbTech* tech
      = reinterpret_cast<dbTechLayer*>(obj->getOwner())->getTech();
  for (const auto& lid : obj->except_layers_) {
    if (lid.isValid()) {
      tbl.push_back(odb::dbTechLayer::getTechLayer(tech, lid));
    }
  }
  return tbl;
}
// User Code End dbTechLayerCoreEdgeLengthRulePublicMethods
}  // namespace odb
// Generator Code End Cpp
