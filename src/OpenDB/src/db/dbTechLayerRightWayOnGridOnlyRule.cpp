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
#include "dbTechLayerRightWayOnGridOnlyRule.h"

#include "db.h"
#include "dbDatabase.h"
#include "dbDiff.hpp"
#include "dbTable.h"
#include "dbTable.hpp"
#include "dbTechLayer.h"
// User Code Begin includes
// User Code End includes
namespace odb {

// User Code Begin definitions
// User Code End definitions
template class dbTable<_dbTechLayerRightWayOnGridOnlyRule>;

bool _dbTechLayerRightWayOnGridOnlyRule::operator==(
    const _dbTechLayerRightWayOnGridOnlyRule& rhs) const
{
  if (_flags._check_mask != rhs._flags._check_mask)
    return false;

  // User Code Begin ==
  // User Code End ==
  return true;
}
bool _dbTechLayerRightWayOnGridOnlyRule::operator<(
    const _dbTechLayerRightWayOnGridOnlyRule& rhs) const
{
  // User Code Begin <
  // User Code End <
  return true;
}
void _dbTechLayerRightWayOnGridOnlyRule::differences(
    dbDiff&                                   diff,
    const char*                               field,
    const _dbTechLayerRightWayOnGridOnlyRule& rhs) const
{
  DIFF_BEGIN

  DIFF_FIELD(_flags._check_mask);
  // User Code Begin differences
  // User Code End differences
  DIFF_END
}
void _dbTechLayerRightWayOnGridOnlyRule::out(dbDiff&     diff,
                                             char        side,
                                             const char* field) const
{
  DIFF_OUT_BEGIN
  DIFF_OUT_FIELD(_flags._check_mask);

  // User Code Begin out
  // User Code End out
  DIFF_END
}
_dbTechLayerRightWayOnGridOnlyRule::_dbTechLayerRightWayOnGridOnlyRule(
    _dbDatabase* db)
{
  uint* _flags_bit_field = (uint*) &_flags;
  *_flags_bit_field      = 0;
  // User Code Begin constructor
  // User Code End constructor
}
_dbTechLayerRightWayOnGridOnlyRule::_dbTechLayerRightWayOnGridOnlyRule(
    _dbDatabase*                              db,
    const _dbTechLayerRightWayOnGridOnlyRule& r)
{
  _flags._check_mask = r._flags._check_mask;
  _flags._spare_bits = r._flags._spare_bits;
  // User Code Begin CopyConstructor
  // User Code End CopyConstructor
}

dbIStream& operator>>(dbIStream&                          stream,
                      _dbTechLayerRightWayOnGridOnlyRule& obj)
{
  uint* _flags_bit_field = (uint*) &obj._flags;
  stream >> *_flags_bit_field;
  // User Code Begin >>
  // User Code End >>
  return stream;
}
dbOStream& operator<<(dbOStream&                                stream,
                      const _dbTechLayerRightWayOnGridOnlyRule& obj)
{
  uint* _flags_bit_field = (uint*) &obj._flags;
  stream << *_flags_bit_field;
  // User Code Begin <<
  // User Code End <<
  return stream;
}

_dbTechLayerRightWayOnGridOnlyRule::~_dbTechLayerRightWayOnGridOnlyRule()
{
  // User Code Begin Destructor
  // User Code End Destructor
}
////////////////////////////////////////////////////////////////////
//
// dbTechLayerRightWayOnGridOnlyRule - Methods
//
////////////////////////////////////////////////////////////////////

void dbTechLayerRightWayOnGridOnlyRule::setCheckMask(bool _check_mask)
{
  _dbTechLayerRightWayOnGridOnlyRule* obj
      = (_dbTechLayerRightWayOnGridOnlyRule*) this;

  obj->_flags._check_mask = _check_mask;
}

bool dbTechLayerRightWayOnGridOnlyRule::isCheckMask() const
{
  _dbTechLayerRightWayOnGridOnlyRule* obj
      = (_dbTechLayerRightWayOnGridOnlyRule*) this;

  return obj->_flags._check_mask;
}

// User Code Begin dbTechLayerRightWayOnGridOnlyRulePublicMethods
dbTechLayerRightWayOnGridOnlyRule* dbTechLayerRightWayOnGridOnlyRule::create(
    dbTechLayer* _layer)
{
  _dbTechLayer*                       layer = (_dbTechLayer*) _layer;
  _dbTechLayerRightWayOnGridOnlyRule* newrule
      = layer->_rwogo_rules_tbl->create();
  return ((dbTechLayerRightWayOnGridOnlyRule*) newrule);
}

dbTechLayerRightWayOnGridOnlyRule*
dbTechLayerRightWayOnGridOnlyRule::getTechLayerRightWayOnGridOnlyRule(
    dbTechLayer* inly,
    uint         dbid)
{
  _dbTechLayer* layer = (_dbTechLayer*) inly;
  return (dbTechLayerRightWayOnGridOnlyRule*) layer->_rwogo_rules_tbl->getPtr(
      dbid);
}
void dbTechLayerRightWayOnGridOnlyRule::destroy(
    dbTechLayerRightWayOnGridOnlyRule* rule)
{
  _dbTechLayer* layer = (_dbTechLayer*) rule->getImpl()->getOwner();
  dbProperty::destroyProperties(rule);
  layer->_rwogo_rules_tbl->destroy((_dbTechLayerRightWayOnGridOnlyRule*) rule);
}
// User Code End dbTechLayerRightWayOnGridOnlyRulePublicMethods
}  // namespace odb
   // Generator Code End 1