// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2026, The OpenROAD Authors

#include "ord/OpenRoad.hh"

// Stubs out functions from OpenRoad that aren't needed by Testing but
// are referenced from dst modules or its dependencies.

namespace ord {

OpenRoad::OpenRoad() = default;

OpenRoad* OpenRoad::openRoad()
{
  return nullptr;
}

}  // namespace ord
