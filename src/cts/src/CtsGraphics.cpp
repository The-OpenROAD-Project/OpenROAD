// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

#include "CtsGraphics.h"

#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

#include "Clock.h"
#include "Util.h"
#include "gui/gui.h"
#include "odb/geom.h"
#include "utl/Logger.h"

namespace cts {

void CtsGraphics::initializeWithClock(HTreeBuilder* h_tree_builder,
                                      Clock& clock)
{
  clock_ = &clock;
  h_tree_builder_ = h_tree_builder;
  sink_clustering_ = nullptr;
  gui::Gui::get()->registerRenderer(this);
  if (guiActive()) {
    clockPlot(true);
  }
}

void CtsGraphics::initializeWithPoints(SinkClustering* SinkClustering,
                                       const std::vector<Point<double>>& points)
{
  clock_ = nullptr;
  h_tree_builder_ = nullptr;
  sink_clustering_ = SinkClustering;
  points_ = points;
  gui::Gui::get()->registerRenderer(this);
  if (guiActive()) {
    clockPlot(true);
  }
}

void CtsGraphics::drawCluster(gui::Painter& painter)
{
  std::vector<gui::Painter::Color> colors{gui::Painter::kRed,
                                          gui::Painter::kYellow,
                                          gui::Painter::kGreen,
                                          gui::Painter::kDarkRed,
                                          gui::Painter::kMagenta,
                                          gui::Painter::kDarkYellow,
                                          gui::Painter::kBlue,
                                          gui::Painter::kDarkGray,
                                          gui::Painter::kDarkGreen,
                                          gui::Painter::kCyan};

  unsigned clusterCounter = 0;
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
      int xreal = lround(unit * point.getX());
      int yreal = lround(unit * point.getY());

      if (first) {
        first = false;
        first_in_cluster = false;
      } else {
        if (first_in_cluster) {
          first_in_cluster = false;
          painter.setPen(gui::Painter::kWhite, /* cosmetic */ true);
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
    clusterCounter++;
  }
}

void CtsGraphics::drawHTree(gui::Painter& painter)
{
  auto color = gui::Painter::kRed;
  color.a = 180;
  painter.setPen(color, /* cosmetic */ true);

  clock_->forEachSink([&](const ClockInst& sink) {
    int xreal = sink.getX();
    int yreal = sink.getY();
    painter.drawCircle(xreal, yreal, 500);
  });

  Point<double> topLevelBufferLoc
      = h_tree_builder_->getSinkRegion().getCenter();
  h_tree_builder_->getTopologyVector().front().forEachBranchingPoint(
      [&](unsigned idx, Point<double> branchPoint) {
        const int unit = h_tree_builder_->getWireSegmentUnit();
        const int x1 = lround(unit * topLevelBufferLoc.getX());
        const int y1 = lround(unit * topLevelBufferLoc.getY());
        const int x2 = lround(unit * branchPoint.getX());
        const int y2 = lround(unit * branchPoint.getY());
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
          auto color = gui::Painter::kYellow;
          if (levelIdx % 2 == 0) {
            color = gui::Painter::kRed;
          }
          color.a = 180;
          painter.setPen(color, /* cosmetic */ true);

          const int unit = h_tree_builder_->getWireSegmentUnit();
          const int x1 = lround(unit * parentPoint.getX());
          const int y1 = lround(unit * parentPoint.getY());
          const int x2 = lround(unit * branchPoint.getX());
          const int y2 = lround(unit * branchPoint.getY());
          painter.drawLine(x1, y1, x2, y2);
        });
  }
}

void CtsGraphics::drawObjects(gui::Painter& painter)
{
  if (clock_) {
    drawHTree(painter);
  }

  if (sink_clustering_) {
    drawCluster(painter);
  }
}

void CtsGraphics::clockPlot(bool pause)
{
  gui::Gui::get()->redraw();
  if (pause) {
    gui::Gui::get()->pause();
  }
}

void CtsGraphics::status(const std::string& message)
{
  gui::Gui::get()->status(message);
}

/* static */
bool CtsGraphics::guiActive()
{
  return gui::Gui::enabled();
}

}  // namespace cts
