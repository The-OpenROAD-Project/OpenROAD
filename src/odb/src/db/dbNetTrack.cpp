// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbNetTrack.h"

#include "dbCore.h"
#include "dbDatabase.h"
#include "dbNet.h"
#include "dbTable.h"
#include "dbTechLayer.h"
#include "odb/db.h"
// User Code Begin Includes
#include <cstdint>

#include "dbBlock.h"
#include "odb/dbSet.h"
#include "odb/geom.h"
// User Code End Includes
namespace odb {
template class dbTable<_dbNetTrack>;

bool _dbNetTrack::operator==(const _dbNetTrack& rhs) const
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
  if (track_next_ != rhs.track_next_) {
    return false;
  }

  return true;
}

bool _dbNetTrack::operator<(const _dbNetTrack& rhs) const
{
  return true;
}

_dbNetTrack::_dbNetTrack(_dbDatabase* db)
{
}

dbIStream& operator>>(dbIStream& stream, _dbNetTrack& obj)
{
  stream >> obj.net_;
  stream >> obj.box_;
  stream >> obj.layer_;
  stream >> obj.track_next_;
  return stream;
}

dbOStream& operator<<(dbOStream& stream, const _dbNetTrack& obj)
{
  stream << obj.net_;
  stream << obj.box_;
  stream << obj.layer_;
  stream << obj.track_next_;
  return stream;
}

void _dbNetTrack::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);
}

////////////////////////////////////////////////////////////////////
//
// dbNetTrack - Methods
//
////////////////////////////////////////////////////////////////////

Rect dbNetTrack::getBox() const
{
  _dbNetTrack* obj = (_dbNetTrack*) this;
  return obj->box_;
}

// User Code Begin dbNetTrackPublicMethods

dbTechLayer* dbNetTrack::getLayer() const
{
  _dbNetTrack* obj = (_dbNetTrack*) this;
  auto tech = getDb()->getTech();
  return odb::dbTechLayer::getTechLayer(tech, obj->layer_);
}

dbNet* dbNetTrack::getNet() const
{
  _dbNetTrack* obj = (_dbNetTrack*) this;
  _dbBlock* block = (_dbBlock*) obj->getOwner();
  return (dbNet*) block->net_tbl_->getPtr(obj->net_);
}

dbNetTrack* dbNetTrack::create(dbNet* net, dbTechLayer* layer, Rect box)
{
  _dbNet* owner = (_dbNet*) net;
  _dbBlock* block = (_dbBlock*) owner->getOwner();
  _dbNetTrack* track = block->net_tracks_tbl_->create();
  track->layer_ = layer->getImpl()->getOID();
  track->box_ = box;
  track->net_ = owner->getId();
  track->track_next_ = owner->tracks_;
  owner->tracks_ = track->getOID();
  return (dbNetTrack*) track;
}

dbNetTrack* dbNetTrack::getNetTrack(dbBlock* block, uint32_t dbid)
{
  _dbBlock* owner = (_dbBlock*) block;
  return (dbNetTrack*) owner->net_tracks_tbl_->getPtr(dbid);
}

void dbNetTrack::destroy(dbNetTrack* track)
{
  _dbBlock* block = (_dbBlock*) track->getImpl()->getOwner();
  _dbNet* net = (_dbNet*) track->getNet();
  _dbNetTrack* _track = (_dbNetTrack*) track;

  uint32_t id = _track->getOID();
  _dbNetTrack* prev = nullptr;
  uint32_t cur = net->tracks_;
  while (cur) {
    _dbNetTrack* c = block->net_tracks_tbl_->getPtr(cur);
    if (cur == id) {
      if (prev == nullptr) {
        net->tracks_ = _track->track_next_;
      } else {
        prev->track_next_ = _track->track_next_;
      }
      break;
    }
    prev = c;
    cur = c->track_next_;
  }

  dbProperty::destroyProperties(track);
  block->net_tracks_tbl_->destroy((_dbNetTrack*) track);
}

dbSet<dbNetTrack>::iterator dbNetTrack::destroy(
    dbSet<dbNetTrack>::iterator& itr)
{
  dbNetTrack* track = *itr;
  dbSet<dbNetTrack>::iterator next = ++itr;
  destroy(track);
  return next;
}

// User Code End dbNetTrackPublicMethods
}  // namespace odb
   // Generator Code End Cpp
