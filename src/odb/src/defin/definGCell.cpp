// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "definGCell.h"

#include <cctype>
#include <cstdio>
#include <cstdlib>

#include "odb/db.h"
#include "odb/dbShape.h"

namespace odb {

definGCell::definGCell()
{
}

definGCell::~definGCell()
{
}

void definGCell::init()
{
  definBase::init();
}

void definGCell::gcell(defDirection dir, int orig, int count, int step)
{
  orig = dbdist(orig);
  step = dbdist(step);

  dbGCellGrid* grid = _block->getGCellGrid();

  if (grid == nullptr) {
    grid = dbGCellGrid::create(_block);
  }

  if (dir == DEF_X) {
    grid->addGridPatternX(orig, count, step);
  } else {
    grid->addGridPatternY(orig, count, step);
  }
}

}  // namespace odb
