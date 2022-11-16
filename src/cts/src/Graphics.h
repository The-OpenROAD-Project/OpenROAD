#pragma once

#include <memory>

#include "Clock.h"
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
class Graphics : public gui::Renderer
{
 public:
  Graphics(HTreeBuilder* HTreeBuilder, Clock* clock);

  Graphics(SinkClustering* SinkClustering,
           const std::vector<Point<double>>& points);

  // Draw the graphics; optionally pausing afterwards
  void clockPlot(bool pause = false);

  // Show a message in the status bar
  void status(const std::string& message);

  // From Renderer API
  virtual void drawObjects(gui::Painter& painter) override;

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
