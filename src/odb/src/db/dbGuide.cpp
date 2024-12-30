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

#include "dbDatabase.h"
#include "dbDiff.hpp"
#include "dbNet.h"
#include "dbTable.h"
#include "dbTable.hpp"
#include "dbTechLayer.h"
#include "odb/db.h"
// User Code Begin Includes
#include "dbBlock.h"
#include "dbJournal.h"
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
  if (via_layer_ != rhs.via_layer_) {
    return false;
  }
  if (guide_next_ != rhs.guide_next_) {
    return false;
  }
  if (is_congested_ != rhs.is_congested_) {
    return false;
  }
  if (is_jumper_ != rhs.is_jumper_) {
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
  DIFF_FIELD(via_layer_);
  DIFF_FIELD(guide_next_);
  DIFF_FIELD(is_congested_);
  DIFF_FIELD(is_jumper_);
  DIFF_END
}

void _dbGuide::out(dbDiff& diff, char side, const char* field) const
{
  DIFF_OUT_BEGIN
  DIFF_OUT_FIELD(net_);
  DIFF_OUT_FIELD(box_);
  DIFF_OUT_FIELD(layer_);
  DIFF_OUT_FIELD(via_layer_);
  DIFF_OUT_FIELD(guide_next_);
  DIFF_OUT_FIELD(is_congested_);
  DIFF_OUT_FIELD(is_jumper_);

  DIFF_END
}

_dbGuide::_dbGuide(_dbDatabase* db)
{
  is_congested_ = false;
  is_jumper_ = false;
}

_dbGuide::_dbGuide(_dbDatabase* db, const _dbGuide& r)
{
  net_ = r.net_;
  box_ = r.box_;
  layer_ = r.layer_;
  via_layer_ = r.via_layer_;
  guide_next_ = r.guide_next_;
  is_congested_ = r.is_congested_;
  is_jumper_ = r.is_jumper_;
}

dbIStream& operator>>(dbIStream& stream, _dbGuide& obj)
{
  stream >> obj.net_;
  stream >> obj.box_;
  stream >> obj.layer_;
  if (obj.getDatabase()->isSchema(db_schema_db_guide_via_layer)) {
    stream >> obj.via_layer_;
  }
  stream >> obj.guide_next_;
  if (obj.getDatabase()->isSchema(db_schema_db_guide_congested)) {
    stream >> obj.is_congested_;
  }
  if (obj.getDatabase()->isSchema(db_schema_has_jumpers)) {
    stream >> obj.is_jumper_;
  }
  return stream;
}

dbOStream& operator<<(dbOStream& stream, const _dbGuide& obj)
{
  stream << obj.net_;
  stream << obj.box_;
  stream << obj.layer_;
  if (obj.getDatabase()->isSchema(db_schema_db_guide_via_layer)) {
    stream << obj.via_layer_;
  }
  stream << obj.guide_next_;
  if (obj.getDatabase()->isSchema(db_schema_db_guide_congested)) {
    stream << obj.is_congested_;
  }
  if (obj.getDatabase()->isSchema(db_schema_has_jumpers)) {
    stream << obj.is_jumper_;
  }
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

dbTechLayer* dbGuide::getViaLayer() const
{
  _dbGuide* obj = (_dbGuide*) this;
  auto tech = getDb()->getTech();
  return odb::dbTechLayer::getTechLayer(tech, obj->via_layer_);
}

bool dbGuide::isCongested() const
{
  _dbGuide* obj = (_dbGuide*) this;
  return obj->is_congested_;
}

dbNet* dbGuide::getNet() const
{
  _dbGuide* obj = (_dbGuide*) this;
  _dbBlock* block = (_dbBlock*) obj->getOwner();
  return (dbNet*) block->_net_tbl->getPtr(obj->net_);
}

dbGuide* dbGuide::create(dbNet* net,
                         dbTechLayer* layer,
                         dbTechLayer* via_layer,
                         Rect box,
                         bool is_congested)
{
  _dbNet* owner = (_dbNet*) net;
  _dbBlock* block = (_dbBlock*) owner->getOwner();
  _dbGuide* guide = block->_guide_tbl->create();

  if (block->_journal) {
    debugPrint(block->getImpl()->getLogger(),
               utl::ODB,
               "DB_ECO",
               1,
               "ECO: create guide, layer {} box {}",
               layer->getName(),
               box);
    block->_journal->beginAction(dbJournal::CREATE_OBJECT);
    block->_journal->pushParam(dbGuideObj);
    block->_journal->pushParam(guide->getOID());
    block->_journal->endAction();
  }

  guide->layer_ = layer->getImpl()->getOID();
  guide->via_layer_ = via_layer->getImpl()->getOID();
  guide->box_ = box;
  guide->net_ = owner->getId();
  guide->is_congested_ = is_congested;
  guide->guide_next_ = owner->guides_;
  guide->is_jumper_ = false;
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

  if (block->_journal) {
    debugPrint(block->getImpl()->getLogger(),
               utl::ODB,
               "DB_ECO",
               1,
               "ECO: destroy guide, id: {}",
               guide->getId());
    block->_journal->beginAction(dbJournal::DELETE_OBJECT);
    block->_journal->pushParam(dbGuideObj);
    block->_journal->pushParam(net->getOID());
    block->_journal->pushParam(_guide->box_.xMin());
    block->_journal->pushParam(_guide->box_.yMin());
    block->_journal->pushParam(_guide->box_.xMax());
    block->_journal->pushParam(_guide->box_.yMax());
    block->_journal->pushParam(_guide->layer_);
    block->_journal->pushParam(_guide->via_layer_);
    block->_journal->pushParam(_guide->is_congested_);
    block->_journal->endAction();
  }

  uint id = _guide->getOID();
  _dbGuide* prev = nullptr;
  uint cur = net->guides_;
  while (cur) {
    _dbGuide* c = block->_guide_tbl->getPtr(cur);
    if (cur == id) {
      if (prev == nullptr) {
        net->guides_ = _guide->guide_next_;
      } else {
        prev->guide_next_ = _guide->guide_next_;
      }
      break;
    }
    prev = c;
    cur = c->guide_next_;
  }

  dbProperty::destroyProperties(guide);
  block->_guide_tbl->destroy((_dbGuide*) guide);
}

dbSet<dbGuide>::iterator dbGuide::destroy(dbSet<dbGuide>::iterator& itr)
{
  dbGuide* g = *itr;
  dbSet<dbGuide>::iterator next = ++itr;
  destroy(g);
  return next;
}

bool dbGuide::isJumper()
{
  bool is_jumper = false;
  _dbGuide* guide = (_dbGuide*) this;
  _dbDatabase* db = guide->getDatabase();
  if (db->isSchema(db_schema_has_jumpers)) {
    is_jumper = guide->is_jumper_;
  }
  return is_jumper;
}

void dbGuide::setIsJumper(bool jumper)
{
  _dbGuide* guide = (_dbGuide*) this;
  _dbDatabase* db = guide->getDatabase();
  if (db->isSchema(db_schema_has_jumpers)) {
    guide->is_jumper_ = jumper;
  }
}

// User Code End dbGuidePublicMethods
}  // namespace odb
// Generator Code End Cpp
