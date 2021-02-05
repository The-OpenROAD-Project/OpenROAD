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
#include "dbTechLayerCutSpacingTableOrthSubRule.h"

#include "db.h"
#include "dbDatabase.h"
#include "dbDiff.hpp"
#include "dbTable.h"
#include "dbTable.hpp"
#include "dbTechLayerCutSpacingRule.h"
// User Code Begin includes
#include "dbTechLayerCutSpacingTableRule.h"
// User Code End includes
namespace odb {

// User Code Begin definitions
// User Code End definitions
template class dbTable<_dbTechLayerCutSpacingTableOrthSubRule>;

bool _dbTechLayerCutSpacingTableOrthSubRule::operator==(
    const _dbTechLayerCutSpacingTableOrthSubRule& rhs) const
{
  // User Code Begin ==
  // User Code End ==
  return true;
}
bool _dbTechLayerCutSpacingTableOrthSubRule::operator<(
    const _dbTechLayerCutSpacingTableOrthSubRule& rhs) const
{
  // User Code Begin <
  // User Code End <
  return true;
}
void _dbTechLayerCutSpacingTableOrthSubRule::differences(
    dbDiff&                                       diff,
    const char*                                   field,
    const _dbTechLayerCutSpacingTableOrthSubRule& rhs) const
{
  DIFF_BEGIN

  // User Code Begin differences
  // User Code End differences
  DIFF_END
}
void _dbTechLayerCutSpacingTableOrthSubRule::out(dbDiff&     diff,
                                                 char        side,
                                                 const char* field) const {
    DIFF_OUT_BEGIN

        // User Code Begin out
        // User Code End out
        DIFF_END} _dbTechLayerCutSpacingTableOrthSubRule::
    _dbTechLayerCutSpacingTableOrthSubRule(_dbDatabase* db)
{
  // User Code Begin constructor
  // User Code End constructor
}
_dbTechLayerCutSpacingTableOrthSubRule::_dbTechLayerCutSpacingTableOrthSubRule(
    _dbDatabase*                                  db,
    const _dbTechLayerCutSpacingTableOrthSubRule& r)
{
  // User Code Begin CopyConstructor
  // User Code End CopyConstructor
}

dbIStream& operator>>(dbIStream&                              stream,
                      _dbTechLayerCutSpacingTableOrthSubRule& obj)
{
  stream >> obj._spacing_tbl;
  // User Code Begin >>
  // User Code End >>
  return stream;
}
dbOStream& operator<<(dbOStream&                                    stream,
                      const _dbTechLayerCutSpacingTableOrthSubRule& obj)
{
  stream << obj._spacing_tbl;
  // User Code Begin <<
  // User Code End <<
  return stream;
}

_dbTechLayerCutSpacingTableOrthSubRule::
    ~_dbTechLayerCutSpacingTableOrthSubRule()
{
  // User Code Begin Destructor
  // User Code End Destructor
}
////////////////////////////////////////////////////////////////////
//
// dbTechLayerCutSpacingTableOrthSubRule - Methods
//
////////////////////////////////////////////////////////////////////

void dbTechLayerCutSpacingTableOrthSubRule::getSpacingTable(
    std::vector<std::pair<int, int>>& tbl) const
{
  _dbTechLayerCutSpacingTableOrthSubRule* obj
      = (_dbTechLayerCutSpacingTableOrthSubRule*) this;
  tbl = obj->_spacing_tbl;
}

// User Code Begin dbTechLayerCutSpacingTableOrthSubRulePublicMethods
void dbTechLayerCutSpacingTableOrthSubRule::setSpacingTable(
    std::vector<std::pair<int, int>> tbl)
{
  _dbTechLayerCutSpacingTableOrthSubRule* obj
      = (_dbTechLayerCutSpacingTableOrthSubRule*) this;
  obj->_spacing_tbl = tbl;
}

dbTechLayerCutSpacingTableOrthSubRule*
dbTechLayerCutSpacingTableOrthSubRule::create(
    dbTechLayerCutSpacingTableRule* parent)
{
  _dbTechLayerCutSpacingTableRule* _parent
      = (_dbTechLayerCutSpacingTableRule*) parent;
  _dbTechLayerCutSpacingTableOrthSubRule* newrule
      = _parent->_techlayercutspacingtableorthsubrule_tbl->create();
  return ((dbTechLayerCutSpacingTableOrthSubRule*) newrule);
}

dbTechLayerCutSpacingTableOrthSubRule*
dbTechLayerCutSpacingTableOrthSubRule::getTechLayerCutSpacingTableOrthSubRule(
    dbTechLayerCutSpacingTableRule* parent,
    uint                            dbid)
{
  _dbTechLayerCutSpacingTableRule* _parent
      = (_dbTechLayerCutSpacingTableRule*) parent;
  return (dbTechLayerCutSpacingTableOrthSubRule*)
      _parent->_techlayercutspacingtableorthsubrule_tbl->getPtr(dbid);
}
void dbTechLayerCutSpacingTableOrthSubRule::destroy(
    dbTechLayerCutSpacingTableOrthSubRule* rule)
{
  _dbTechLayerCutSpacingTableRule* _parent
      = (_dbTechLayerCutSpacingTableRule*) rule->getImpl()->getOwner();
  dbProperty::destroyProperties(rule);
  _parent->_techlayercutspacingtableorthsubrule_tbl->destroy(
      (_dbTechLayerCutSpacingTableOrthSubRule*) rule);
}

// User Code End dbTechLayerCutSpacingTableOrthSubRulePublicMethods
}  // namespace odb
   // Generator Code End 1