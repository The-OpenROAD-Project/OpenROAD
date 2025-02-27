///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2020, The Regents of the University of California
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
#include "dbTechLayerCutClassRule.h"

#include <cstdint>
#include <cstring>

#include "dbDatabase.h"
#include "dbHashTable.h"
#include "dbTable.h"
#include "dbTable.hpp"
#include "dbTechLayer.h"
#include "odb/db.h"
// User Code Begin Includes
#include "dbHashTable.hpp"
// User Code End Includes
namespace odb {
template class dbTable<_dbTechLayerCutClassRule>;

bool _dbTechLayerCutClassRule::operator==(
    const _dbTechLayerCutClassRule& rhs) const
{
  if (flags_.length_valid_ != rhs.flags_.length_valid_) {
    return false;
  }
  if (flags_.cuts_valid_ != rhs.flags_.cuts_valid_) {
    return false;
  }
  if (_name != rhs._name) {
    return false;
  }
  if (width_ != rhs.width_) {
    return false;
  }
  if (length_ != rhs.length_) {
    return false;
  }
  if (num_cuts_ != rhs.num_cuts_) {
    return false;
  }
  if (_next_entry != rhs._next_entry) {
    return false;
  }

  return true;
}

bool _dbTechLayerCutClassRule::operator<(
    const _dbTechLayerCutClassRule& rhs) const
{
  return true;
}

_dbTechLayerCutClassRule::_dbTechLayerCutClassRule(_dbDatabase* db)
{
  flags_ = {};
  _name = nullptr;
  width_ = 0;
  length_ = 0;
  num_cuts_ = 0;
}

dbIStream& operator>>(dbIStream& stream, _dbTechLayerCutClassRule& obj)
{
  uint32_t flags_bit_field;
  stream >> flags_bit_field;
  static_assert(sizeof(obj.flags_) == sizeof(flags_bit_field));
  std::memcpy(&obj.flags_, &flags_bit_field, sizeof(flags_bit_field));
  stream >> obj._name;
  stream >> obj.width_;
  stream >> obj.length_;
  stream >> obj.num_cuts_;
  stream >> obj._next_entry;
  return stream;
}

dbOStream& operator<<(dbOStream& stream, const _dbTechLayerCutClassRule& obj)
{
  uint32_t flags_bit_field;
  static_assert(sizeof(obj.flags_) == sizeof(flags_bit_field));
  std::memcpy(&flags_bit_field, &obj.flags_, sizeof(obj.flags_));
  stream << flags_bit_field;
  stream << obj._name;
  stream << obj.width_;
  stream << obj.length_;
  stream << obj.num_cuts_;
  stream << obj._next_entry;
  return stream;
}

void _dbTechLayerCutClassRule::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);

  // User Code Begin collectMemInfo
  info.children_["name"].add(_name);
  // User Code End collectMemInfo
}

_dbTechLayerCutClassRule::~_dbTechLayerCutClassRule()
{
  if (_name) {
    free((void*) _name);
  }
}

////////////////////////////////////////////////////////////////////
//
// dbTechLayerCutClassRule - Methods
//
////////////////////////////////////////////////////////////////////

const char* dbTechLayerCutClassRule::getName() const
{
  _dbTechLayerCutClassRule* obj = (_dbTechLayerCutClassRule*) this;
  return obj->_name;
}

void dbTechLayerCutClassRule::setWidth(int width)
{
  _dbTechLayerCutClassRule* obj = (_dbTechLayerCutClassRule*) this;

  obj->width_ = width;
}

int dbTechLayerCutClassRule::getWidth() const
{
  _dbTechLayerCutClassRule* obj = (_dbTechLayerCutClassRule*) this;
  return obj->width_;
}

void dbTechLayerCutClassRule::setLength(int length)
{
  _dbTechLayerCutClassRule* obj = (_dbTechLayerCutClassRule*) this;

  obj->length_ = length;
}

int dbTechLayerCutClassRule::getLength() const
{
  _dbTechLayerCutClassRule* obj = (_dbTechLayerCutClassRule*) this;
  return obj->length_;
}

void dbTechLayerCutClassRule::setNumCuts(int num_cuts)
{
  _dbTechLayerCutClassRule* obj = (_dbTechLayerCutClassRule*) this;

  obj->num_cuts_ = num_cuts;
}

int dbTechLayerCutClassRule::getNumCuts() const
{
  _dbTechLayerCutClassRule* obj = (_dbTechLayerCutClassRule*) this;
  return obj->num_cuts_;
}

void dbTechLayerCutClassRule::setLengthValid(bool length_valid)
{
  _dbTechLayerCutClassRule* obj = (_dbTechLayerCutClassRule*) this;

  obj->flags_.length_valid_ = length_valid;
}

bool dbTechLayerCutClassRule::isLengthValid() const
{
  _dbTechLayerCutClassRule* obj = (_dbTechLayerCutClassRule*) this;

  return obj->flags_.length_valid_;
}

void dbTechLayerCutClassRule::setCutsValid(bool cuts_valid)
{
  _dbTechLayerCutClassRule* obj = (_dbTechLayerCutClassRule*) this;

  obj->flags_.cuts_valid_ = cuts_valid;
}

bool dbTechLayerCutClassRule::isCutsValid() const
{
  _dbTechLayerCutClassRule* obj = (_dbTechLayerCutClassRule*) this;

  return obj->flags_.cuts_valid_;
}

// User Code Begin dbTechLayerCutClassRulePublicMethods
dbTechLayerCutClassRule* dbTechLayerCutClassRule::create(dbTechLayer* _layer,
                                                         const char* name)
{
  if (_layer->findTechLayerCutClassRule(name) != nullptr) {
    return nullptr;
  }
  _dbTechLayer* layer = (_dbTechLayer*) _layer;
  _dbTechLayerCutClassRule* newrule = layer->cut_class_rules_tbl_->create();
  newrule->_name = strdup(name);
  ZALLOCATED(newrule->_name);
  layer->cut_class_rules_hash_.insert(newrule);
  return ((dbTechLayerCutClassRule*) newrule);
}

dbTechLayerCutClassRule* dbTechLayerCutClassRule::getTechLayerCutClassRule(
    dbTechLayer* inly,
    uint dbid)
{
  _dbTechLayer* layer = (_dbTechLayer*) inly;
  return (dbTechLayerCutClassRule*) layer->cut_class_rules_tbl_->getPtr(dbid);
}
void dbTechLayerCutClassRule::destroy(dbTechLayerCutClassRule* rule)
{
  _dbTechLayerCutClassRule* _rule = (_dbTechLayerCutClassRule*) rule;
  _dbTechLayer* layer = (_dbTechLayer*) _rule->getOwner();
  layer->cut_class_rules_hash_.remove(_rule);
  dbProperty::destroyProperties(rule);
  layer->cut_class_rules_tbl_->destroy((_dbTechLayerCutClassRule*) rule);
}
// User Code End dbTechLayerCutClassRulePublicMethods
}  // namespace odb
// Generator Code End Cpp
