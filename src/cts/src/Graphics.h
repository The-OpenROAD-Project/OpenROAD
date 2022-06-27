#pragma once

#include <memory>

#include "gui/gui.h"
#include "HTreeBuilder.h"
#include "Clock.h"
#include "SinkClustering.h"

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
  
  // Graphics(utl::Logger* logger_,Clock& clock_,HTreeBuilder::LevelTopology& topLevelTopology_, Point<double> topLevelBufferLoc_);
  Graphics(utl::Logger* logger_, HTreeBuilder* HTreeBuilder_, Clock* clock_);

  Graphics(utl::Logger* logger_, SinkClustering* SinkClustering_, unsigned groupSize);

  // Draw the graphics; optionally pausing afterwards
  void clockPlot(bool pause = false);

  // Show a message in the status bar
  void status(const std::string& message);

  // From Renderer API
  virtual void drawObjects(gui::Painter& painter) override;
  // virtual gui::SelectionSet select(odb::dbTechLayer* layer,
  //                                  const odb::Rect& region) override;

  // Is the GUI being displayed (true) or are we in batch mode (false)
  static bool guiActive();

 private:
  Clock* clock_;
  // TreeBuilder::LevelTopology& topLevelTopology_;
  // Point<double> topLevelBufferLoc_;
  HTreeBuilder* HTreeBuilder_;
  unsigned groupSize_;
  SinkClustering* SinkClustering_;
//   GCell* selected_ = nullptr;
  utl::Logger* logger_;
  int x1_, x2_, y1_, y2_;

//   void drawNesterov(gui::Painter& painter);
//   void drawInitial(gui::Painter& painter);
//   void drawBounds(gui::Painter& painter);

  void drawHTree(gui::Painter& painter);
  void drawCluster(gui::Painter& painter);
  void setLineCoordinates(int x1, int y1, int x2, int y2);
  void drawLineObject(gui::Painter& painter);
};

}  // namespace cts
