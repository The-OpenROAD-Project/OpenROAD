// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include "odb/db.h"
#include "odb/geom.h"

namespace grt {
struct PinGridLocation
{
  PinGridLocation(odb::dbITerm* iterm,
                  odb::dbBTerm* bterm,
                  odb::Point pt,
                  odb::Point grid_pt,
                  int conn_layer)
      : iterm(iterm),
        bterm(bterm),
        pt(pt),
        grid_pt(grid_pt),
        conn_layer(conn_layer)
  {
  }

  odb::dbITerm* iterm;
  odb::dbBTerm* bterm;
  odb::Point pt;
  odb::Point grid_pt;
  int conn_layer;
};
}  // namespace grt
