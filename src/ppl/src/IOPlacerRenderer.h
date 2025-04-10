// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2025, The OpenROAD Authors

#pragma once

#include <vector>

#include "AbstractIOPlacerRenderer.h"
#include "SimulatedAnnealing.h"
#include "gui/gui.h"
#include "ppl/IOPlacer.h"

namespace ppl {

class IOPlacerRenderer : public gui::Renderer, public AbstractIOPlacerRenderer
{
 public:
  IOPlacerRenderer();
  ~IOPlacerRenderer() override;
  void setCurrentIteration(const int& current_iteration) override;
  void setPaintingInterval(const int& painting_interval) override;
  void setPinAssignment(const std::vector<IOPin>& assignment) override;
  void setSinks(const std::vector<std::vector<InstancePin>>& sinks) override;
  void setIsNoPauseMode(const bool& is_no_pause_mode) override;

  void redrawAndPause() override;

  void drawObjects(gui::Painter& painter) override;

 private:
  bool isDrawingNeeded() const;

  std::vector<ppl::IOPin> pin_assignment_;
  std::vector<std::vector<InstancePin>> sinks_;

  int painting_interval_;
  int current_iteration_;
  bool is_no_pause_mode_;
};

}  // namespace ppl
