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

// Generator Code Begin Cpp
#include "dbTechLayerEolExtensionRule.h"

#include "db.h"
#include "dbDatabase.h"
#include "dbDiff.hpp"
#include "dbTable.h"
#include "dbTable.hpp"
#include "dbTechLayer.h"
// User Code Begin Includes
// User Code End Includes
namespace odb {

template class dbTable<_dbTechLayerEolExtensionRule>;

bool _dbTechLayerEolExtensionRule::operator==(
    const _dbTechLayerEolExtensionRule& rhs) const
{
  if (flags_.parallel_only_ != rhs.flags_.parallel_only_)
    return false;

  if (spacing_ != rhs.spacing_)
    return false;

  // User Code Begin ==
  // User Code End ==
  return true;
}
bool _dbTechLayerEolExtensionRule::operator<(
    const _dbTechLayerEolExtensionRule& rhs) const
{
  // User Code Begin <
  if (spacing_ >= rhs.spacing_)
    return false;
  // User Code End <
  return true;
}
void _dbTechLayerEolExtensionRule::differences(
    dbDiff& diff,
    const char* field,
    const _dbTechLayerEolExtensionRule& rhs) const
{
  DIFF_BEGIN

  DIFF_FIELD(flags_.parallel_only_);
  DIFF_FIELD(spacing_);
  // User Code Begin Differences
  // User Code End Differences
  DIFF_END
}
void _dbTechLayerEolExtensionRule::out(dbDiff& diff,
                                       char side,
                                       const char* field) const
{
  DIFF_OUT_BEGIN
  DIFF_OUT_FIELD(flags_.parallel_only_);
  DIFF_OUT_FIELD(spacing_);

  // User Code Begin Out
  // User Code End Out
  DIFF_END
}
_dbTechLayerEolExtensionRule::_dbTechLayerEolExtensionRule(_dbDatabase* db)
{
  uint32_t* flags__bit_field = (uint32_t*) &flags_;
  *flags__bit_field = 0;
  spacing_ = 0;
  // User Code Begin Constructor
  // User Code End Constructor
}
_dbTechLayerEolExtensionRule::_dbTechLayerEolExtensionRule(
    _dbDatabase* db,
    const _dbTechLayerEolExtensionRule& r)
{
  flags_.parallel_only_ = r.flags_.parallel_only_;
  flags_.spare_bits_ = r.flags_.spare_bits_;
  spacing_ = r.spacing_;
  // User Code Begin CopyConstructor
  // User Code End CopyConstructor
}

dbIStream& operator>>(dbIStream& stream, _dbTechLayerEolExtensionRule& obj)
{
  uint32_t* flags__bit_field = (uint32_t*) &obj.flags_;
  stream >> *flags__bit_field;
  stream >> obj.spacing_;
  stream >> obj.extension_tbl_;
  // User Code Begin >>
  // User Code End >>
  return stream;
}
dbOStream& operator<<(dbOStream& stream,
                      const _dbTechLayerEolExtensionRule& obj)
{
  uint32_t* flags__bit_field = (uint32_t*) &obj.flags_;
  stream << *flags__bit_field;
  stream << obj.spacing_;
  stream << obj.extension_tbl_;
  // User Code Begin <<
  // User Code End <<
  return stream;
}

_dbTechLayerEolExtensionRule::~_dbTechLayerEolExtensionRule()
{
  // User Code Begin Destructor
  // User Code End Destructor
}

// User Code Begin PrivateMethods
// User Code End PrivateMethods

////////////////////////////////////////////////////////////////////
//
// dbTechLayerEolExtensionRule - Methods
//
////////////////////////////////////////////////////////////////////

void dbTechLayerEolExtensionRule::setSpacing(int spacing)
{
  _dbTechLayerEolExtensionRule* obj = (_dbTechLayerEolExtensionRule*) this;

  obj->spacing_ = spacing;
}

int dbTechLayerEolExtensionRule::getSpacing() const
{
  _dbTechLayerEolExtensionRule* obj = (_dbTechLayerEolExtensionRule*) this;
  return obj->spacing_;
}

void dbTechLayerEolExtensionRule::getExtensionTable(
    std::vector<std::pair<int, int>>& tbl) const
{
  _dbTechLayerEolExtensionRule* obj = (_dbTechLayerEolExtensionRule*) this;
  tbl = obj->extension_tbl_;
}

void dbTechLayerEolExtensionRule::setParallelOnly(bool parallel_only)
{
  _dbTechLayerEolExtensionRule* obj = (_dbTechLayerEolExtensionRule*) this;

  obj->flags_.parallel_only_ = parallel_only;
}

bool dbTechLayerEolExtensionRule::isParallelOnly() const
{
  _dbTechLayerEolExtensionRule* obj = (_dbTechLayerEolExtensionRule*) this;

  return obj->flags_.parallel_only_;
}

// User Code Begin dbTechLayerEolExtensionRulePublicMethods

void dbTechLayerEolExtensionRule::addEntry(int eol, int ext)
{
  _dbTechLayerEolExtensionRule* obj = (_dbTechLayerEolExtensionRule*) this;
  obj->extension_tbl_.push_back({eol, ext});
}

dbTechLayerEolExtensionRule* dbTechLayerEolExtensionRule::create(
    dbTechLayer* _layer)
{
  _dbTechLayer* layer = (_dbTechLayer*) _layer;
  _dbTechLayerEolExtensionRule* newrule = layer->eol_ext_rules_tbl_->create();
  return ((dbTechLayerEolExtensionRule*) newrule);
}

dbTechLayerEolExtensionRule*
dbTechLayerEolExtensionRule::getTechLayerEolExtensionRule(dbTechLayer* inly,
                                                          uint dbid)
{
  _dbTechLayer* layer = (_dbTechLayer*) inly;
  return (dbTechLayerEolExtensionRule*) layer->eol_ext_rules_tbl_->getPtr(dbid);
}
void dbTechLayerEolExtensionRule::destroy(dbTechLayerEolExtensionRule* rule)
{
  _dbTechLayer* layer = (_dbTechLayer*) rule->getImpl()->getOwner();
  dbProperty::destroyProperties(rule);
  layer->eol_ext_rules_tbl_->destroy((_dbTechLayerEolExtensionRule*) rule);
}

// User Code End dbTechLayerEolExtensionRulePublicMethods
}  // namespace odb
   // Generator Code End Cpp