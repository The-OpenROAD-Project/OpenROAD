// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbNetTrackItr.h"

#include "dbNetTrack.h"
#include "dbTable.h"
#include "dbTable.hpp"
#include "odb/odb.h"
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
  uint id = _parent->tracks_;
  uint list = 0;

  while (id != 0) {
    _dbNetTrack* _child = net_tracks_tbl_->getPtr(id);
    uint n = _child->track_next_;
    _child->track_next_ = list;
    list = id;
    id = n;
  }
  _parent->tracks_ = list;
  // User Code End reverse
}

uint dbNetTrackItr::sequential() const
{
  return 0;
}

uint dbNetTrackItr::size(dbObject* parent) const
{
  uint id;
  uint cnt = 0;

  for (id = dbNetTrackItr::begin(parent); id != dbNetTrackItr::end(parent);
       id = dbNetTrackItr::next(id)) {
    ++cnt;
  }

  return cnt;
}

uint dbNetTrackItr::begin(dbObject* parent) const
{
  // User Code Begin begin
  _dbNet* _parent = (_dbNet*) parent;
  return _parent->tracks_;
  // User Code End begin
}

uint dbNetTrackItr::end(dbObject* /* unused: parent */) const
{
  return 0;
}

uint dbNetTrackItr::next(uint id, ...) const
{
  // User Code Begin next
  _dbNetTrack* _track = net_tracks_tbl_->getPtr(id);
  return _track->track_next_;
  // User Code End next
}

dbObject* dbNetTrackItr::getObject(uint id, ...)
{
  return net_tracks_tbl_->getPtr(id);
}
}  // namespace odb
   // Generator Code End Cpp
