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
#include "dbNetTrack.h"

#include "dbDatabase.h"
#include "dbDiff.hpp"
#include "dbNet.h"
#include "dbTable.h"
#include "dbTable.hpp"
#include "dbTechLayer.h"
#include "odb/db.h"
// User Code Begin Includes
#include "dbBlock.h"
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

void _dbNetTrack::differences(dbDiff& diff,
                              const char* field,
                              const _dbNetTrack& rhs) const
{
  DIFF_BEGIN
  DIFF_FIELD(net_);
  DIFF_FIELD(box_);
  DIFF_FIELD(layer_);
  DIFF_FIELD(track_next_);
  DIFF_END
}

void _dbNetTrack::out(dbDiff& diff, char side, const char* field) const
{
  DIFF_OUT_BEGIN
  DIFF_OUT_FIELD(net_);
  DIFF_OUT_FIELD(box_);
  DIFF_OUT_FIELD(layer_);
  DIFF_OUT_FIELD(track_next_);

  DIFF_END
}

_dbNetTrack::_dbNetTrack(_dbDatabase* db)
{
}

_dbNetTrack::_dbNetTrack(_dbDatabase* db, const _dbNetTrack& r)
{
  net_ = r.net_;
  box_ = r.box_;
  layer_ = r.layer_;
  track_next_ = r.track_next_;
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
  return (dbNet*) block->_net_tbl->getPtr(obj->net_);
}

dbNetTrack* dbNetTrack::create(dbNet* net, dbTechLayer* layer, Rect box)
{
  _dbNet* owner = (_dbNet*) net;
  _dbBlock* block = (_dbBlock*) owner->getOwner();
  _dbNetTrack* track = block->_net_tracks_tbl->create();
  track->layer_ = layer->getImpl()->getOID();
  track->box_ = box;
  track->net_ = owner->getId();
  track->track_next_ = owner->tracks_;
  owner->tracks_ = track->getOID();
  return (dbNetTrack*) track;
}

dbNetTrack* dbNetTrack::getNetTrack(dbBlock* block, uint dbid)
{
  _dbBlock* owner = (_dbBlock*) block;
  return (dbNetTrack*) owner->_net_tracks_tbl->getPtr(dbid);
}

void dbNetTrack::destroy(dbNetTrack* track)
{
  _dbBlock* block = (_dbBlock*) track->getImpl()->getOwner();
  _dbNet* net = (_dbNet*) track->getNet();
  _dbNetTrack* _track = (_dbNetTrack*) track;

  uint id = _track->getOID();
  _dbNetTrack* prev = nullptr;
  uint cur = net->tracks_;
  while (cur) {
    _dbNetTrack* c = block->_net_tracks_tbl->getPtr(cur);
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
  block->_net_tracks_tbl->destroy((_dbNetTrack*) track);
}

// User Code End dbNetTrackPublicMethods
}  // namespace odb
   // Generator Code End Cpp