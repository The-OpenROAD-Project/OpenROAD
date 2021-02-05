///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2020, OpenRoad Project
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

// Generator Code Begin 1
#include "dbTechLayerCutClassRule.h"

#include "db.h"
#include "dbDatabase.h"
#include "dbDiff.hpp"
#include "dbHashTable.h"
#include "dbTable.h"
#include "dbTable.hpp"
#include "dbTechLayer.h"
// User Code Begin includes
// User Code End includes
namespace odb {

// User Code Begin definitions
// User Code End definitions
template class dbTable<_dbTechLayerCutClassRule>;

bool _dbTechLayerCutClassRule::operator==(
    const _dbTechLayerCutClassRule& rhs) const
{
  if (_flags._length_valid != rhs._flags._length_valid)
    return false;

  if (_flags._cuts_valid != rhs._flags._cuts_valid)
    return false;

  if (_name != rhs._name)
    return false;

  if (_width != rhs._width)
    return false;

  if (_length != rhs._length)
    return false;

  if (_num_cuts != rhs._num_cuts)
    return false;

  if (_next_entry != rhs._next_entry)
    return false;

  // User Code Begin ==
  // User Code End ==
  return true;
}
bool _dbTechLayerCutClassRule::operator<(
    const _dbTechLayerCutClassRule& rhs) const
{
  // User Code Begin <
  // User Code End <
  return true;
}
void _dbTechLayerCutClassRule::differences(
    dbDiff&                         diff,
    const char*                     field,
    const _dbTechLayerCutClassRule& rhs) const
{
  DIFF_BEGIN

  DIFF_FIELD(_flags._length_valid);
  DIFF_FIELD(_flags._cuts_valid);
  DIFF_FIELD(_name);
  DIFF_FIELD(_width);
  DIFF_FIELD(_length);
  DIFF_FIELD(_num_cuts);
  DIFF_FIELD_NO_DEEP(_next_entry);
  // User Code Begin differences
  // User Code End differences
  DIFF_END
}
void _dbTechLayerCutClassRule::out(dbDiff&     diff,
                                   char        side,
                                   const char* field) const
{
  DIFF_OUT_BEGIN
  DIFF_OUT_FIELD(_flags._length_valid);
  DIFF_OUT_FIELD(_flags._cuts_valid);
  DIFF_OUT_FIELD(_name);
  DIFF_OUT_FIELD(_width);
  DIFF_OUT_FIELD(_length);
  DIFF_OUT_FIELD(_num_cuts);
  DIFF_OUT_FIELD_NO_DEEP(_next_entry);

  // User Code Begin out
  // User Code End out
  DIFF_END
}
_dbTechLayerCutClassRule::_dbTechLayerCutClassRule(_dbDatabase* db)
{
  uint* _flags_bit_field = (uint*) &_flags;
  *_flags_bit_field      = 0;
  // User Code Begin constructor
  // User Code End constructor
}
_dbTechLayerCutClassRule::_dbTechLayerCutClassRule(
    _dbDatabase*                    db,
    const _dbTechLayerCutClassRule& r)
{
  _flags._length_valid = r._flags._length_valid;
  _flags._cuts_valid   = r._flags._cuts_valid;
  _flags._spare_bits   = r._flags._spare_bits;
  _name                = r._name;
  _width               = r._width;
  _length              = r._length;
  _num_cuts            = r._num_cuts;
  _next_entry          = r._next_entry;
  // User Code Begin CopyConstructor
  // User Code End CopyConstructor
}

dbIStream& operator>>(dbIStream& stream, _dbTechLayerCutClassRule& obj)
{
  uint* _flags_bit_field = (uint*) &obj._flags;
  stream >> *_flags_bit_field;
  stream >> obj._name;
  stream >> obj._width;
  stream >> obj._length;
  stream >> obj._num_cuts;
  stream >> obj._next_entry;
  // User Code Begin >>
  // User Code End >>
  return stream;
}
dbOStream& operator<<(dbOStream& stream, const _dbTechLayerCutClassRule& obj)
{
  uint* _flags_bit_field = (uint*) &obj._flags;
  stream << *_flags_bit_field;
  stream << obj._name;
  stream << obj._width;
  stream << obj._length;
  stream << obj._num_cuts;
  stream << obj._next_entry;
  // User Code Begin <<
  // User Code End <<
  return stream;
}

_dbTechLayerCutClassRule::~_dbTechLayerCutClassRule()
{
  if (_name)
    free((void*) _name);
  // User Code Begin Destructor
  // User Code End Destructor
}
////////////////////////////////////////////////////////////////////
//
// dbTechLayerCutClassRule - Methods
//
////////////////////////////////////////////////////////////////////

char* dbTechLayerCutClassRule::getName() const
{
  _dbTechLayerCutClassRule* obj = (_dbTechLayerCutClassRule*) this;
  return obj->_name;
}

void dbTechLayerCutClassRule::setWidth(int _width)
{
  _dbTechLayerCutClassRule* obj = (_dbTechLayerCutClassRule*) this;

  obj->_width = _width;
}

int dbTechLayerCutClassRule::getWidth() const
{
  _dbTechLayerCutClassRule* obj = (_dbTechLayerCutClassRule*) this;
  return obj->_width;
}

void dbTechLayerCutClassRule::setLength(int _length)
{
  _dbTechLayerCutClassRule* obj = (_dbTechLayerCutClassRule*) this;

  obj->_length = _length;
}

int dbTechLayerCutClassRule::getLength() const
{
  _dbTechLayerCutClassRule* obj = (_dbTechLayerCutClassRule*) this;
  return obj->_length;
}

void dbTechLayerCutClassRule::setNumCuts(int _num_cuts)
{
  _dbTechLayerCutClassRule* obj = (_dbTechLayerCutClassRule*) this;

  obj->_num_cuts = _num_cuts;
}

int dbTechLayerCutClassRule::getNumCuts() const
{
  _dbTechLayerCutClassRule* obj = (_dbTechLayerCutClassRule*) this;
  return obj->_num_cuts;
}

void dbTechLayerCutClassRule::setLengthValid(bool _length_valid)
{
  _dbTechLayerCutClassRule* obj = (_dbTechLayerCutClassRule*) this;

  obj->_flags._length_valid = _length_valid;
}

bool dbTechLayerCutClassRule::isLengthValid() const
{
  _dbTechLayerCutClassRule* obj = (_dbTechLayerCutClassRule*) this;

  return obj->_flags._length_valid;
}

void dbTechLayerCutClassRule::setCutsValid(bool _cuts_valid)
{
  _dbTechLayerCutClassRule* obj = (_dbTechLayerCutClassRule*) this;

  obj->_flags._cuts_valid = _cuts_valid;
}

bool dbTechLayerCutClassRule::isCutsValid() const
{
  _dbTechLayerCutClassRule* obj = (_dbTechLayerCutClassRule*) this;

  return obj->_flags._cuts_valid;
}

// User Code Begin dbTechLayerCutClassRulePublicMethods
dbTechLayerCutClassRule* dbTechLayerCutClassRule::create(dbTechLayer* _layer,
                                                         const char*  name)
{
  if (_layer->findTechLayerCutClassRule(name) != nullptr)
    return nullptr;
  _dbTechLayer*             layer   = (_dbTechLayer*) _layer;
  _dbTechLayerCutClassRule* newrule = layer->_cut_class_rules_tbl->create();
  newrule->_name                    = strdup(name);
  ZALLOCATED(newrule->_name);
  layer->_cut_class_rules_hash.insert(newrule);
  return ((dbTechLayerCutClassRule*) newrule);
}

dbTechLayerCutClassRule* dbTechLayerCutClassRule::getTechLayerCutClassRule(
    dbTechLayer* inly,
    uint         dbid)
{
  _dbTechLayer* layer = (_dbTechLayer*) inly;
  return (dbTechLayerCutClassRule*) layer->_cut_class_rules_tbl->getPtr(dbid);
}
void dbTechLayerCutClassRule::destroy(dbTechLayerCutClassRule* rule)
{
  _dbTechLayerCutClassRule* _rule = (_dbTechLayerCutClassRule*) rule;
  _dbTechLayer* layer = (_dbTechLayer*) _rule->getOwner();
  layer->_cut_class_rules_hash.remove(_rule);
  dbProperty::destroyProperties(rule);
  layer->_cut_class_rules_tbl->destroy((_dbTechLayerCutClassRule*) rule);
}
// User Code End dbTechLayerCutClassRulePublicMethods
}  // namespace odb
   // Generator Code End 1