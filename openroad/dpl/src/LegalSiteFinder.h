// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include "dpl/Opendp.h"
#include "infrastructure/Coordinates.h"
#include "utl/Logger.h"

namespace dpl {

class Architecture;
class DplObserver;
class Grid;
class Node;
class PixelPt;
class PlacementDRC;

// Decides whether a cell can be placed at a grid site, checking pixel
// occupancy, groups, regions, one-site gaps, orientation, the power rail
// stack, and placement DRCs, and searches outward from a site for the
// nearest legal one. It holds pointers into Opendp's state, so a finder
// is only valid while the grid, DRC engine, and region tree it was made
// from are unchanged.
class LegalSiteFinder
{
 public:
  struct Limits
  {
    int max_displacement_x = 0;  // sites
    int max_displacement_y = 0;  // rows
    bool disallow_one_site_gaps = false;
  };

  LegalSiteFinder(utl::Logger* logger,
                  const Grid* grid,
                  Architecture* arch,
                  const PlacementDRC* drc_engine,
                  const Opendp::RtreeBox* regions_rtree,
                  DplObserver* debug_observer,
                  const Limits& limits);

  // Nearest legal site to (x, y) within the displacement limits and the
  // cell's group, or an empty PixelPt if there is none.
  PixelPt diamondSearch(const Node* cell, GridX x, GridY y) const;
  bool canBePlaced(const Node* cell, GridX bin_x, GridY bin_y) const;
  // True if the cell's span [x, x_end) x [y, y_end) lies entirely in its own
  // region, or overlaps no region when the cell has none.
  bool checkRegionOverlap(const Node* cell,
                          GridX x,
                          GridY y,
                          GridX x_end,
                          GridY y_end) const;
  // True if a multi-row cell's power pins line up with the rails when its
  // bottom row is y.
  bool checkRowPowerCompatible(const Node* cell, GridY y) const;
  static bool checkMasterSym(unsigned masterSym, unsigned cellOri);

 private:
  // Check all pixels in the span are free and the site is otherwise legal.
  bool checkPixels(const Node* cell,
                   GridX x,
                   GridY y,
                   GridX x_end,
                   GridY y_end) const;
  int calcDist(GridPt p0, GridPt p1) const;

  utl::Logger* logger_;
  const Grid* grid_;
  Architecture* arch_;
  const PlacementDRC* drc_engine_;
  const Opendp::RtreeBox* regions_rtree_;
  DplObserver* debug_observer_;
  Limits limits_;
};

}  // namespace dpl
