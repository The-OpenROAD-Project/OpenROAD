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
#include "dbTechLayerRectOnlyRule.h"

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
template class dbTable<_dbTechLayerRectOnlyRule>;

bool _dbTechLayerRectOnlyRule::operator==(
    const _dbTechLayerRectOnlyRule& rhs) const
{
  if (_flags._except_non_core_pins != rhs._flags._except_non_core_pins)
    return false;

  // User Code Begin ==
  // User Code End ==
  return true;
}
bool _dbTechLayerRectOnlyRule::operator<(
    const _dbTechLayerRectOnlyRule& rhs) const
{
  // User Code Begin <
  // User Code End <
  return true;
}
void _dbTechLayerRectOnlyRule::differences(
    dbDiff&                         diff,
    const char*                     field,
    const _dbTechLayerRectOnlyRule& rhs) const
{
  DIFF_BEGIN

  DIFF_FIELD(_flags._except_non_core_pins);
  // User Code Begin differences
  // User Code End differences
  DIFF_END
}
void _dbTechLayerRectOnlyRule::out(dbDiff&     diff,
                                   char        side,
                                   const char* field) const
{
  DIFF_OUT_BEGIN
  DIFF_OUT_FIELD(_flags._except_non_core_pins);

  // User Code Begin out
  // User Code End out
  DIFF_END
}
_dbTechLayerRectOnlyRule::_dbTechLayerRectOnlyRule(_dbDatabase* db)
{
  uint* _flags_bit_field = (uint*) &_flags;
  *_flags_bit_field      = 0;
  // User Code Begin constructor
  // User Code End constructor
}
_dbTechLayerRectOnlyRule::_dbTechLayerRectOnlyRule(
    _dbDatabase*                    db,
    const _dbTechLayerRectOnlyRule& r)
{
  _flags._except_non_core_pins = r._flags._except_non_core_pins;
  _flags._spare_bits           = r._flags._spare_bits;
  // User Code Begin CopyConstructor
  // User Code End CopyConstructor
}

dbIStream& operator>>(dbIStream& stream, _dbTechLayerRectOnlyRule& obj)
{
  uint* _flags_bit_field = (uint*) &obj._flags;
  stream >> *_flags_bit_field;
  // User Code Begin >>
  // User Code End >>
  return stream;
}
dbOStream& operator<<(dbOStream& stream, const _dbTechLayerRectOnlyRule& obj)
{
  uint* _flags_bit_field = (uint*) &obj._flags;
  stream << *_flags_bit_field;
  // User Code Begin <<
  // User Code End <<
  return stream;
}

_dbTechLayerRectOnlyRule::~_dbTechLayerRectOnlyRule()
{
  // User Code Begin Destructor
  // User Code End Destructor
}
////////////////////////////////////////////////////////////////////
//
// dbTechLayerRectOnlyRule - Methods
//
////////////////////////////////////////////////////////////////////

void dbTechLayerRectOnlyRule::setExceptNonCorePins(bool _except_non_core_pins)
{
  _dbTechLayerRectOnlyRule* obj = (_dbTechLayerRectOnlyRule*) this;

  obj->_flags._except_non_core_pins = _except_non_core_pins;
}

bool dbTechLayerRectOnlyRule::isExceptNonCorePins() const
{
  _dbTechLayerRectOnlyRule* obj = (_dbTechLayerRectOnlyRule*) this;

  return obj->_flags._except_non_core_pins;
}

// User Code Begin dbTechLayerRectOnlyRulePublicMethods
dbTechLayerRectOnlyRule* dbTechLayerRectOnlyRule::create(dbTechLayer* _layer)
{
  _dbTechLayer*             layer   = (_dbTechLayer*) _layer;
  _dbTechLayerRectOnlyRule* newrule = layer->_rect_only_rules_tbl->create();
  return ((dbTechLayerRectOnlyRule*) newrule);
}

dbTechLayerRectOnlyRule* dbTechLayerRectOnlyRule::getTechLayerRectOnlyRule(
    dbTechLayer* inly,
    uint         dbid)
{
  _dbTechLayer* layer = (_dbTechLayer*) inly;
  return (dbTechLayerRectOnlyRule*) layer->_rect_only_rules_tbl->getPtr(dbid);
}
void dbTechLayerRectOnlyRule::destroy(dbTechLayerRectOnlyRule* rule)
{
  _dbTechLayer* layer = (_dbTechLayer*) rule->getImpl()->getOwner();
  dbProperty::destroyProperties(rule);
  layer->_rect_only_rules_tbl->destroy((_dbTechLayerRectOnlyRule*) rule);
}
// User Code End dbTechLayerRectOnlyRulePublicMethods
}  // namespace odb
   // Generator Code End 1