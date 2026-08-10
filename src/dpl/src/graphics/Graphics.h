// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2025, The OpenROAD Authors

#pragma once

#include <mutex>
#include <unordered_map>
#include <unordered_set>
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
  void clearDiamondSearch(const Node* cell) override;
  void clearAllDiamondSearches() override;
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

  void addNegotiationViolationsPoint(int iter,
                                     int violations,
                                     int illegal_count,
                                     int illegal_site_count) override;
  void addNegotiationPhase2Marker(int iter) override;
  void addCurrentIterMover(odb::dbInst* inst) override;
  void clearCurrentIterMovers() override;

  // From Renderer API
  void drawObjects(gui::Painter& painter) override;

  static bool guiActive();

 private:
  Opendp* dp_;
  const odb::dbInst* debug_instance_;
  odb::dbBlock* block_ = nullptr;
  bool paint_pixels_;
  bool paint_negotiation_pixels_;

  std::mutex state_mutex_;

  // One row of a diamond search region, in dbu.
  struct RowSpan
  {
    int y_lo;
    int y_hi;
    int x_lo;
    int x_hi;
  };

  // Most recent diamond search of one cell: the x range tried in each row, not
  // a box per candidate, so a 200k-candidate search costs one span per row.
  struct DiamondSearch
  {
    std::vector<RowSpan> rows;                        // sorted by y_lo
    odb::Rect last;                                   // last candidate tried
    std::vector<std::vector<odb::Point>> boundaries;  // union of |rows|, cached
    bool boundaries_valid = false;
  };

  // An entry means a search for that cell has started; binSearch() ignores
  // cells without one.
  std::unordered_map<const odb::dbInst*, DiamondSearch> searched_diamond_;

  // Row span capacity kept between searches.
  static constexpr size_t kMaxRetainedRows = 64;
  static constexpr int kSearchOutlineWidth = 3;

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

  // Last instance whose search window size was logged, so drawObjects()
  // (called on every repaint) only reports it once per selection change.
  odb::dbInst* last_logged_search_window_inst_ = nullptr;

  // Cells that moved during the most recent negotiation iteration.
  // Empty means "no iteration info available" — all movers use directional
  // colors.
  std::unordered_set<odb::dbInst*> current_iter_movers_;

  gui::Chart* violations_chart_ = nullptr;
};

}  // namespace dpl
