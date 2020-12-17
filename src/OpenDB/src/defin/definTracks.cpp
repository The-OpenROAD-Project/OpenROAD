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

#include "definTracks.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include "db.h"
#include "dbShape.h"

namespace odb {

definTracks::definTracks()
{
}

definTracks::~definTracks()
{
}

void definTracks::init()
{
  definBase::init();
}

void definTracks::tracksBegin(defDirection dir, int orig, int count, int step)
{
  _track._dir   = dir;
  _track._orig  = dbdist(orig);
  _track._step  = dbdist(step);
  _track._count = count;
}

void definTracks::tracksLayer(const char* layer_name)
{
  dbTechLayer* layer = _tech->findLayer(layer_name);

  if (layer == NULL) {
    notice(0, "error: undefined layer (%s) referenced\n", layer_name);
    ++_errors;
    return;
  }

  dbTrackGrid* grid = _block->findTrackGrid(layer);

  if (grid == NULL)
    grid = dbTrackGrid::create(_block, layer);

  if (_track._dir == DEF_X)
    grid->addGridPatternX(_track._orig, _track._count, _track._step);
  else
    grid->addGridPatternY(_track._orig, _track._count, _track._step);
}

void definTracks::tracksEnd()
{
}

}  // namespace odb
