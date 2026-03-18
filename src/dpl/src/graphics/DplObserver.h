// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2025, The OpenROAD Authors

#pragma once

#include <cstdint>
#include <vector>

#include "dpl/Opendp.h"
#include "odb/db.h"

namespace dpl {

class Opendp;
class Node;

// Pixel state values passed from HybridLegalizer to the observer.
enum class HybridPixelState : int8_t
{
  kNoRow = 0,    // no row exists here (capacity == 0, not a blockage)
  kFree = 1,     // valid site, unused
  kOccupied = 2, // valid site, usage == capacity
  kOveruse = 3,  // valid site, usage > capacity
  kBlocked = 4   // blockage (fixed cell / capacity forced to 0)
};

class DplObserver
{
 public:
  virtual ~DplObserver() = default;

  virtual void startPlacement(odb::dbBlock* block) = 0;
  virtual void placeInstance(odb::dbInst* instance) = 0;
  virtual void binSearch(const Node* cell,
                         GridX xl,
                         GridY yl,
                         GridX xh,
                         GridY yh)
      = 0;
  virtual void redrawAndPause() = 0;

  // Hybrid-legalizer grid visualisation support (default no-ops).
  virtual void setHybridPixels(const std::vector<HybridPixelState>& pixels,
                               int grid_w,
                               int grid_h,
                               int die_xlo,
                               int die_ylo,
                               int site_width,
                               int row_height)
  {
  }
  virtual void clearHybridPixels() {}
};

}  // namespace dpl
