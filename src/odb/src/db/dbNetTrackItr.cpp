///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2019, Nefelus Inc
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
#include "dbNetTrackItr.h"

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

bool dbNetTrackItr::reversible()
{
  return true;
}

bool dbNetTrackItr::orderReversed()
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
    _dbNetTrack* _child = _net_tracks_tbl->getPtr(id);
    uint n = _child->track_next_;
    _child->track_next_ = list;
    list = id;
    id = n;
  }
  _parent->tracks_ = list;
  // User Code End reverse
}

uint dbNetTrackItr::sequential()
{
  return 0;
}

uint dbNetTrackItr::size(dbObject* parent)
{
  uint id;
  uint cnt = 0;

  for (id = dbNetTrackItr::begin(parent); id != dbNetTrackItr::end(parent);
       id = dbNetTrackItr::next(id)) {
    ++cnt;
  }

  return cnt;
}

uint dbNetTrackItr::begin(dbObject* parent)
{
  // User Code Begin begin
  _dbNet* _parent = (_dbNet*) parent;
  return _parent->tracks_;
  // User Code End begin
}

uint dbNetTrackItr::end(dbObject* /* unused: parent */)
{
  return 0;
}

uint dbNetTrackItr::next(uint id, ...)
{
  // User Code Begin next
  _dbNetTrack* _track = _net_tracks_tbl->getPtr(id);
  return _track->track_next_;
  // User Code End next
}

dbObject* dbNetTrackItr::getObject(uint id, ...)
{
  return _net_tracks_tbl->getPtr(id);
}
}  // namespace odb
   // Generator Code End Cpp