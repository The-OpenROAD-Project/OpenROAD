// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024-2025, The OpenROAD Authors

#include "cut/utils.h"

#include "sta/Liberty.hh"

namespace cut {

bool IsCombinational(sta::LibertyCell* cell)
{
  if (!cell) {
    return false;
  }
  return (!cell->isClockGate() && !cell->isPad() && !cell->isMacro()
          && !cell->hasSequentials() && !cell->isLevelShifter()
          && !cell->isIsolationCell() && !cell->isMemory());
}

}  // namespace cut
