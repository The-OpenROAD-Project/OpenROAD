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
#include "dbGuide.h"

#include "db.h"
#include "dbDatabase.h"
#include "dbDiff.hpp"
#include "dbNet.h"
#include "dbTable.h"
#include "dbTable.hpp"
#include "dbTechLayer.h"
// User Code Begin Includes
#include "dbBlock.h"
// User Code End Includes
namespace odb {
template class dbTable<_dbGuide>;

bool _dbGuide::operator==(const _dbGuide& rhs) const
{
  if (net_ != rhs.net_) {
    return false;
  }
  if (box_ != rhs.box_) {
    return false;
  }
  if (layer_ != rhs.layer_) {
    return false;
  }
  if (guide_next_ != rhs.guide_next_) {
    return false;
  }

  return true;
}

bool _dbGuide::operator<(const _dbGuide& rhs) const
{
  return true;
}

void _dbGuide::differences(dbDiff& diff,
                           const char* field,
                           const _dbGuide& rhs) const
{
  DIFF_BEGIN
  DIFF_FIELD(net_);
  DIFF_FIELD(box_);
  DIFF_FIELD(layer_);
  DIFF_FIELD(guide_next_);
  DIFF_END
}

void _dbGuide::out(dbDiff& diff, char side, const char* field) const
{
  DIFF_OUT_BEGIN
  DIFF_OUT_FIELD(net_);
  DIFF_OUT_FIELD(box_);
  DIFF_OUT_FIELD(layer_);
  DIFF_OUT_FIELD(guide_next_);

  DIFF_END
}

_dbGuide::_dbGuide(_dbDatabase* db)
{
}

_dbGuide::_dbGuide(_dbDatabase* db, const _dbGuide& r)
{
  net_ = r.net_;
  box_ = r.box_;
  layer_ = r.layer_;
  guide_next_ = r.guide_next_;
}

dbIStream& operator>>(dbIStream& stream, _dbGuide& obj)
{
  stream >> obj.net_;
  stream >> obj.box_;
  stream >> obj.layer_;
  stream >> obj.guide_next_;
  return stream;
}

dbOStream& operator<<(dbOStream& stream, const _dbGuide& obj)
{
  stream << obj.net_;
  stream << obj.box_;
  stream << obj.layer_;
  stream << obj.guide_next_;
  return stream;
}

////////////////////////////////////////////////////////////////////
//
// dbGuide - Methods
//
////////////////////////////////////////////////////////////////////

Rect dbGuide::getBox() const
{
  _dbGuide* obj = (_dbGuide*) this;
  return obj->box_;
}

// User Code Begin dbGuidePublicMethods

dbTechLayer* dbGuide::getLayer() const
{
  _dbGuide* obj = (_dbGuide*) this;
  auto tech = getDb()->getTech();
  return odb::dbTechLayer::getTechLayer(tech, obj->layer_);
}

dbNet* dbGuide::getNet() const
{
  _dbGuide* obj = (_dbGuide*) this;
  _dbBlock* block = (_dbBlock*) obj->getOwner();
  return (dbNet*) block->_net_tbl->getPtr(obj->net_);
}

dbGuide* dbGuide::create(dbNet* net, dbTechLayer* layer, Rect box)
{
  _dbNet* owner = (_dbNet*) net;
  _dbBlock* block = (_dbBlock*) owner->getOwner();
  _dbGuide* guide = block->_guide_tbl->create();
  guide->layer_ = layer->getImpl()->getOID();
  guide->box_ = box;
  guide->net_ = owner->getId();
  guide->guide_next_ = owner->guides_;
  owner->guides_ = guide->getOID();
  return (dbGuide*) guide;
}

dbGuide* dbGuide::getGuide(dbBlock* block, uint dbid)
{
  _dbBlock* owner = (_dbBlock*) block;
  return (dbGuide*) owner->_guide_tbl->getPtr(dbid);
}

void dbGuide::destroy(dbGuide* guide)
{
  _dbBlock* block = (_dbBlock*) guide->getImpl()->getOwner();
  _dbNet* net = (_dbNet*) guide->getNet();
  _dbGuide* _guide = (_dbGuide*) guide;

  uint id = _guide->getOID();
  _dbGuide* prev = nullptr;
  uint cur = net->guides_;
  while (cur) {
    _dbGuide* c = block->_guide_tbl->getPtr(cur);
    if (cur == id) {
      if (prev == nullptr)
        net->guides_ = _guide->guide_next_;
      else
        prev->guide_next_ = _guide->guide_next_;
      break;
    }
    prev = c;
    cur = c->guide_next_;
  }

  dbProperty::destroyProperties(guide);
  block->_guide_tbl->destroy((_dbGuide*) guide);
}

// User Code End dbGuidePublicMethods
}  // namespace odb
// Generator Code End Cpp
