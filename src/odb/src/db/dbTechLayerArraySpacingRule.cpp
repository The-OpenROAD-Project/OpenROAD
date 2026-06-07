// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbTechLayerArraySpacingRule.h"

#include <cstdint>
#include <cstring>
#include <map>

#include "dbCore.h"
#include "dbDatabase.h"
#include "dbProperty.h"
#include "dbTable.h"
#include "dbTechLayer.h"
#include "dbTechLayerCutClassRule.h"
#include "odb/db.h"
namespace odb {
template class dbTable<_dbTechLayerArraySpacingRule>;

bool _dbTechLayerArraySpacingRule::operator==(
    const _dbTechLayerArraySpacingRule& rhs) const
{
  // NOLINTBEGIN(readability-simplify-boolean-expr)
  if (flags_.parallel_overlap != rhs.flags_.parallel_overlap) {
    return false;
  }
  if (flags_.long_array != rhs.flags_.long_array) {
    return false;
  }
  if (flags_.via_width_valid != rhs.flags_.via_width_valid) {
    return false;
  }
  if (flags_.within_valid != rhs.flags_.within_valid) {
    return false;
  }
  if (via_width_ != rhs.via_width_) {
    return false;
  }
  if (cut_spacing_ != rhs.cut_spacing_) {
    return false;
  }
  if (within_ != rhs.within_) {
    return false;
  }
  if (array_width_ != rhs.array_width_) {
    return false;
  }
  if (cut_class_ != rhs.cut_class_) {
    return false;
  }

  return true;
  // NOLINTEND(readability-simplify-boolean-expr)
}

bool _dbTechLayerArraySpacingRule::operator<(
    const _dbTechLayerArraySpacingRule& rhs) const
{
  return true;
}

_dbTechLayerArraySpacingRule::_dbTechLayerArraySpacingRule(_dbDatabase* db)
{
  flags_ = {};
  via_width_ = 0;
  cut_spacing_ = 0;
  within_ = 0;
  array_width_ = 0;
}

dbIStream& operator>>(dbIStream& stream, _dbTechLayerArraySpacingRule& obj)
{
  uint32_t flags_bit_field;
  stream >> flags_bit_field;
  static_assert(sizeof(obj.flags_) == sizeof(flags_bit_field));
  std::memcpy(&obj.flags_, &flags_bit_field, sizeof(flags_bit_field));
  stream >> obj.via_width_;
  stream >> obj.cut_spacing_;
  stream >> obj.within_;
  stream >> obj.array_width_;
  stream >> obj.array_spacing_map_;
  stream >> obj.cut_class_;
  return stream;
}

dbOStream& operator<<(dbOStream& stream,
                      const _dbTechLayerArraySpacingRule& obj)
{
  uint32_t flags_bit_field;
  static_assert(sizeof(obj.flags_) == sizeof(flags_bit_field));
  std::memcpy(&flags_bit_field, &obj.flags_, sizeof(obj.flags_));
  stream << flags_bit_field;
  stream << obj.via_width_;
  stream << obj.cut_spacing_;
  stream << obj.within_;
  stream << obj.array_width_;
  stream << obj.array_spacing_map_;
  stream << obj.cut_class_;
  return stream;
}

void _dbTechLayerArraySpacingRule::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);

  info.children["array_spacing_map"].add(array_spacing_map_);
}

////////////////////////////////////////////////////////////////////
//
// dbTechLayerArraySpacingRule - Methods
//
////////////////////////////////////////////////////////////////////

void dbTechLayerArraySpacingRule::setViaWidth(int via_width)
{
  _dbTechLayerArraySpacingRule* obj = (_dbTechLayerArraySpacingRule*) this;

  obj->via_width_ = via_width;
}

int dbTechLayerArraySpacingRule::getViaWidth() const
{
  _dbTechLayerArraySpacingRule* obj = (_dbTechLayerArraySpacingRule*) this;
  return obj->via_width_;
}

void dbTechLayerArraySpacingRule::setCutSpacing(int cut_spacing)
{
  _dbTechLayerArraySpacingRule* obj = (_dbTechLayerArraySpacingRule*) this;

  obj->cut_spacing_ = cut_spacing;
}

int dbTechLayerArraySpacingRule::getCutSpacing() const
{
  _dbTechLayerArraySpacingRule* obj = (_dbTechLayerArraySpacingRule*) this;
  return obj->cut_spacing_;
}

void dbTechLayerArraySpacingRule::setWithin(int within)
{
  _dbTechLayerArraySpacingRule* obj = (_dbTechLayerArraySpacingRule*) this;

  obj->within_ = within;
}

int dbTechLayerArraySpacingRule::getWithin() const
{
  _dbTechLayerArraySpacingRule* obj = (_dbTechLayerArraySpacingRule*) this;
  return obj->within_;
}

void dbTechLayerArraySpacingRule::setArrayWidth(int array_width)
{
  _dbTechLayerArraySpacingRule* obj = (_dbTechLayerArraySpacingRule*) this;

  obj->array_width_ = array_width;
}

int dbTechLayerArraySpacingRule::getArrayWidth() const
{
  _dbTechLayerArraySpacingRule* obj = (_dbTechLayerArraySpacingRule*) this;
  return obj->array_width_;
}

void dbTechLayerArraySpacingRule::setCutClass(
    dbTechLayerCutClassRule* cut_class)
{
  _dbTechLayerArraySpacingRule* obj = (_dbTechLayerArraySpacingRule*) this;

  obj->cut_class_ = cut_class->getImpl()->getOID();
}

void dbTechLayerArraySpacingRule::setParallelOverlap(bool parallel_overlap)
{
  _dbTechLayerArraySpacingRule* obj = (_dbTechLayerArraySpacingRule*) this;

  obj->flags_.parallel_overlap = parallel_overlap;
}

bool dbTechLayerArraySpacingRule::isParallelOverlap() const
{
  _dbTechLayerArraySpacingRule* obj = (_dbTechLayerArraySpacingRule*) this;

  return obj->flags_.parallel_overlap;
}

void dbTechLayerArraySpacingRule::setLongArray(bool long_array)
{
  _dbTechLayerArraySpacingRule* obj = (_dbTechLayerArraySpacingRule*) this;

  obj->flags_.long_array = long_array;
}

bool dbTechLayerArraySpacingRule::isLongArray() const
{
  _dbTechLayerArraySpacingRule* obj = (_dbTechLayerArraySpacingRule*) this;

  return obj->flags_.long_array;
}

void dbTechLayerArraySpacingRule::setViaWidthValid(bool via_width_valid)
{
  _dbTechLayerArraySpacingRule* obj = (_dbTechLayerArraySpacingRule*) this;

  obj->flags_.via_width_valid = via_width_valid;
}

bool dbTechLayerArraySpacingRule::isViaWidthValid() const
{
  _dbTechLayerArraySpacingRule* obj = (_dbTechLayerArraySpacingRule*) this;

  return obj->flags_.via_width_valid;
}

void dbTechLayerArraySpacingRule::setWithinValid(bool within_valid)
{
  _dbTechLayerArraySpacingRule* obj = (_dbTechLayerArraySpacingRule*) this;

  obj->flags_.within_valid = within_valid;
}

bool dbTechLayerArraySpacingRule::isWithinValid() const
{
  _dbTechLayerArraySpacingRule* obj = (_dbTechLayerArraySpacingRule*) this;

  return obj->flags_.within_valid;
}

dbTechLayerArraySpacingRule* dbTechLayerArraySpacingRule::create(
    dbTechLayer* parent)
{
  _dbTechLayer* _parent = (_dbTechLayer*) parent;
  return (dbTechLayerArraySpacingRule*)
      _parent->array_spacing_rules_tbl_->create();
}
void dbTechLayerArraySpacingRule::destroy(dbTechLayerArraySpacingRule* obj)
{
  _dbTechLayer* _parent = (_dbTechLayer*) obj->getImpl()->getOwner();
  dbProperty::destroyProperties(obj);
  _parent->array_spacing_rules_tbl_->destroy(
      (_dbTechLayerArraySpacingRule*) obj);
}
// User Code Begin dbTechLayerArraySpacingRulePublicMethods

void dbTechLayerArraySpacingRule::setCutsArraySpacing(int num_cuts, int spacing)
{
  _dbTechLayerArraySpacingRule* obj = (_dbTechLayerArraySpacingRule*) this;
  obj->array_spacing_map_[num_cuts] = spacing;
}

const std::map<int, int>& dbTechLayerArraySpacingRule::getCutsArraySpacing()
    const
{
  _dbTechLayerArraySpacingRule* obj = (_dbTechLayerArraySpacingRule*) this;
  return obj->array_spacing_map_;
}

dbTechLayerCutClassRule* dbTechLayerArraySpacingRule::getCutClass() const
{
  _dbTechLayerArraySpacingRule* obj = (_dbTechLayerArraySpacingRule*) this;
  if (!obj->cut_class_.isValid()) {
    return nullptr;
  }
  _dbTechLayer* layer = (_dbTechLayer*) obj->getOwner();
  return (dbTechLayerCutClassRule*) layer->cut_class_rules_tbl_->getPtr(
      obj->cut_class_);
}
dbTechLayerArraySpacingRule*
dbTechLayerArraySpacingRule::getTechLayerArraySpacingRule(dbTechLayer* inly,
                                                          uint32_t dbid)
{
  _dbTechLayer* layer = (_dbTechLayer*) inly;
  return ((dbTechLayerArraySpacingRule*)
              layer->array_spacing_rules_tbl_->getPtr(dbid));
}
// User Code End dbTechLayerArraySpacingRulePublicMethods
}  // namespace odb
   // Generator Code End Cpp
