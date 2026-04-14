// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2025, The OpenROAD Authors

#pragma once

#include <cstdint>
#include <vector>

#include "dpl/Opendp.h"
#include "odb/db.h"
#include "odb/geom.h"

namespace dpl {

class Opendp;
class Node;

// Pixel state values passed from NegotiationLegalizer to the observer.
enum class NegotiationPixelState : int8_t
{
  kNoRow = 0,     // no row exists here (capacity == 0, not a blockage)
  kFree = 1,      // valid site, unused
  kOccupied = 2,  // valid site, usage == capacity
  kOveruse = 3,   // valid site, usage > capacity
  kBlocked = 4,   // blockage (fixed cell / capacity forced to 0)
  kInvalid = 5,   // pixel is outside of core or in a hole (is_valid == false)
  kDrcViolation = 6  // cell at this site fails a PlacementDRC check
};

class DplObserver
{
 public:
  virtual ~DplObserver() = default;

  virtual void startPlacement(odb::dbBlock* block) = 0;
  virtual void drawSelected(odb::dbInst* instance, bool force) = 0;
  virtual void binSearch(const Node* cell,
                         GridX xl,
                         GridY yl,
                         GridX xh,
                         GridY yh)
      = 0;
  virtual void redrawAndPause() = 0;
  virtual const odb::dbInst* getDebugInstance() const { return nullptr; }

  // Negotiation-legalizer grid visualisation support (default no-ops).
  virtual void setNegotiationPixels(
      const std::vector<NegotiationPixelState>& pixels,
      int grid_w,
      int grid_h,
      int die_xlo,
      int die_ylo,
      int site_width,
      const std::vector<int>& row_y_dbu)
  {
  }
  virtual void clearNegotiationPixels() {}

  // Store the search window for |inst| so it can be drawn when that instance
  // is selected in the GUI.  |curr_window| is the window around the current
  // (displaced) position; pass an empty Rect when the cell has not moved.
  virtual void setNegotiationSearchWindow(odb::dbInst* inst,
                                          const odb::Rect& init_window,
                                          const odb::Rect& curr_window)
  {
  }
  // Clear all stored search windows (call at the start of each iteration).
  virtual void clearNegotiationSearchWindows() {}
};

}  // namespace dpl
