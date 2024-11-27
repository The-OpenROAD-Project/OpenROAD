///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2023, The Regents of the University of California
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
#include "dbTechLayerForbiddenSpacingRule.h"

#include "dbDatabase.h"
#include "dbDiff.hpp"
#include "dbTable.h"
#include "dbTable.hpp"
#include "dbTechLayer.h"
#include "odb/db.h"
namespace odb {
template class dbTable<_dbTechLayerForbiddenSpacingRule>;

bool _dbTechLayerForbiddenSpacingRule::operator==(
    const _dbTechLayerForbiddenSpacingRule& rhs) const
{
  if (width_ != rhs.width_) {
    return false;
  }
  if (within_ != rhs.within_) {
    return false;
  }
  if (prl_ != rhs.prl_) {
    return false;
  }
  if (two_edges_ != rhs.two_edges_) {
    return false;
  }

  return true;
}

bool _dbTechLayerForbiddenSpacingRule::operator<(
    const _dbTechLayerForbiddenSpacingRule& rhs) const
{
  return true;
}

void _dbTechLayerForbiddenSpacingRule::differences(
    dbDiff& diff,
    const char* field,
    const _dbTechLayerForbiddenSpacingRule& rhs) const
{
  DIFF_BEGIN
  DIFF_FIELD(width_);
  DIFF_FIELD(within_);
  DIFF_FIELD(prl_);
  DIFF_FIELD(two_edges_);
  DIFF_END
}

void _dbTechLayerForbiddenSpacingRule::out(dbDiff& diff,
                                           char side,
                                           const char* field) const
{
  DIFF_OUT_BEGIN
  DIFF_OUT_FIELD(width_);
  DIFF_OUT_FIELD(within_);
  DIFF_OUT_FIELD(prl_);
  DIFF_OUT_FIELD(two_edges_);

  DIFF_END
}

_dbTechLayerForbiddenSpacingRule::_dbTechLayerForbiddenSpacingRule(
    _dbDatabase* db)
{
  width_ = 0;
  within_ = 0;
  prl_ = 0;
  two_edges_ = 0;
}

_dbTechLayerForbiddenSpacingRule::_dbTechLayerForbiddenSpacingRule(
    _dbDatabase* db,
    const _dbTechLayerForbiddenSpacingRule& r)
{
  width_ = r.width_;
  within_ = r.within_;
  prl_ = r.prl_;
  two_edges_ = r.two_edges_;
}

dbIStream& operator>>(dbIStream& stream, _dbTechLayerForbiddenSpacingRule& obj)
{
  stream >> obj.forbidden_spacing_;
  stream >> obj.width_;
  stream >> obj.within_;
  stream >> obj.prl_;
  stream >> obj.two_edges_;
  return stream;
}

dbOStream& operator<<(dbOStream& stream,
                      const _dbTechLayerForbiddenSpacingRule& obj)
{
  stream << obj.forbidden_spacing_;
  stream << obj.width_;
  stream << obj.within_;
  stream << obj.prl_;
  stream << obj.two_edges_;
  return stream;
}

////////////////////////////////////////////////////////////////////
//
// dbTechLayerForbiddenSpacingRule - Methods
//
////////////////////////////////////////////////////////////////////

void dbTechLayerForbiddenSpacingRule::setForbiddenSpacing(
    const std::pair<int, int>& forbidden_spacing)
{
  _dbTechLayerForbiddenSpacingRule* obj
      = (_dbTechLayerForbiddenSpacingRule*) this;

  obj->forbidden_spacing_ = forbidden_spacing;
}

std::pair<int, int> dbTechLayerForbiddenSpacingRule::getForbiddenSpacing() const
{
  _dbTechLayerForbiddenSpacingRule* obj
      = (_dbTechLayerForbiddenSpacingRule*) this;
  return obj->forbidden_spacing_;
}

void dbTechLayerForbiddenSpacingRule::setWidth(int width)
{
  _dbTechLayerForbiddenSpacingRule* obj
      = (_dbTechLayerForbiddenSpacingRule*) this;

  obj->width_ = width;
}

int dbTechLayerForbiddenSpacingRule::getWidth() const
{
  _dbTechLayerForbiddenSpacingRule* obj
      = (_dbTechLayerForbiddenSpacingRule*) this;
  return obj->width_;
}

void dbTechLayerForbiddenSpacingRule::setWithin(int within)
{
  _dbTechLayerForbiddenSpacingRule* obj
      = (_dbTechLayerForbiddenSpacingRule*) this;

  obj->within_ = within;
}

int dbTechLayerForbiddenSpacingRule::getWithin() const
{
  _dbTechLayerForbiddenSpacingRule* obj
      = (_dbTechLayerForbiddenSpacingRule*) this;
  return obj->within_;
}

void dbTechLayerForbiddenSpacingRule::setPrl(int prl)
{
  _dbTechLayerForbiddenSpacingRule* obj
      = (_dbTechLayerForbiddenSpacingRule*) this;

  obj->prl_ = prl;
}

int dbTechLayerForbiddenSpacingRule::getPrl() const
{
  _dbTechLayerForbiddenSpacingRule* obj
      = (_dbTechLayerForbiddenSpacingRule*) this;
  return obj->prl_;
}

void dbTechLayerForbiddenSpacingRule::setTwoEdges(int two_edges)
{
  _dbTechLayerForbiddenSpacingRule* obj
      = (_dbTechLayerForbiddenSpacingRule*) this;

  obj->two_edges_ = two_edges;
}

int dbTechLayerForbiddenSpacingRule::getTwoEdges() const
{
  _dbTechLayerForbiddenSpacingRule* obj
      = (_dbTechLayerForbiddenSpacingRule*) this;
  return obj->two_edges_;
}

// User Code Begin dbTechLayerForbiddenSpacingRulePublicMethods

bool dbTechLayerForbiddenSpacingRule::hasWidth()
{
  _dbTechLayerForbiddenSpacingRule* obj
      = (_dbTechLayerForbiddenSpacingRule*) this;
  return obj->width_ != 0;
}

bool dbTechLayerForbiddenSpacingRule::hasWithin()
{
  _dbTechLayerForbiddenSpacingRule* obj
      = (_dbTechLayerForbiddenSpacingRule*) this;
  return obj->within_ != 0;
}

bool dbTechLayerForbiddenSpacingRule::hasPrl()
{
  _dbTechLayerForbiddenSpacingRule* obj
      = (_dbTechLayerForbiddenSpacingRule*) this;
  return obj->prl_ != 0;
}

bool dbTechLayerForbiddenSpacingRule::hasTwoEdges()
{
  _dbTechLayerForbiddenSpacingRule* obj
      = (_dbTechLayerForbiddenSpacingRule*) this;
  return obj->two_edges_ != 0;
}

dbTechLayerForbiddenSpacingRule* dbTechLayerForbiddenSpacingRule::create(
    dbTechLayer* _layer)
{
  _dbTechLayer* layer = (_dbTechLayer*) _layer;
  _dbTechLayerForbiddenSpacingRule* newrule
      = layer->forbidden_spacing_rules_tbl_->create();
  return ((dbTechLayerForbiddenSpacingRule*) newrule);
}

void dbTechLayerForbiddenSpacingRule::destroy(
    dbTechLayerForbiddenSpacingRule* rule)
{
  _dbTechLayer* layer = (_dbTechLayer*) rule->getImpl()->getOwner();
  dbProperty::destroyProperties(rule);
  layer->forbidden_spacing_rules_tbl_->destroy(
      (_dbTechLayerForbiddenSpacingRule*) rule);
}

// User Code End dbTechLayerForbiddenSpacingRulePublicMethods
}  // namespace odb
// Generator Code End Cpp
