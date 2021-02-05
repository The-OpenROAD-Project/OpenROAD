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
#include "dbTechLayerCutClassSubRule.h"

#include "db.h"
#include "dbDatabase.h"
#include "dbDiff.hpp"
#include "dbTable.h"
#include "dbTable.hpp"

// User Code Begin includes
#include "dbTechLayerCutClassRule.h"
// User Code End includes
namespace odb {

// User Code Begin definitions
// User Code End definitions
template class dbTable<_dbTechLayerCutClassSubRule>;

bool _dbTechLayerCutClassSubRule::operator==(
    const _dbTechLayerCutClassSubRule& rhs) const
{
  if (_flags._length_valid != rhs._flags._length_valid)
    return false;

  if (_flags._cuts_valid != rhs._flags._cuts_valid)
    return false;

  if (_class_name != rhs._class_name)
    return false;

  if (_width != rhs._width)
    return false;

  if (_length != rhs._length)
    return false;

  if (_num_cuts != rhs._num_cuts)
    return false;

  // User Code Begin ==
  // User Code End ==
  return true;
}
bool _dbTechLayerCutClassSubRule::operator<(
    const _dbTechLayerCutClassSubRule& rhs) const
{
  // User Code Begin <
  // User Code End <
  return true;
}
void _dbTechLayerCutClassSubRule::differences(
    dbDiff&                            diff,
    const char*                        field,
    const _dbTechLayerCutClassSubRule& rhs) const
{
  DIFF_BEGIN

  DIFF_FIELD(_flags._length_valid);
  DIFF_FIELD(_flags._cuts_valid);
  DIFF_FIELD(_class_name);
  DIFF_FIELD(_width);
  DIFF_FIELD(_length);
  DIFF_FIELD(_num_cuts);
  // User Code Begin differences
  // User Code End differences
  DIFF_END
}
void _dbTechLayerCutClassSubRule::out(dbDiff&     diff,
                                      char        side,
                                      const char* field) const
{
  DIFF_OUT_BEGIN
  DIFF_OUT_FIELD(_flags._length_valid);
  DIFF_OUT_FIELD(_flags._cuts_valid);
  DIFF_OUT_FIELD(_class_name);
  DIFF_OUT_FIELD(_width);
  DIFF_OUT_FIELD(_length);
  DIFF_OUT_FIELD(_num_cuts);

  // User Code Begin out
  // User Code End out
  DIFF_END
}
_dbTechLayerCutClassSubRule::_dbTechLayerCutClassSubRule(_dbDatabase* db)
{
  uint* _flags_bit_field = (uint*) &_flags;
  *_flags_bit_field      = 0;
  // User Code Begin constructor
  // User Code End constructor
}
_dbTechLayerCutClassSubRule::_dbTechLayerCutClassSubRule(
    _dbDatabase*                       db,
    const _dbTechLayerCutClassSubRule& r)
{
  _flags._length_valid = r._flags._length_valid;
  _flags._cuts_valid   = r._flags._cuts_valid;
  _flags._spare_bits   = r._flags._spare_bits;
  _class_name          = r._class_name;
  _width               = r._width;
  _length              = r._length;
  _num_cuts            = r._num_cuts;
  // User Code Begin CopyConstructor
  // User Code End CopyConstructor
}

dbIStream& operator>>(dbIStream& stream, _dbTechLayerCutClassSubRule& obj)
{
  uint* _flags_bit_field = (uint*) &obj._flags;
  stream >> *_flags_bit_field;
  stream >> obj._class_name;
  stream >> obj._width;
  stream >> obj._length;
  stream >> obj._num_cuts;
  // User Code Begin >>
  // User Code End >>
  return stream;
}
dbOStream& operator<<(dbOStream& stream, const _dbTechLayerCutClassSubRule& obj)
{
  uint* _flags_bit_field = (uint*) &obj._flags;
  stream << *_flags_bit_field;
  stream << obj._class_name;
  stream << obj._width;
  stream << obj._length;
  stream << obj._num_cuts;
  // User Code Begin <<
  // User Code End <<
  return stream;
}

_dbTechLayerCutClassSubRule::~_dbTechLayerCutClassSubRule()
{
  // User Code Begin Destructor
  // User Code End Destructor
}
////////////////////////////////////////////////////////////////////
//
// dbTechLayerCutClassSubRule - Methods
//
////////////////////////////////////////////////////////////////////

char* dbTechLayerCutClassSubRule::getClassName() const
{
  _dbTechLayerCutClassSubRule* obj = (_dbTechLayerCutClassSubRule*) this;
  return obj->_class_name;
}

void dbTechLayerCutClassSubRule::setWidth(int _width)
{
  _dbTechLayerCutClassSubRule* obj = (_dbTechLayerCutClassSubRule*) this;

  obj->_width = _width;
}

int dbTechLayerCutClassSubRule::getWidth() const
{
  _dbTechLayerCutClassSubRule* obj = (_dbTechLayerCutClassSubRule*) this;
  return obj->_width;
}

void dbTechLayerCutClassSubRule::setLength(int _length)
{
  _dbTechLayerCutClassSubRule* obj = (_dbTechLayerCutClassSubRule*) this;

  obj->_length = _length;
}

int dbTechLayerCutClassSubRule::getLength() const
{
  _dbTechLayerCutClassSubRule* obj = (_dbTechLayerCutClassSubRule*) this;
  return obj->_length;
}

void dbTechLayerCutClassSubRule::setNumCuts(int _num_cuts)
{
  _dbTechLayerCutClassSubRule* obj = (_dbTechLayerCutClassSubRule*) this;

  obj->_num_cuts = _num_cuts;
}

int dbTechLayerCutClassSubRule::getNumCuts() const
{
  _dbTechLayerCutClassSubRule* obj = (_dbTechLayerCutClassSubRule*) this;
  return obj->_num_cuts;
}

void dbTechLayerCutClassSubRule::setLengthValid(bool _length_valid)
{
  _dbTechLayerCutClassSubRule* obj = (_dbTechLayerCutClassSubRule*) this;

  obj->_flags._length_valid = _length_valid;
}

bool dbTechLayerCutClassSubRule::isLengthValid() const
{
  _dbTechLayerCutClassSubRule* obj = (_dbTechLayerCutClassSubRule*) this;

  return obj->_flags._length_valid;
}

void dbTechLayerCutClassSubRule::setCutsValid(bool _cuts_valid)
{
  _dbTechLayerCutClassSubRule* obj = (_dbTechLayerCutClassSubRule*) this;

  obj->_flags._cuts_valid = _cuts_valid;
}

bool dbTechLayerCutClassSubRule::isCutsValid() const
{
  _dbTechLayerCutClassSubRule* obj = (_dbTechLayerCutClassSubRule*) this;

  return obj->_flags._cuts_valid;
}

// User Code Begin dbTechLayerCutClassSubRulePublicMethods

dbTechLayerCutClassSubRule* dbTechLayerCutClassSubRule::create(
    dbTechLayerCutClassRule* parent,
    const char*              name)
{
  _dbTechLayerCutClassRule*    _parent = (_dbTechLayerCutClassRule*) parent;
  _dbTechLayerCutClassSubRule* newrule
      = _parent->_techlayercutclasssubrule_tbl->create();
  newrule->_class_name = strdup(name);
  return ((dbTechLayerCutClassSubRule*) newrule);
}

dbTechLayerCutClassSubRule*
dbTechLayerCutClassSubRule::getTechLayerCutClassSubRule(
    dbTechLayerCutClassRule* parent,
    uint                     dbid)
{
  _dbTechLayerCutClassRule* _parent = (_dbTechLayerCutClassRule*) parent;
  return (dbTechLayerCutClassSubRule*)
      _parent->_techlayercutclasssubrule_tbl->getPtr(dbid);
}
void dbTechLayerCutClassSubRule::destroy(dbTechLayerCutClassSubRule* rule)
{
  _dbTechLayerCutClassRule* _parent
      = (_dbTechLayerCutClassRule*) rule->getImpl()->getOwner();
  dbProperty::destroyProperties(rule);
  _parent->_techlayercutclasssubrule_tbl->destroy(
      (_dbTechLayerCutClassSubRule*) rule);
}

// User Code End dbTechLayerCutClassSubRulePublicMethods
}  // namespace odb
   // Generator Code End 1