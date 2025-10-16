// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

#pragma once

#include <memory>
#include <string>
#include <vector>

#include "Clock.h"
#include "CtsObserver.h"
#include "HTreeBuilder.h"
#include "SinkClustering.h"
#include "gui/gui.h"

namespace utl {
class Logger;
}

namespace cts {

class HTreeBuilder;
class SinkClustering;

// This class draws debugging graphics on the layout
class CtsGraphics : public gui::Renderer, public CtsObserver
{
 public:
  void initializeWithClock(HTreeBuilder* h_tree_builder, Clock& clock) override;

  void initializeWithPoints(SinkClustering* SinkClustering,
                            const std::vector<Point<double>>& points) override;

  // Draw the graphics; optionally pausing afterwards
  void clockPlot(bool pause) override;

  // Show a message in the status bar
  void status(const std::string& message) override;

  // From Renderer API
  void drawObjects(gui::Painter& painter) override;

  // Is the GUI being displayed (true) or are we in batch mode (false)
  static bool guiActive();

 private:
  void drawHTree(gui::Painter& painter);
  void drawCluster(gui::Painter& painter);

  Clock* clock_;
  HTreeBuilder* h_tree_builder_;
  SinkClustering* sink_clustering_;
  std::vector<Point<double>> points_;  // used for sink_clustering
};

}  // namespace cts
