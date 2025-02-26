///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2022, The Regents of the University of California
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
#include "dbTechLayerMaxSpacingRule.h"

#include "dbDatabase.h"
#include "dbTable.h"
#include "dbTable.hpp"
#include "dbTechLayer.h"
#include "odb/db.h"
namespace odb {
template class dbTable<_dbTechLayerMaxSpacingRule>;

bool _dbTechLayerMaxSpacingRule::operator==(
    const _dbTechLayerMaxSpacingRule& rhs) const
{
  if (cut_class_ != rhs.cut_class_) {
    return false;
  }
  if (max_spacing_ != rhs.max_spacing_) {
    return false;
  }

  return true;
}

bool _dbTechLayerMaxSpacingRule::operator<(
    const _dbTechLayerMaxSpacingRule& rhs) const
{
  return true;
}

_dbTechLayerMaxSpacingRule::_dbTechLayerMaxSpacingRule(_dbDatabase* db)
{
  max_spacing_ = 0;
}

dbIStream& operator>>(dbIStream& stream, _dbTechLayerMaxSpacingRule& obj)
{
  stream >> obj.cut_class_;
  stream >> obj.max_spacing_;
  return stream;
}

dbOStream& operator<<(dbOStream& stream, const _dbTechLayerMaxSpacingRule& obj)
{
  stream << obj.cut_class_;
  stream << obj.max_spacing_;
  return stream;
}

void _dbTechLayerMaxSpacingRule::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);

  // User Code Begin collectMemInfo
  info.children_["cut_class"].add(cut_class_);
  // User Code End collectMemInfo
}

////////////////////////////////////////////////////////////////////
//
// dbTechLayerMaxSpacingRule - Methods
//
////////////////////////////////////////////////////////////////////

void dbTechLayerMaxSpacingRule::setCutClass(const std::string& cut_class)
{
  _dbTechLayerMaxSpacingRule* obj = (_dbTechLayerMaxSpacingRule*) this;

  obj->cut_class_ = cut_class;
}

std::string dbTechLayerMaxSpacingRule::getCutClass() const
{
  _dbTechLayerMaxSpacingRule* obj = (_dbTechLayerMaxSpacingRule*) this;
  return obj->cut_class_;
}

void dbTechLayerMaxSpacingRule::setMaxSpacing(int max_spacing)
{
  _dbTechLayerMaxSpacingRule* obj = (_dbTechLayerMaxSpacingRule*) this;

  obj->max_spacing_ = max_spacing;
}

int dbTechLayerMaxSpacingRule::getMaxSpacing() const
{
  _dbTechLayerMaxSpacingRule* obj = (_dbTechLayerMaxSpacingRule*) this;
  return obj->max_spacing_;
}

// User Code Begin dbTechLayerMaxSpacingRulePublicMethods

bool dbTechLayerMaxSpacingRule::hasCutClass() const
{
  _dbTechLayerMaxSpacingRule* obj = (_dbTechLayerMaxSpacingRule*) this;
  return (!obj->cut_class_.empty());
}
dbTechLayerMaxSpacingRule* dbTechLayerMaxSpacingRule::create(
    dbTechLayer* _layer)
{
  _dbTechLayer* layer = (_dbTechLayer*) _layer;
  _dbTechLayerMaxSpacingRule* newrule = layer->max_spacing_rules_tbl_->create();
  return ((dbTechLayerMaxSpacingRule*) newrule);
}

void dbTechLayerMaxSpacingRule::destroy(dbTechLayerMaxSpacingRule* rule)
{
  _dbTechLayer* layer = (_dbTechLayer*) rule->getImpl()->getOwner();
  dbProperty::destroyProperties(rule);
  layer->max_spacing_rules_tbl_->destroy((_dbTechLayerMaxSpacingRule*) rule);
}
// User Code End dbTechLayerMaxSpacingRulePublicMethods
}  // namespace odb
   // Generator Code End Cpp
