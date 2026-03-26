// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2025, The OpenROAD Authors

#pragma once

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
           bool paint_hybrid_pixels = false);
  ~Graphics() override = default;
  void startPlacement(odb::dbBlock* block) override;
  void drawSelected(odb::dbInst* instance) override;
  void binSearch(const Node* cell,
                 GridX xl,
                 GridY yl,
                 GridX xh,
                 GridY yh) override;
  void redrawAndPause() override;
  const odb::dbInst* getDebugInstance() const override { return debug_instance_; }

  // HybridLegalizer grid visualisation
  void setHybridPixels(const std::vector<HybridPixelState>& pixels,
                       int grid_w,
                       int grid_h,
                       int die_xlo,
                       int die_ylo,
                       int site_width,
                       const std::vector<int>& row_y_dbu) override;
  void clearHybridPixels() override;

  // From Renderer API
  void drawObjects(gui::Painter& painter) override;

  static bool guiActive();

 private:
  Opendp* dp_;
  const odb::dbInst* debug_instance_;
  odb::dbBlock* block_ = nullptr;
  bool paint_pixels_;
  bool paint_hybrid_pixels_;
  std::vector<odb::Rect> searched_;

  // HybridLegalizer grid snapshot for rendering
  std::vector<HybridPixelState> hybrid_pixels_;
  int hybrid_grid_w_{0};
  int hybrid_grid_h_{0};
  int hybrid_die_xlo_{0};
  int hybrid_die_ylo_{0};
  int hybrid_site_width_{0};
  std::vector<int> hybrid_row_y_dbu_;
};

}  // namespace dpl
