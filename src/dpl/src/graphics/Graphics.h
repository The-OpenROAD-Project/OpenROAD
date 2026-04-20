// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2025, The OpenROAD Authors

#pragma once

#include <unordered_map>
#include <utility>
#include <vector>

#include "DplObserver.h"
#include "dpl/Opendp.h"
#include "gui/gui.h"
#include "odb/db.h"
#include "odb/geom.h"

namespace dpl {

class Opendp;
class Node;

class Graphics : public gui::Renderer, public DplObserver
{
 public:
  Graphics(Opendp* dp,
           const odb::dbInst* debug_instance,
           bool paint_pixels = true,
           bool paint_negotiation_pixels = false);
  ~Graphics() override = default;
  void startPlacement(odb::dbBlock* block) override;
  void drawSelected(odb::dbInst* instance, bool force) override;
  void binSearch(const Node* cell,
                 GridX xl,
                 GridY yl,
                 GridX xh,
                 GridY yh) override;
  void redrawAndPause() override;
  const odb::dbInst* getDebugInstance() const override
  {
    return debug_instance_;
  }

  // NegotiationLegalizer grid visualisation
  void setNegotiationPixels(const std::vector<NegotiationPixelState>& pixels,
                            int grid_w,
                            int grid_h,
                            int die_xlo,
                            int die_ylo,
                            int site_width,
                            const std::vector<int>& row_y_dbu) override;
  void clearNegotiationPixels() override;
  void setNegotiationSearchWindow(odb::dbInst* inst,
                                  const odb::Rect& init_window,
                                  const odb::Rect& curr_window) override;
  void clearNegotiationSearchWindows() override;

  // From Renderer API
  void drawObjects(gui::Painter& painter) override;

  static bool guiActive();

 private:
  Opendp* dp_;
  const odb::dbInst* debug_instance_;
  odb::dbBlock* block_ = nullptr;
  bool paint_pixels_;
  bool paint_negotiation_pixels_;
  std::vector<odb::Rect> searched_;

  // NegotiationLegalizer grid snapshot for rendering
  std::vector<NegotiationPixelState> negotiation_pixels_;
  int negotiation_grid_w_{0};
  int negotiation_grid_h_{0};
  int negotiation_die_xlo_{0};
  int negotiation_die_ylo_{0};
  int negotiation_site_width_{0};
  std::vector<int> negotiation_row_y_dbu_;

  // Per-cell search windows: init window + current-position window (may be
  // empty if the cell is not displaced).  Keyed by dbInst* so drawObjects()
  // can look up whichever instance the user has selected in the GUI.
  std::unordered_map<odb::dbInst*, std::pair<odb::Rect, odb::Rect>>
      negotiation_search_windows_;
};

}  // namespace dpl
