// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include "odb/db.h"

namespace grt {
struct PinGridLocation
{
  PinGridLocation(odb::dbITerm* iterm, odb::dbBTerm* bterm, odb::Point pt)
      : iterm(iterm), bterm(bterm), pt(pt)
  {
  }

  odb::dbITerm* iterm;
  odb::dbBTerm* bterm;
  odb::Point pt;
};
}  // namespace grt
