#include <algorithm>
#include <cstdio>
#include <limits>

#include "Graphics.h"
#include "utl/Logger.h"

namespace cts {
using utl::CTS;

Graphics::Graphics(utl::Logger* logger,
                   HTreeBuilder* h_tree_builder,
                   Clock* clock_)
    : clock_(clock_),
      h_tree_builder_(h_tree_builder),
      group_size_(0),
      sink_clustering_(nullptr),
      logger_(logger)
{
  gui::Gui::get()->registerRenderer(this);
}

Graphics::Graphics(utl::Logger* logger,
                   SinkClustering* SinkClustering,
                   unsigned groupSize,
                   const std::vector<Point<double>>& points)
    : clock_(nullptr),
      h_tree_builder_(nullptr),
      group_size_(groupSize),
      sink_clustering_(SinkClustering),
      logger_(logger),
      points_(points)
{
  gui::Gui::get()->registerRenderer(this);
}

void Graphics::drawCluster(gui::Painter& painter)
{
  std::vector<gui::Painter::Color> colors{gui::Painter::red,
                                          gui::Painter::yellow,
                                          gui::Painter::green,
                                          gui::Painter::dark_red,
                                          gui::Painter::magenta,
                                          gui::Painter::dark_yellow,
                                          gui::Painter::blue,
                                          gui::Painter::dark_gray,
                                          gui::Painter::dark_green,
                                          gui::Painter::cyan};

  unsigned clusterCounter = 0;
  double totalWL = 0;
  bool first = true;
  odb::Point last;
  for (const std::vector<unsigned>& clusters :
       sink_clustering_->sinkClusteringSolution()) {
    const unsigned color = clusterCounter % colors.size();

    std::vector<Point<double>> clusterNodes;
    bool first_in_cluster = true;
    for (unsigned idx : clusters) {
      const Point<double>& point = points_.at(idx);
      clusterNodes.emplace_back(point);

      int unit = sink_clustering_->getScaleFactor();
      int xreal = unit * point.getX() + 0.5;
      int yreal = unit * point.getY() + 0.5;

      if (first) {
        first = false;
        first_in_cluster = false;
      } else {
        if (first_in_cluster) {
          first_in_cluster = false;
          painter.setPen(gui::Painter::white, /* cosmetic */ true);
        } else {
          painter.setPen(colors[color], /* cosmetic */ true);
        }
        painter.drawLine(last, {xreal, yreal});
      }
      last = {xreal, yreal};

      painter.setPenAndBrush(colors[color]);
      painter.setPenWidth(2500);
      painter.drawCircle(xreal, yreal, 500);
    }
    const double wl = sink_clustering_->getWireLength(clusterNodes);
    totalWL += wl;
    clusterCounter++;
  }
}

void Graphics::drawHTree(gui::Painter& painter)
{
  auto color = gui::Painter::red;
  color.a = 180;
  painter.setPen(color, /* cosmetic */ true);

  clock_->forEachSink([&](const ClockInst& sink) {
    int xreal = sink.getX();
    int yreal = sink.getY();
    painter.drawCircle(xreal, yreal, 500);
  });

  Point<double> topLevelBufferLoc
      = h_tree_builder_->getSinkRegion().computeCenter();
  h_tree_builder_->getTopologyVector().front().forEachBranchingPoint(
      [&](unsigned idx, Point<double> branchPoint) {
        const int unit = h_tree_builder_->getWireSegmentUnit();
        const int x1 = unit * topLevelBufferLoc.getX() + 0.5;
        const int y1 = unit * topLevelBufferLoc.getY() + 0.5;
        const int x2 = unit * branchPoint.getX() + 0.5;
        const int y2 = unit * branchPoint.getY() + 0.5;
        painter.drawLine(x1, y1, x2, y2);
      });

  for (int levelIdx = 1; levelIdx < h_tree_builder_->getTopologyVector().size();
       ++levelIdx) {
    h_tree_builder_->getTopologyVector()[levelIdx].forEachBranchingPoint(
        [&](unsigned idx, Point<double> branchPoint) {
          unsigned parentIdx = h_tree_builder_->getTopologyVector()[levelIdx]
                                   .getBranchingPointParentIdx(idx);
          Point<double> parentPoint
              = h_tree_builder_->getTopologyVector()[levelIdx - 1]
                    .getBranchingPoint(parentIdx);
          auto color = gui::Painter::yellow;
          if (levelIdx % 2 == 0) {
            color = gui::Painter::red;
          }
          color.a = 180;
          painter.setPen(color, /* cosmetic */ true);

          const int unit = h_tree_builder_->getWireSegmentUnit();
          const int x1 = unit * parentPoint.getX() + 0.5;
          const int y1 = unit * parentPoint.getY() + 0.5;
          const int x2 = unit * branchPoint.getX() + 0.5;
          const int y2 = unit * branchPoint.getY() + 0.5;
          painter.drawLine(x1, y1, x2, y2);
        });
  }
}

void Graphics::drawObjects(gui::Painter& painter)
{
  if (clock_) {
    drawHTree(painter);
  }

  if (sink_clustering_) {
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
