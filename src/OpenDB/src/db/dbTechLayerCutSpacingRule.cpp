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
#include "dbTechLayerCutSpacingRule.h"

#include "db.h"
#include "dbDatabase.h"
#include "dbDiff.hpp"
#include "dbSet.h"
#include "dbTable.h"
#include "dbTable.hpp"
#include "dbTechLayer.h"
#include "dbTechLayerCutSpacingSubRule.h"
// User Code Begin includes
// User Code End includes
namespace odb {

// User Code Begin definitions
// User Code End definitions
template class dbTable<_dbTechLayerCutSpacingRule>;

bool _dbTechLayerCutSpacingRule::operator==(
    const _dbTechLayerCutSpacingRule& rhs) const
{
  if (*_techlayercutspacingsubrule_tbl != *rhs._techlayercutspacingsubrule_tbl)
    return false;

  // User Code Begin ==
  // User Code End ==
  return true;
}
bool _dbTechLayerCutSpacingRule::operator<(
    const _dbTechLayerCutSpacingRule& rhs) const
{
  // User Code Begin <
  // User Code End <
  return true;
}
void _dbTechLayerCutSpacingRule::differences(
    dbDiff&                           diff,
    const char*                       field,
    const _dbTechLayerCutSpacingRule& rhs) const
{
  DIFF_BEGIN

  DIFF_TABLE(_techlayercutspacingsubrule_tbl);
  // User Code Begin differences
  // User Code End differences
  DIFF_END
}
void _dbTechLayerCutSpacingRule::out(dbDiff&     diff,
                                     char        side,
                                     const char* field) const
{
  DIFF_OUT_BEGIN
  DIFF_OUT_TABLE(_techlayercutspacingsubrule_tbl);

  // User Code Begin out
  // User Code End out
  DIFF_END
}
_dbTechLayerCutSpacingRule::_dbTechLayerCutSpacingRule(_dbDatabase* db)
{
  _techlayercutspacingsubrule_tbl = new dbTable<_dbTechLayerCutSpacingSubRule>(
      db,
      this,
      (GetObjTbl_t) &_dbTechLayerCutSpacingRule::getObjectTable,
      dbTechLayerCutSpacingSubRuleObj);
  ZALLOCATED(_techlayercutspacingsubrule_tbl);
  // User Code Begin constructor
  // User Code End constructor
}
_dbTechLayerCutSpacingRule::_dbTechLayerCutSpacingRule(
    _dbDatabase*                      db,
    const _dbTechLayerCutSpacingRule& r)
{
  _techlayercutspacingsubrule_tbl = new dbTable<_dbTechLayerCutSpacingSubRule>(
      db, this, *r._techlayercutspacingsubrule_tbl);
  ZALLOCATED(_techlayercutspacingsubrule_tbl);
  // User Code Begin CopyConstructor
  // User Code End CopyConstructor
}

dbIStream& operator>>(dbIStream& stream, _dbTechLayerCutSpacingRule& obj)
{
  stream >> *obj._techlayercutspacingsubrule_tbl;
  // User Code Begin >>
  // User Code End >>
  return stream;
}
dbOStream& operator<<(dbOStream& stream, const _dbTechLayerCutSpacingRule& obj)
{
  stream << *obj._techlayercutspacingsubrule_tbl;
  // User Code Begin <<
  // User Code End <<
  return stream;
}

dbObjectTable* _dbTechLayerCutSpacingRule::getObjectTable(dbObjectType type)
{
  switch (type) {
    case dbTechLayerCutSpacingSubRuleObj:
      return _techlayercutspacingsubrule_tbl;
      // User Code Begin getObjectTable
    // User Code End getObjectTable
    default:
      break;
  }
  return getTable()->getObjectTable(type);
}
_dbTechLayerCutSpacingRule::~_dbTechLayerCutSpacingRule()
{
  delete _techlayercutspacingsubrule_tbl;
  // User Code Begin Destructor
  // User Code End Destructor
}
////////////////////////////////////////////////////////////////////
//
// dbTechLayerCutSpacingRule - Methods
//
////////////////////////////////////////////////////////////////////

dbSet<dbTechLayerCutSpacingSubRule>
dbTechLayerCutSpacingRule::getTechLayerCutSpacingSubRules() const
{
  _dbTechLayerCutSpacingRule* obj = (_dbTechLayerCutSpacingRule*) this;
  return dbSet<dbTechLayerCutSpacingSubRule>(
      obj, obj->_techlayercutspacingsubrule_tbl);
}

// User Code Begin dbTechLayerCutSpacingRulePublicMethods
dbTechLayerCutSpacingRule* dbTechLayerCutSpacingRule::create(
    dbTechLayer* _layer)
{
  _dbTechLayer*               layer   = (_dbTechLayer*) _layer;
  _dbTechLayerCutSpacingRule* newrule = layer->_cut_spacing_rules_tbl->create();
  return ((dbTechLayerCutSpacingRule*) newrule);
}

dbTechLayerCutSpacingRule*
dbTechLayerCutSpacingRule::getTechLayerCutSpacingRule(dbTechLayer* inly,
                                                      uint         dbid)
{
  _dbTechLayer* layer = (_dbTechLayer*) inly;
  return (dbTechLayerCutSpacingRule*) layer->_cut_spacing_rules_tbl->getPtr(
      dbid);
}
void dbTechLayerCutSpacingRule::destroy(dbTechLayerCutSpacingRule* rule)
{
  for (auto subrule : rule->getTechLayerCutSpacingSubRules()) {
    dbTechLayerCutSpacingSubRule::destroy(subrule);
  }
  _dbTechLayer* layer = (_dbTechLayer*) rule->getImpl()->getOwner();
  dbProperty::destroyProperties(rule);
  layer->_cut_spacing_rules_tbl->destroy((_dbTechLayerCutSpacingRule*) rule);
}
// User Code End dbTechLayerCutSpacingRulePublicMethods
}  // namespace odb
   // Generator Code End 1