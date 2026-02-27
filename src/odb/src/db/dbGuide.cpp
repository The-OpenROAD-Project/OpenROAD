// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbGuide.h"

#include "dbCore.h"
#include "dbDatabase.h"
#include "dbNet.h"
#include "dbTable.h"
#include "dbTechLayer.h"
#include "odb/db.h"
// User Code Begin Includes
#include <cstdint>

#include "dbBlock.h"
#include "dbJournal.h"
#include "odb/dbObject.h"
#include "odb/dbSet.h"
#include "odb/geom.h"
#include "utl/Logger.h"
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
  if (is_connect_to_term_ != rhs.is_connect_to_term_) {
    return false;
  }

  return true;
}

bool _dbGuide::operator<(const _dbGuide& rhs) const
{
  return true;
}

_dbGuide::_dbGuide(_dbDatabase* db)
{
  is_congested_ = false;
  is_jumper_ = false;
  is_connect_to_term_ = false;
}

dbIStream& operator>>(dbIStream& stream, _dbGuide& obj)
{
  stream >> obj.net_;
  stream >> obj.box_;
  stream >> obj.layer_;
  if (obj.getDatabase()->isSchema(kSchemaDbGuideViaLayer)) {
    stream >> obj.via_layer_;
  }
  stream >> obj.guide_next_;
  if (obj.getDatabase()->isSchema(kSchemaDbGuideCongested)) {
    stream >> obj.is_congested_;
  }
  if (obj.getDatabase()->isSchema(kSchemaHasJumpers)) {
    stream >> obj.is_jumper_;
  }
  if (obj.getDatabase()->isSchema(kSchemaGuideConnectedToTerm)) {
    stream >> obj.is_connect_to_term_;
  }
  return stream;
}

dbOStream& operator<<(dbOStream& stream, const _dbGuide& obj)
{
  stream << obj.net_;
  stream << obj.box_;
  stream << obj.layer_;
  stream << obj.via_layer_;
  stream << obj.guide_next_;
  stream << obj.is_congested_;
  stream << obj.is_jumper_;
  stream << obj.is_connect_to_term_;
  return stream;
}

void _dbGuide::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);
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
  return (dbNet*) block->net_tbl_->getPtr(obj->net_);
}

dbGuide* dbGuide::create(dbNet* net,
                         dbTechLayer* layer,
                         dbTechLayer* via_layer,
                         Rect box,
                         bool is_congested)
{
  _dbNet* owner = (_dbNet*) net;
  _dbBlock* block = (_dbBlock*) owner->getOwner();
  _dbGuide* guide = block->guide_tbl_->create();

  debugPrint(block->getImpl()->getLogger(),
             utl::ODB,
             "DB_EDIT",
             2,
             "EDIT: create dbGuide at id {}, in layer {} box {}",
             guide->getOID(),
             layer->getName(),
             box);

  if (block->journal_) {
    block->journal_->beginAction(dbJournal::kCreateObject);
    block->journal_->pushParam(dbGuideObj);
    block->journal_->pushParam(guide->getOID());
    block->journal_->endAction();
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

dbGuide* dbGuide::getGuide(dbBlock* block, uint32_t dbid)
{
  _dbBlock* owner = (_dbBlock*) block;
  return (dbGuide*) owner->guide_tbl_->getPtr(dbid);
}

void dbGuide::destroy(dbGuide* guide)
{
  _dbBlock* block = (_dbBlock*) guide->getImpl()->getOwner();
  _dbNet* net = (_dbNet*) guide->getNet();
  _dbGuide* _guide = (_dbGuide*) guide;

  debugPrint(block->getImpl()->getLogger(),
             utl::ODB,
             "DB_EDIT",
             2,
             "EDIT: delete dbGuide at id {}, in layer {} box {}",
             guide->getId(),
             guide->getLayer()->getName(),
             guide->getBox());

  if (block->journal_) {
    block->journal_->beginAction(dbJournal::kDeleteObject);
    block->journal_->pushParam(dbGuideObj);
    block->journal_->pushParam(net->getOID());
    block->journal_->pushParam(_guide->box_.xMin());
    block->journal_->pushParam(_guide->box_.yMin());
    block->journal_->pushParam(_guide->box_.xMax());
    block->journal_->pushParam(_guide->box_.yMax());
    block->journal_->pushParam(_guide->layer_);
    block->journal_->pushParam(_guide->via_layer_);
    block->journal_->pushParam(_guide->is_congested_);
    block->journal_->endAction();
  }

  uint32_t id = _guide->getOID();
  _dbGuide* prev = nullptr;
  uint32_t cur = net->guides_;
  while (cur) {
    _dbGuide* c = block->guide_tbl_->getPtr(cur);
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
  block->guide_tbl_->destroy((_dbGuide*) guide);
}

dbSet<dbGuide>::iterator dbGuide::destroy(dbSet<dbGuide>::iterator& itr)
{
  dbGuide* g = *itr;
  dbSet<dbGuide>::iterator next = ++itr;
  destroy(g);
  return next;
}

bool dbGuide::isJumper() const
{
  bool is_jumper = false;
  _dbGuide* guide = (_dbGuide*) this;
  _dbDatabase* db = guide->getDatabase();
  if (db->isSchema(kSchemaHasJumpers)) {
    is_jumper = guide->is_jumper_;
  }
  return is_jumper;
}

void dbGuide::setIsJumper(bool jumper)
{
  _dbGuide* guide = (_dbGuide*) this;
  _dbDatabase* db = guide->getDatabase();
  if (db->isSchema(kSchemaHasJumpers)) {
    guide->is_jumper_ = jumper;
  }
}

bool dbGuide::isConnectedToTerm() const
{
  bool is_connected_to_term = false;
  _dbGuide* guide = (_dbGuide*) this;
  is_connected_to_term = guide->is_connect_to_term_;
  return is_connected_to_term;
}

void dbGuide::setIsConnectedToTerm(bool is_connected)
{
  _dbGuide* guide = (_dbGuide*) this;
  guide->is_connect_to_term_ = is_connected;
}

// User Code End dbGuidePublicMethods
}  // namespace odb
// Generator Code End Cpp
