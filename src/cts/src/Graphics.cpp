#include <algorithm>
#include <cstdio>
#include <limits>

#include "Graphics.h"
#include "utl/Logger.h"

namespace cts {
  using utl::CTS;

Graphics::Graphics(utl::Logger* logger,
          HTreeBuilder* HTreeBuilder_,
          Clock* clock_)
    : HTreeBuilder_(HTreeBuilder_),
    logger_(logger),
    clock_(clock_)
{
  gui::Gui::get()->registerRenderer(this);
}

Graphics::Graphics(utl::Logger* logger,SinkClustering* SinkClustering,unsigned groupSize)
    : SinkClustering_(SinkClustering),logger_(logger),groupSize_(groupSize),clock_(nullptr)
{
  gui::Gui::get()->registerRenderer(this);
}

void Graphics::drawLineObject(gui::Painter& painter)
{
  gui::Painter::Color color = gui::Painter::red;
  color.a = 180;
  painter.setBrush(color);
  painter.drawLine(x1_, y1_, x2_, y2_);
}

void Graphics::setLineCoordinates(int x1, int y1, int x2, int y2)
{
  x1_=x1;
  x2_=x2;
  y1_=y1;
  y2_=y2;
}

void Graphics::drawCluster(gui::Painter& painter)
{
  std::vector<gui::Painter::Color> colors{gui::Painter::red,
                                          gui::Painter::yellow, //orange
                                          gui::Painter::green,
                                          gui::Painter::dark_red, //red
                                          gui::Painter::magenta, //purple
                                          gui::Painter::dark_yellow, //brown
                                          gui::Painter::blue, //pink
                                          gui::Painter::gray,
                                          gui::Painter::dark_green, //olive
                                          gui::Painter::cyan};

  unsigned clusterCounter = 0;
  double totalWL = 0;
  for (const std::vector<unsigned>& clusters : SinkClustering_->sinkClusteringSolution()) {
    const unsigned color = clusterCounter % colors.size();

    std::vector<Point<double>> clusterNodes;
    int tile_size_= 48000 * 13;
    for (unsigned idx : clusters) {
      const Point<double>& point = SinkClustering_->getPoints()[idx];
      clusterNodes.emplace_back(SinkClustering_->getPoints()[idx]); 

      painter.setBrush(colors[color]);
      painter.setPen(colors[color]);
      painter.setPenWidth(2500);
      int xreal= tile_size_ * (point.getX() + 0.5);
      int yreal= tile_size_ * (point.getY() + 0.5);

      painter.drawCircle(xreal, yreal, 3500);
    }
    const double wl = SinkClustering_->getWireLength(clusterNodes);
    totalWL += wl;
    clusterCounter++;
  }
}

void Graphics::drawHTree(gui::Painter& painter)
{
  gui::Painter::Color color = gui::Painter::red;
  color.a = 180;
  painter.setBrush(color);
  painter.setPenWidth(700);
  int tile_size_= 8000;

   clock_->forEachSink([&](const ClockInst& sink) { 
    int xreal= tile_size_ * (sink.getX() / HTreeBuilder_->getWireSegmentUnit() + 0.5);
    int yreal= tile_size_ * (sink.getY() / HTreeBuilder_->getWireSegmentUnit() + 0.5);
        painter.drawCircle(xreal, yreal, 500);
    });

  Point<double> topLevelBufferLoc = HTreeBuilder_->getSinkRegion().computeCenter();
  HTreeBuilder_->getTopologyVector().front().forEachBranchingPoint(
      [&](unsigned idx, Point<double> branchPoint) {
        if (topLevelBufferLoc.getX() < branchPoint.getX()) {
         int x1 = tile_size_ * (topLevelBufferLoc.getX() + 0.5);
         int y1 = tile_size_ * (topLevelBufferLoc.getY() + 0.5);
         int x2 =  tile_size_ * (branchPoint.getX() + 0.5);
         int y2 =  tile_size_ * (branchPoint.getY() + 0.5);
          painter.drawLine(x1, y1, x2, y2);
        } else {
         int x1 = tile_size_ * (topLevelBufferLoc.getX() + 0.5);
         int y1 = tile_size_ * (topLevelBufferLoc.getY() + 0.5);
         int x2 =  tile_size_ * (branchPoint.getX() + 0.5);
         int y2 =  tile_size_ * (branchPoint.getY() + 0.5);
           painter.drawLine(x2, y2, x1, y1);
        }
      });

  for (int levelIdx = 1; levelIdx < HTreeBuilder_->getTopologyVector().size(); ++levelIdx) {

    HTreeBuilder_->getTopologyVector()[levelIdx].forEachBranchingPoint([&](unsigned idx,
                                       Point<double> branchPoint) {
      unsigned parentIdx = HTreeBuilder_->getTopologyVector()[levelIdx].getBranchingPointParentIdx(idx);
      Point<double> parentPoint
          = HTreeBuilder_->getTopologyVector()[levelIdx - 1].getBranchingPoint(parentIdx);
      gui::Painter::Color color = gui::Painter::yellow;
      color.a = 180;
      if (levelIdx % 2 == 0) {
        gui::Painter::Color color = gui::Painter::red;
        color.a = 180;
      }
      painter.setBrush(color);
      painter.setPen(color);
      painter.setPenWidth(700);

      if (parentPoint.getX() < branchPoint.getX()) {
         int x1 = tile_size_ * (parentPoint.getX() + 0.5);
         int y1 = tile_size_ * (parentPoint.getY() + 0.5);
         int x2 =  tile_size_ * (branchPoint.getX() + 0.5);
         int y2 =  tile_size_ * (branchPoint.getY() + 0.5);
         painter.drawLine(x1, y1, x2, y2);
      } else {
         int x1 = tile_size_ * (parentPoint.getX() + 0.5);
         int y1 = tile_size_ * (parentPoint.getY() + 0.5);
         int x2 =  tile_size_ * (branchPoint.getX() + 0.5);
         int y2 =  tile_size_ * (branchPoint.getY() + 0.5);
         painter.drawLine(x2, y2, x1, y1);
      }
    });
  }
}

void Graphics::drawObjects(gui::Painter& painter)
{
  // if(clock_)
  //   drawHTree(painter);

  if(SinkClustering_){
      logger_->report("calling draw cluster");
    drawCluster(painter);  
  }
}

void Graphics::clockPlot(bool pause)
{
  gui::Gui::get()->redraw();
  if (pause) {
    gui::Gui::get()->pause();
  }
}

void Graphics::status(const std::string& message)
{
  gui::Gui::get()->status(message);
}

/* static */
bool Graphics::guiActive()
{
  return gui::Gui::enabled();
}

}  // namespace cts
