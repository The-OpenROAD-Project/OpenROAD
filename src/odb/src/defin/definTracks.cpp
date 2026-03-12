// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "definTracks.h"

#include <cctype>
#include <cstdio>
#include <cstdlib>

#include "definTypes.h"
#include "odb/db.h"
#include "odb/dbShape.h"
#include "utl/Logger.h"

namespace odb {

void definTracks::tracksBegin(defDirection dir,
                              int orig,
                              int count,
                              int step,
                              int first_mask,
                              bool samemask)
{
  _track._dir = dir;
  _track._orig = dbdist(orig);
  _track._step = dbdist(step);
  _track._count = count;
  _track._first_mask = first_mask;
  _track._samemask = samemask;
}

void definTracks::tracksLayer(const char* layer_name)
{
  dbTechLayer* layer = _tech->findLayer(layer_name);

  if (layer == nullptr) {
    _logger->warn(
        utl::ODB, 165, "error: undefined layer ({}) referenced", layer_name);
    ++_errors;
    return;
  }

  dbTrackGrid* grid = _block->findTrackGrid(layer);

  if (grid == nullptr) {
    grid = dbTrackGrid::create(_block, layer);
  }

  if (_track._dir == DEF_X) {
    grid->addGridPatternX(_track._orig,
                          _track._count,
                          _track._step,
                          _track._first_mask,
                          _track._samemask);
  } else {
    grid->addGridPatternY(_track._orig,
                          _track._count,
                          _track._step,
                          _track._first_mask,
                          _track._samemask);
  }
}

void definTracks::tracksEnd()
{
}

}  // namespace odb
