// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include "odb/db.h"

namespace grt {
struct PinGridLocation
{
  PinGridLocation(odb::dbITerm* iterm, odb::dbBTerm* bterm, odb::Point pt)
      : iterm_(iterm), bterm_(bterm), pt_(pt)
  {
  }

  odb::dbITerm* iterm_;
  odb::dbBTerm* bterm_;
  odb::Point pt_;
};
}  // namespace grt
