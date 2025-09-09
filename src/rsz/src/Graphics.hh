// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#pragma once

#include "ResizerObserver.hh"
#include "gui/gui.h"

namespace rsz {

class Graphics : public gui::Renderer, public ResizerObserver
{
 public:
  Graphics();

  // ResizerObserver
  void setNet(odb::dbNet* net) override;
  void stopOnSubdivideStep(bool stop) override;
  void subdivideStart(odb::dbNet* net) override;
  void subdivide(const odb::Line& line) override;
  void subdivideDone() override;

  // Renderer
  void drawObjects(gui::Painter& painter) override;

 private:
  odb::dbNet* net_{nullptr};
  std::vector<odb::Line> lines_;
  // Ingore this net if true
  bool subdivide_ignore_{false};
  // stop at each step?
  bool stop_on_subdivide_step_{false};
};

}  // namespace rsz
