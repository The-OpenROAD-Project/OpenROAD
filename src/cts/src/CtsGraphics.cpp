/////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2022, The Regents of the University of California
// All rights reserved.
//
// BSD 3-Clause License
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
///////////////////////////////////////////////////////////////////////////////

#include "CtsGraphics.h"

#include <algorithm>
#include <cstdio>
#include <limits>

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
    clusterCounter++;
  }
}

void CtsGraphics::drawHTree(gui::Painter& painter)
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
          auto color = gui::Painter::yellow;
          if (levelIdx % 2 == 0) {
            color = gui::Painter::red;
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
