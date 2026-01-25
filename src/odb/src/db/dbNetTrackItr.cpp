// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbNetTrackItr.h"

#include <cstdint>

#include "dbNetTrack.h"
#include "dbTable.h"
// User Code Begin Includes
#include "dbNet.h"
// User Code End Includes

namespace odb {

////////////////////////////////////////////////////////////////////
//
// dbNetTrackItr - Methods
//
////////////////////////////////////////////////////////////////////

bool dbNetTrackItr::reversible() const
{
  return true;
}

bool dbNetTrackItr::orderReversed() const
{
  return true;
}

void dbNetTrackItr::reverse(dbObject* parent)
{
  // User Code Begin reverse
  _dbNet* _parent = (_dbNet*) parent;
  uint32_t id = _parent->tracks_;
  uint32_t list = 0;

  while (id != 0) {
    _dbNetTrack* _child = net_tracks_tbl_->getPtr(id);
    uint32_t n = _child->track_next_;
    _child->track_next_ = list;
    list = id;
    id = n;
  }
  _parent->tracks_ = list;
  // User Code End reverse
}

uint32_t dbNetTrackItr::sequential() const
{
  return 0;
}

uint32_t dbNetTrackItr::size(dbObject* parent) const
{
  uint32_t id;
  uint32_t cnt = 0;

  for (id = dbNetTrackItr::begin(parent); id != dbNetTrackItr::end(parent);
       id = dbNetTrackItr::next(id)) {
    ++cnt;
  }

  return cnt;
}

uint32_t dbNetTrackItr::begin(dbObject* parent) const
{
  // User Code Begin begin
  _dbNet* _parent = (_dbNet*) parent;
  return _parent->tracks_;
  // User Code End begin
}

uint32_t dbNetTrackItr::end(dbObject* /* unused: parent */) const
{
  return 0;
}

uint32_t dbNetTrackItr::next(uint32_t id, ...) const
{
  // User Code Begin next
  _dbNetTrack* _track = net_tracks_tbl_->getPtr(id);
  return _track->track_next_;
  // User Code End next
}

dbObject* dbNetTrackItr::getObject(uint32_t id, ...)
{
  return net_tracks_tbl_->getPtr(id);
}
}  // namespace odb
   // Generator Code End Cpp
