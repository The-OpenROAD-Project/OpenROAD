// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "HTreeBuilder.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <limits>
#include <string>
#include <utility>
#include <vector>

#include "Clock.h"
#include "Clustering.h"
#include "CtsObserver.h"
#include "SinkClustering.h"
#include "TechChar.h"
#include "TreeBuilder.h"
#include "Util.h"
#include "odb/db.h"
#include "odb/isotropy.h"
#include "utl/Logger.h"

namespace cts {

using utl::CTS;

Point<double> HTreeBuilder::legalizeOneBuffer(Point<double> bufferLoc,
                                              const std::string& bufferName)
{
  Point<double> legalLoc
      = TreeBuilder::legalizeOneBuffer(bufferLoc, bufferName);
  return resolveLocationCollision(legalLoc);
}

void HTreeBuilder::preSinkClustering(
    const std::vector<std::pair<float, float>>& sinks,
    const std::vector<const ClockInst*>& sinkInsts,
    const float maxDiameter,
    const unsigned clusterSize,
    const bool secondLevel)
{
  bool maxDiameterSet = (type_ == TreeType::MacroTree)
                            ? options_->isMacroMaxDiameterSet()
                            : options_->isMaxDiameterSet();
  bool clusterSizeSet = (type_ == TreeType::MacroTree)
                            ? options_->isMacroSinkClusteringSizeSet()
                            : options_->isSinkClusteringSizeSet();

  unsigned min_clustering_sinks = (type_ == TreeType::MacroTree)
                                      ? min_clustering_macro_sinks_
                                      : min_clustering_sinks_;

  const std::vector<std::pair<float, float>>& points = sinks;
  if (!secondLevel) {
    clock_.forEachSink([&](ClockInst& inst) {
      const Point<double> normLocation((float) inst.getX() / wireSegmentUnit_,
                                       (float) inst.getY() / wireSegmentUnit_);
      mapLocationToSink_[normLocation] = &inst;
      if (!fuzzyEqual(inst.getInsertionDelay(), 0.0, 1e-6)) {
        setSinkInsertionDelay(normLocation,
                              inst.getInsertionDelay() / wireSegmentUnit_);
        // clang-format off
	debugPrint(logger_, CTS, "clustering", 1, "sink {} has insDelay {} at {}",
		   inst.getName(), getSinkInsertionDelay(normLocation),
		   normLocation);
        // clang-format on
      }
    });
  }

  if (sinks.size() <= min_clustering_sinks
      || !(options_->getSinkClustering())) {
    topLevelSinksClustered_ = sinks;
    return;
  }

  SinkClustering matching(options_, techChar_, this);
  const unsigned numPoints = points.size();

  for (int pointIdx = 0; pointIdx < numPoints; ++pointIdx) {
    const std::pair<float, float>& point = points[pointIdx];
    matching.addPoint(point.first, point.second);
    if (sinkInsts[pointIdx]->getInputCap() == 0) {
      // Comes here in second level since first level buf cap is not set
      matching.addCap(options_->getSinkBufferInputCap());
    } else {
      matching.addCap(sinkInsts[pointIdx]->getInputCap());
    }
  }

  unsigned bestClusterSize = 0;
  float bestDiameter = 0.0;
  if (clusterSizeSet && maxDiameterSet) {
    // clang-format off
      debugPrint(logger_, CTS, "clustering", 1, "**** match.run({}, {}, {}) ****",
                 clusterSize, maxDiameter, wireSegmentUnit_);
    // clang-format on
    matching.run(clusterSize,
                 maxDiameter,
                 wireSegmentUnit_,
                 bestClusterSize,
                 bestDiameter);
  } else if (!clusterSizeSet && maxDiameterSet) {
    // only diameter is set, try clustering sizes of 10, 20 and 30
    for (unsigned clusterSize2 : options_->getSinkClusteringSizes()) {
      // clang-format off
      debugPrint(logger_, CTS, "clustering", 1, "**** match.run({}, {}, {}) ****",
                 clusterSize2, maxDiameter, wireSegmentUnit_);
      // clang-format on
      matching.run(clusterSize2,
                   maxDiameter,
                   wireSegmentUnit_,
                   bestClusterSize,
                   bestDiameter);
    }
  } else if (clusterSizeSet && !maxDiameterSet) {
    // only clustering size is set, try diameters of 50, 100 and 200 um
    for (unsigned clusterDiameter2 : options_->getSinkClusteringDiameters()) {
      // clang-format off
      debugPrint(logger_, CTS, "clustering", 1, "**** match.run({}, {}, {}) ****",
                 clusterSize, clusterDiameter2, wireSegmentUnit_);
      // clang-format on
      float maxDiameter2 = clusterDiameter2 * (float) options_->getDbUnits()
                           / wireSegmentUnit_;
      matching.run(clusterSize,
                   maxDiameter2,
                   wireSegmentUnit_,
                   bestClusterSize,
                   bestDiameter);
    }
  } else {  // neighther clustering size nor diameter is set
    // try diameters of 50, 100 and 200 um
    for (unsigned clusterDiameter2 : clusterDiameters()) {
      // try clustering sizes of 10, 20 and 30
      for (unsigned clusterSize2 : options_->getSinkClusteringSizes()) {
        // clang-format off
        debugPrint(logger_, CTS, "clustering", 1, "**** match.run({}, {}, {}) ****",
                   clusterSize2, clusterDiameter2, wireSegmentUnit_);
        // clang-format on
        float maxDiameter2 = clusterDiameter2 * (float) options_->getDbUnits()
                             / wireSegmentUnit_;
        matching.run(clusterSize2,
                     maxDiameter2,
                     wireSegmentUnit_,
                     bestClusterSize,
                     bestDiameter);
      }
    }
  }

  if (clusterSizeSet || maxDiameterSet) {
    logger_->info(
        CTS,
        204,
        "A clustering solution was found from clustering size of {} "
        "and clustering diameter of {:0.0f}.",
        bestClusterSize,
        std::round(bestDiameter / options_->getDbUnits() * wireSegmentUnit_));
    logger_->info(CTS,
                  205,
                  "Better solution may be possible if either "
                  "-sink_clustering_size, -sink_clustering_max_diameter, or "
                  "both options are omitted to enable automatic clustering.");
  } else {
    logger_->info(
        CTS,
        206,
        "Best clustering solution was found from clustering size of {} "
        "and clustering diameter of {:0.0f}.",
        bestClusterSize,
        std::round(bestDiameter / options_->getDbUnits() * wireSegmentUnit_));
  }

  unsigned clusterCount = 0;

  std::vector<std::pair<float, float>> newSinkLocations;
  for (const std::vector<unsigned>& cluster :
       matching.sinkClusteringSolution()) {
    if (cluster.size() == 1) {
      const std::pair<float, float>& point = points[cluster[0]];
      newSinkLocations.emplace_back(point);
    }
    if (cluster.size() > 1) {
      std::vector<ClockInst*> clusterClockInsts;  // sink clock insts
      float xSum = 0;
      float ySum = 0;
      for (auto point_idx : cluster) {
        const std::pair<double, double>& point = points[point_idx];
        const Point<double> mapPoint(point.first, point.second);
        if (mapLocationToSink_.find(mapPoint) == mapLocationToSink_.end()) {
          logger_->error(CTS, 79, "Sink not found.");
        }
        xSum += point.first;
        ySum += point.second;
        clusterClockInsts.push_back(mapLocationToSink_[mapPoint]);
        // clock inst needs to be added to the new subnet
      }
      const unsigned pointCounter = cluster.size();
      const float normCenterX
          = (xSum / (float) pointCounter);  // geometric center of cluster
      const float normCenterY = (ySum / (float) pointCounter);
      Point<double> center((double) normCenterX, (double) normCenterY);
      Point<double> rootBufLoc
          = legalizeOneBuffer(center, options_->getSinkBuffer());
      commitMoveLoc(center, rootBufLoc);

      const char* baseName = secondLevel ? "clkbuf_leaf2_" : "clkbuf_leaf_";
      ClockInst& rootBuffer
          = clock_.addClockBuffer(baseName + std::to_string(clusterCount),
                                  options_->getSinkBuffer(),
                                  rootBufLoc.getX() * wireSegmentUnit_,
                                  rootBufLoc.getY() * wireSegmentUnit_);
      if (center != rootBufLoc) {
        debugPrint(logger_,
                   CTS,
                   "legalizer",
                   2,
                   "preSinkClustering legalizeOneBuffer {}: {} => {}",
                   baseName + std::to_string(clusterCount),
                   center,
                   rootBufLoc);
      }

      if (!secondLevel) {
        addFirstLevelSinkDriver(&rootBuffer);
      } else {
        addSecondLevelSinkDriver(&rootBuffer);
      }

      baseName = secondLevel ? "clknet_leaf2_" : "clknet_leaf_";
      ClockSubNet& clockSubNet
          = clock_.addSubNet(baseName + std::to_string(clusterCount));
      // Subnet that connects the new -sink- buffer to each specific sink
      clockSubNet.addInst(rootBuffer);
      for (ClockInst* clockInstObj : clusterClockInsts) {
        clockSubNet.addInst(*clockInstObj);
      }
      if (!secondLevel) {
        clockSubNet.setLeafLevel(true);
      }

      const std::pair<float, float> point(rootBufLoc.getX(), rootBufLoc.getY());
      newSinkLocations.emplace_back(point);

      // Simulate the float conversion to ensure consistent map keys
      Point<double> mapKey(point.first, point.second);

      mapLocationToSink_[mapKey] = &rootBuffer;
    }
    clusterCount++;
  }
  topLevelSinksClustered_ = std::move(newSinkLocations);
  if (clusterCount) {
    treeBufLevels_++;
  }

  logger_->info(CTS,
                19,
                " Total number of sinks after clustering: {}.",
                topLevelSinksClustered_.size());
}

Point<double> HTreeBuilder::resolveLocationCollision(
    const Point<double>& legalCenter) const
{
  Point<double> resolvedLocation = legalCenter;

  // Collision check and jittering to ensure unique coordinates
  unsigned jitterCount = 0;
  while (true) {
    // Simulate the float conversion that happens when storing in
    // newSinkLocations. This is needed because the existing logic uses the
    // double->float conversion for the center location.
    Point<double> checkKey((float) resolvedLocation.getX(),
                           (float) resolvedLocation.getY());
    if (mapLocationToSink_.find(checkKey) == mapLocationToSink_.end()) {
      break;
    }
    jitterCount++;
    double offset = (double) jitterCount / wireSegmentUnit_;
    resolvedLocation.setX(legalCenter.getX() + offset);
  }

  if (jitterCount > 0) {
    debugPrint(
        logger_,
        utl::CTS,
        "clustering",
        1,
        "Resolved collision via jittering ({} times): ({}, {}) -> ({}, {})",
        jitterCount,
        legalCenter.getX(),
        legalCenter.getY(),
        resolvedLocation.getX(),
        resolvedLocation.getY());
  }
  return resolvedLocation;
}

void HTreeBuilder::initSinkRegion()
{
  const unsigned wireSegmentUnitInDbu = techChar_->getLengthUnit();
  const int dbUnits = options_->getDbUnits();
  wireSegmentUnit_ = wireSegmentUnitInDbu;

  double clusterDiameter = (type_ == TreeType::MacroTree)
                               ? options_->getMacroMaxDiameter()
                               : options_->getMaxDiameter();
  unsigned clusterSize = (type_ == TreeType::MacroTree)
                             ? options_->getMacroSinkClusteringSize()
                             : options_->getSinkClusteringSize();
  unsigned min_clustering_sinks = (type_ == TreeType::MacroTree)
                                      ? min_clustering_macro_sinks_
                                      : min_clustering_sinks_;

  logger_->info(CTS,
                20,
                " Wire segment unit: {}  dbu ({} um).",
                wireSegmentUnit_,
                wireSegmentUnitInDbu / dbUnits);

  if (options_->isSimpleSegmentEnabled()) {
    const int remainingLength
        = options_->getBufferDistance() / (wireSegmentUnitInDbu * 2);
    logger_->info(CTS,
                  21,
                  " Distance between buffers: {} units ({} um).",
                  remainingLength,
                  static_cast<int>(options_->getBufferDistance() / dbUnits));
    if (options_->isVertexBuffersEnabled()) {
      const int vertexBufferLength
          = options_->getVertexBufferDistance() / (wireSegmentUnitInDbu * 2);
      logger_->info(
          CTS,
          22,
          " Branch length for Vertex Buffer: {} units ({} um).",
          vertexBufferLength,
          static_cast<int>(options_->getVertexBufferDistance() / dbUnits));
    }
  }

  std::vector<std::pair<float, float>> topLevelSinks;
  std::vector<const ClockInst*> sinkInsts;
  initTopLevelSinks(topLevelSinks, sinkInsts);

  const float maxDiameter = (clusterDiameter * dbUnits) / wireSegmentUnit_;
  // clang-format off
  debugPrint(logger_, CTS, "clustering", 1, "maxDiameter={:0.3f} = "
             "origMaxDiam={} * dbUnits={} / wireSegmentUnit_={}",
             maxDiameter, clusterDiameter, dbUnits,
             wireSegmentUnit_);
  // clang-format on

  preSinkClustering(topLevelSinks, sinkInsts, maxDiameter, clusterSize);
  if (topLevelSinks.size() <= min_clustering_sinks
      || !(options_->getSinkClustering())) {
    Box<int> sinkRegionDbu = clock_.computeSinkRegion();
    logger_->info(CTS, 23, " Original sink region: {}.", sinkRegionDbu);

    sinkRegion_ = sinkRegionDbu.normalize(1.0 / wireSegmentUnit_);
  } else {
    if (topLevelSinksClustered_.size() > 400
        && options_->getSinkClusteringLevels() > 0) {
      std::vector<std::pair<float, float>> secondLevelLocs;
      std::vector<const ClockInst*> secondLevelInsts;
      initSecondLevelSinks(secondLevelLocs, secondLevelInsts);
      preSinkClustering(secondLevelLocs,
                        secondLevelInsts,
                        maxDiameter * 4,
                        std::ceil(std::sqrt(clusterSize)),
                        true);
    }
    sinkRegion_ = clock_.computeSinkRegionClustered(topLevelSinksClustered_);
  }
  logger_->info(CTS, 24, " Normalized sink region: {}.", sinkRegion_);
  logger_->info(CTS, 25, "    Width:  {:.4f}.", sinkRegion_.getWidth());
  logger_->info(CTS, 26, "    Height: {:.4f}.", sinkRegion_.getHeight());
}

static void plotBlockage(std::ofstream& file,
                         odb::dbDatabase* db_,
                         int scalingFactor)
{
  unsigned i = 0;
  for (odb::dbBlockage* blockage : db_->getChip()->getBlock()->getBlockages()) {
    odb::dbBox* bbox = blockage->getBBox();
    int x = bbox->xMin() / scalingFactor;
    int y = bbox->yMin() / scalingFactor;
    int w = bbox->xMax() / scalingFactor - bbox->xMin() / scalingFactor;
    int h = bbox->yMax() / scalingFactor - bbox->yMin() / scalingFactor;
    file << i++ << " " << x << " " << y << " " << w << " " << h
         << " block  scalingFactor=";
    file << scalingFactor << " " << blockage->getId() << '\n';
  }
}

// distance to move sinks from old loc to new loc
double HTreeBuilder::weightedDistance(const Point<double>& newLoc,
                                      const Point<double>& oldLoc,
                                      const std::vector<Point<double>>& sinks)
{
  double dist = 0;
  for (const Point<double>& sink : sinks) {
    dist += computeDist(newLoc, sink);
    dist += computeDist(newLoc, oldLoc);
  }
  return dist;
}

static void plotSinks(std::ofstream& file,
                      const std::vector<Point<double>>& sinks)
{
  unsigned cnt = 0;
  for (const Point<double>& pt : sinks) {
    double x = pt.getX();
    double y = pt.getY();
    double w = 1;
    double h = 1;
    auto name = "sink_";
    file << cnt++ << " " << x << " " << y << " " << w << " " << h;
    file << " " << name << '\n';
  }
}

unsigned HTreeBuilder::findSibling(LevelTopology& topology,
                                   unsigned i,
                                   unsigned par)
{
  for (unsigned idx = 0; idx < topology.getBranchingPointSize(); ++idx) {
    unsigned k = topology.getBranchingPointParentIdx(idx);
    if (idx != i && k == par) {
      return idx;
    }
  }
  return i;
}

void HTreeBuilder::scalePosition(Point<double>& loc,
                                 const Point<double>& parLoc,
                                 double leng,
                                 double scale)
{
  double px = parLoc.getX();
  double py = parLoc.getY();
  double ax = loc.getX();
  double ay = loc.getY();

  double d = computeDist(loc, parLoc);
  double x, y;
  if (d > 0) {  // yy8
    double delta = d * scale;
    double dx = ax - px;
    double dy = ay - py;
    dx += (dx > 0) ? delta : -delta;
    dy += (dy > 0) ? -delta : delta;
    double scale = leng / d;
    x = px + dx * scale;
    y = py + dy * scale;
  } else {
    x = px + leng / 2;
    y = py + leng / 2;
  }
  loc.setX(x);
  loc.setY(y);
}

static void setSiblingPosition(const Point<double>& a,
                               Point<double>& b,
                               const Point<double>& parLoc)
{
  double px = parLoc.getX();
  double py = parLoc.getY();
  double ax = a.getX();
  double ay = a.getY();
  double bx = 2 * px - ax;
  double by = 2 * py - ay;
  b.setX(bx);
  b.setY(by);
}

// Balance the two branches on the very top level
void HTreeBuilder::adjustToplevelTopology(Point<double>& a,
                                          Point<double>& b,
                                          const Point<double>& parLoc)
{
  double da = computeDist(a, parLoc);
  double db = computeDist(b, parLoc);
  if (da < db) {
    setSiblingPosition(a, b, parLoc);
  } else {
    setSiblingPosition(b, a, parLoc);
  }
}

void HTreeBuilder::findLegalLocations(const Point<double>& parentPoint,
                                      const Point<double>& branchPoint,
                                      double x1,
                                      double y1,
                                      double x2,
                                      double y2,
                                      std::vector<Point<double>>& points)
{
  // add 4 corners of blockage
  addCandidateLoc(x1, y1, parentPoint, x1, y1, x2, y2, points);
  addCandidateLoc(x1, y2, parentPoint, x1, y1, x2, y2, points);
  addCandidateLoc(x2, y2, parentPoint, x1, y1, x2, y2, points);
  addCandidateLoc(x2, y1, parentPoint, x1, y1, x2, y2, points);

  // add straightline neighbors
  double bx = branchPoint.getX();
  double by = branchPoint.getY();
  addCandidateLoc(bx, y1, parentPoint, x1, y1, x2, y2, points);
  addCandidateLoc(bx, y2, parentPoint, x1, y1, x2, y2, points);
  addCandidateLoc(x1, by, parentPoint, x1, y1, x2, y2, points);
  addCandidateLoc(x2, by, parentPoint, x1, y1, x2, y2, points);

  double px = parentPoint.getX();
  double py = parentPoint.getY();
  double dx = px - bx;
  double dy = py - by;
  double m = dy / dx;
  // y = m*(x-bx) + by
  addCandidateLoc(x1, m * (x1 - bx) + by, parentPoint, x1, y1, x2, y2, points);
  addCandidateLoc(x2, m * (x2 - bx) + by, parentPoint, x1, y1, x2, y2, points);
  // x = (y-by)/m + bx
  addCandidateLoc((y1 - by) / m + bx, y1, parentPoint, x1, y1, x2, y2, points);
  addCandidateLoc((y2 - by) / m + bx, y2, parentPoint, x1, y1, x2, y2, points);
  // clang-format off
  if (logger_->debugCheck(utl::CTS, "legalizer", 3)) {
    logger_->report("    branchPt:{} is not legal, parentPt:{} blockages:({:0.3f} {:0.3f}) "
        "({:0.3f} {:0.3f})", branchPoint, parentPoint, x1, y1, x2, y2);
    for (Point<double> point : points) {
      logger_->report("      FLL candiate {}", point);
    }
    // clang-format on
  }
}

Point<double> HTreeBuilder::findBestLegalLocation(
    double targetDist,
    const Point<double>& branchPoint,
    const Point<double>& parentPoint,
    const std::vector<Point<double>>& legalLocations,
    const std::vector<Point<double>>& sinks,
    double x1,
    double y1,
    double x2,
    double y2,
    int scalingFactor,
    odb::Direction2D direction)
{
  Point<double> best(0.0, 0.0);
  double minDiff = std::numeric_limits<double>::max();
  for (const Point<double>& loc : legalLocations) {
    double dist = computeDist(loc, parentPoint);
    dist += computeDist(loc, branchPoint);
    double diff = abs(dist - targetDist);
    // clang-format off
    if (logger_->debugCheck(utl::CTS, "legalizer", 3)) {
      logger_->report("      Loc {}: curr dist={:0.3f} target dist={:0.3f} sink"
		      " dist={:0.3f}", loc, dist, targetDist,
		      weightedDistance(loc, branchPoint, sinks));
    }
    // clang-format on
    if (diff < minDiff) {
      minDiff = diff;
      best = loc;
    }
  }

  return adjustBestLegalLocation(targetDist,
                                 best,
                                 parentPoint,
                                 sinks,
                                 x1,
                                 y1,
                                 x2,
                                 y2,
                                 scalingFactor,
                                 direction);
}

// Adjust buffer location in two steps:
// step1: try moving buffer to existing blockage boundary (less expensive)
// step2: try moving buffer beyond existing blockage boundary (more expensive)
// In both steps, the first priority is to match target distance from current
// point to parent point.  The second priority is to lower the weighted sink
// distance
Point<double> HTreeBuilder::adjustBestLegalLocation(
    double targetDist,
    const Point<double>& currLoc,
    const Point<double>& parentPoint,
    const std::vector<Point<double>>& sinks,
    double x1,
    double y1,
    double x2,
    double y2,
    int scalingFactor,
    odb::Direction2D direction)
{
  if (fuzzyEqual(targetDist, computeDist(currLoc, parentPoint))) {
    return currLoc;
  }

  // try moving along blockage boundary
  Point<double> bestLoc = currLoc;
  if (adjustAlongBlockage(targetDist,
                          currLoc,
                          parentPoint,
                          sinks,
                          x1,
                          y1,
                          x2,
                          y2,
                          scalingFactor,
                          bestLoc)) {
    return bestLoc;
  }

  // try moving beyond blockage boundary
  return adjustBeyondBlockage(
      currLoc, parentPoint, targetDist, sinks, scalingFactor, direction);
}

bool HTreeBuilder::adjustAlongBlockage(double targetDist,
                                       const Point<double>& currLoc,
                                       const Point<double>& parentPoint,
                                       const std::vector<Point<double>>& sinks,
                                       double x1,
                                       double y1,
                                       double x2,
                                       double y2,
                                       int scalingFactor,
                                       Point<double>& bestLoc)
{
  Point<double> newLoc = currLoc;
  // clang-format off
  debugPrint(logger_, CTS, "legalizer", 3, "{} currDist={:0.3f} != "
	     "targetDist={:0.3f}, adjustAlongBlockage...", currLoc,
	     computeDist(currLoc, parentPoint), targetDist);
  // clang-format on
  double x = currLoc.getX();
  double y = currLoc.getY();
  double px = parentPoint.getX();
  double py = parentPoint.getY();

  Point<double> noBest(std::numeric_limits<double>::min(),
                       std::numeric_limits<double>::min());
  bestLoc = noBest;
  double bestSinkDist = std::numeric_limits<double>::max();
  double sinkDist = 0.0;
  std::vector<Point<double>> candidates;

  // move along y axis within blockage
  double newX = x;
  double newY = py - targetDist + abs(px - newX);
  newLoc.setX(newX);
  newLoc.setY(newY);
  candidates.emplace_back(newLoc);  // trial y#1

  newY = py + targetDist - abs(px - newX);
  newLoc.setY(newY);
  candidates.emplace_back(newLoc);  // trial y#2

  // move along x axis within blockage
  newY = y;
  newX = px - targetDist + abs(py - newY);
  newLoc.setX(newX);
  newLoc.setY(newY);
  candidates.emplace_back(newLoc);  // trial x#1

  newX = px + targetDist - abs(py - newY);
  newLoc.setX(newX);
  candidates.emplace_back(newLoc);  // trial x#2

  for (const Point<double>& candidate : candidates) {
    checkLegalityAndCostSpecial(currLoc,
                                candidate,
                                parentPoint,
                                targetDist,
                                sinks,
                                scalingFactor,
                                x1,
                                y1,
                                x2,
                                y2,
                                bestLoc,
                                sinkDist,
                                bestSinkDist);
  }

  return (bestLoc != noBest);
}

void HTreeBuilder::checkLegalityAndCostSpecial(
    const Point<double>& oldLoc,
    const Point<double>& newLoc,
    const Point<double>& parentPoint,
    double targetDist,
    const std::vector<Point<double>>& sinks,
    int scalingFactor,
    double x1,
    double y1,
    double x2,
    double y2,
    Point<double>& bestLoc,
    double& sinkDist,
    double& bestSinkDist)
{
  if (fuzzyEqual(computeDist(newLoc, parentPoint), targetDist)
      && checkLegalitySpecial(newLoc, x1, y1, x2, y2, scalingFactor)) {
    sinkDist = weightedDistance(newLoc, oldLoc, sinks);
    if (sinkDist < bestSinkDist) {
      bestLoc = newLoc;
      bestSinkDist = sinkDist;
    }
    // clang-format off
    debugPrint(logger_, CTS, "legalizer", 3, "adjustBestLegalLoc: branchPt "
	       "move:{}=>{} is legal, dist={:0.3f}, sinkDist={:0.3f}",
	       oldLoc, newLoc, targetDist, sinkDist);
    // clang-format on
  }
  // clang-format off
  debugPrint(logger_, CTS, "legalizer", 3, "adjustBestLegalLoc: branchPt "
	     "move:{}=>{} is illegal or dist {:0.3f} != {:0.3f}",
	     oldLoc, newLoc, computeDist(newLoc, parentPoint), targetDist);
  // clang-format on
}

// 1) Branch point couldn't be legalized by simply moving it along blockage
//    boundary, or
// 2) Branch point is legal but parent point has moved, so it is necessary to
//    move it to match target topology length
// In both cases, move branch point to match target distance and minimize
// weighted sink distance
Point<double> HTreeBuilder::adjustBeyondBlockage(
    const Point<double>& branchPoint,
    const Point<double>& parentPoint,
    double targetDist,
    const std::vector<Point<double>>& sinks,
    int scalingFactor,
    odb::Direction2D direction)
{
  double px = parentPoint.getX();
  double py = parentPoint.getY();
  std::vector<Point<double>> candidates;
  Point<double> point = branchPoint;

  // try points that are on edges of "Manhattan square"
  // with the parent point in the center
  //              p16 p1 p9
  //            p8         p5
  //          p15            p10
  //        p4        pp       p2
  //         p14             p11
  //           p7           p6
  //              p13 p3 p12
  double leng50 = targetDist * 0.5;
  double leng25 = targetDist * 0.25;
  double leng75 = targetDist * 0.75;
  switch (direction) {
    case odb::Direction2D::North:
      addCandidatePoint(px, py + targetDist, point, candidates);       // p1
      addCandidatePoint(px + leng50, py + leng50, point, candidates);  // p5
      addCandidatePoint(px - leng50, py + leng50, point, candidates);  // p8
      addCandidatePoint(px + leng25, py + leng75, point, candidates);  // p9
      addCandidatePoint(px - leng25, py + leng75, point, candidates);  // p16
      break;
    case odb::Direction2D::East:
      addCandidatePoint(px + targetDist, py, point, candidates);       // p2
      addCandidatePoint(px + leng50, py + leng50, point, candidates);  // p5
      addCandidatePoint(px + leng50, py - leng50, point, candidates);  // p6
      addCandidatePoint(px + leng75, py + leng25, point, candidates);  // p10
      addCandidatePoint(px + leng75, py - leng25, point, candidates);  // p11
      break;
    case odb::Direction2D::South:
      addCandidatePoint(px, py - targetDist, point, candidates);       // p3
      addCandidatePoint(px + leng50, py - leng50, point, candidates);  // p6
      addCandidatePoint(px - leng50, py - leng50, point, candidates);  // p7
      addCandidatePoint(px + leng25, py - leng75, point, candidates);  // p12
      addCandidatePoint(px - leng25, py - leng75, point, candidates);  // p13
      break;
    default:
      addCandidatePoint(px - targetDist, py, point, candidates);       // p4
      addCandidatePoint(px - leng50, py - leng50, point, candidates);  // p7
      addCandidatePoint(px - leng50, py + leng50, point, candidates);  // p8
      addCandidatePoint(px - leng75, py - leng25, point, candidates);  // p14
      addCandidatePoint(px - leng75, py + leng25, point, candidates);  // p15
      // odb::Direction2D::West
      break;
  }

  // check if any corners of Manhanttan square are inside some blockage
  // if so add candidate points that intersect blockage and Manhattan square
  // p1 is top corner

  if (direction == odb::Direction2D(odb::Direction2D::North)) {
    point.setX(px);
    point.setY(py + targetDist);
    addCandidatePointsAlongBlockage(point,
                                    parentPoint,
                                    targetDist,
                                    scalingFactor,
                                    candidates,
                                    odb::Direction2D::North);
  }

  // p2 is right corner
  if (direction == odb::Direction2D(odb::Direction2D::East)) {
    point.setX(px + targetDist);
    point.setY(py);
    addCandidatePointsAlongBlockage(point,
                                    parentPoint,
                                    targetDist,
                                    scalingFactor,
                                    candidates,
                                    odb::Direction2D::East);
  }

  // p3 is bottom corner
  if (direction == odb::Direction2D(odb::Direction2D::South)) {
    point.setX(px);
    point.setY(py - targetDist);
    addCandidatePointsAlongBlockage(point,
                                    parentPoint,
                                    targetDist,
                                    scalingFactor,
                                    candidates,
                                    odb::Direction2D::South);
  }

  // p4 is left corner
  if (direction == odb::Direction2D(odb::Direction2D::West)) {
    point.setX(px - targetDist);
    point.setY(py);
    addCandidatePointsAlongBlockage(point,
                                    parentPoint,
                                    targetDist,
                                    scalingFactor,
                                    candidates,
                                    odb::Direction2D::West);
  }

  // try moving cell along x or y, with some offset
  double bx = branchPoint.getX();
  double by = branchPoint.getY();
  Point<double> noBest(std::numeric_limits<double>::min(),
                       std::numeric_limits<double>::min());
  Point<double> bestLoc = noBest;
  double bestSinkDist = std::numeric_limits<double>::max();
  double sinkDist = 0.0;
  // get small x and y offset to find "channel" through blockages
  double minX = 10.0 * getBufferWidth();
  double minY = 10.0 * getBufferHeight();

  // try small offset in x direction
  Point<double> newLoc(bx, by);
  double newX = bx + minX;
  double newY = py - targetDist + abs(px - newX);
  newLoc.setX(newX);
  newLoc.setY(newY);
  if (direction == odb::Direction2D(odb::Direction2D::South)) {
    candidates.emplace_back(newLoc);  // trial x#1
  }

  newLoc.setY(py + targetDist - abs(px - newX));
  if (direction == odb::Direction2D(odb::Direction2D::North)) {
    candidates.emplace_back(newLoc);  // trial x#2
  }

  newX = bx - minX;
  newY = py - targetDist + abs(px - newX);
  newLoc.setX(newX);
  newLoc.setY(newY);
  if (direction == odb::Direction2D(odb::Direction2D::South)) {
    candidates.emplace_back(newLoc);  // trial x#3
  }

  newLoc.setY(py + targetDist - abs(px - newX));
  if (direction == odb::Direction2D(odb::Direction2D::North)) {
    candidates.emplace_back(newLoc);  // trial x#4
  }

  // try small offset in y direction
  newY = by + minY;
  newX = px - targetDist + abs(py - newY);
  newLoc.setX(newX);
  newLoc.setY(newY);
  if (direction == odb::Direction2D(odb::Direction2D::West)) {
    candidates.emplace_back(newLoc);  // trial y#1
  }

  newLoc.setX(px + targetDist - abs(py - newY));
  if (direction == odb::Direction2D(odb::Direction2D::East)) {
    candidates.emplace_back(newLoc);  // trial y#2
  }

  newY = by - minY;
  newX = px - targetDist + abs(py - newY);
  newLoc.setX(newX);
  newLoc.setY(newY);
  if (direction == odb::Direction2D(odb::Direction2D::West)) {
    candidates.emplace_back(newLoc);  // trial y#3
  }

  newLoc.setX(px + targetDist - abs(py - newY));
  if (direction == odb::Direction2D(odb::Direction2D::East)) {
    candidates.emplace_back(newLoc);  // trial y#4
  }

  for (const Point<double>& candidate : candidates) {
    checkLegalityAndCost(branchPoint,
                         candidate,
                         parentPoint,
                         targetDist,
                         sinks,
                         scalingFactor,
                         bestLoc,
                         sinkDist,
                         bestSinkDist);
  }

  if (bestLoc != noBest) {
    return bestLoc;
  }
  return branchPoint;
}

void HTreeBuilder::addCandidatePointsAlongBlockage(
    const Point<double>& point,
    const Point<double>& parentPoint,
    double targetDist,
    int scalingFactor,
    std::vector<Point<double>>& candidates,
    odb::Direction2D direction)
{
  double x1, y1, x2, y2;
  if (findBlockage(point, scalingFactor, x1, y1, x2, y2)) {
    Point<double> point2 = point;
    double px = parentPoint.getX();
    double py = parentPoint.getY();
    // clang-format off
    debugPrint(logger_, CTS, "legalizer", 3, "  {} corner {} of Manhattan "
	       "sqaure is inside blockage ({:0.3f} {:0.3f}) ({:0.3f} {:0.3f})",
	       direction, point, x1, y1, x2, y2);
    // clang-format on
    switch (direction) {
      case odb::Direction2D::North:
        //        ----------
        //        |        |
        //        |   top  |
        //        |   / \  |
        // (x1, y1)--x---x-(x2, y1)
        //          /  .
        //          (px, py)
        //
        // (px - x) + (y1 - py) = targetDist
        // (x - px) + (y1 - py) = targetDist
        point2.setX(-targetDist - py + px + y1);
        point2.setY(y1);
        candidates.emplace_back(point2);
        point2.setY(targetDist + py + px - y1);
        candidates.emplace_back(point2);
        break;
      case odb::Direction2D::East:
        // (x1, y2)---------
        //       \ |        |
        //         x        |
        //         |\       |
        //   .     | right  |
        // (px,py)| /       |
        //         x        |
        // (x1, y1)---------
        // (x1 - px) + (y - py) = targetDist
        // (x1 - px) + (py - y) = targetDist
        point2.setX(x1);
        point2.setY(targetDist + py + px - x1);
        candidates.emplace_back(point2);
        point2.setY(-targetDist + py - px + x1);
        candidates.emplace_back(point2);
        break;
      case odb::Direction2D::South:
        point2.setX(-targetDist + py + px - y2);
        point2.setY(y2);
        candidates.emplace_back(point2);
        point2.setX(targetDist - py + px + y2);
        candidates.emplace_back(point2);
        break;
      default:
        // odb::Direction2D::West
        point2.setX(x2);
        point2.setY(targetDist + py - px + x2);
        candidates.emplace_back(point2);
        point2.setY(-targetDist + py + px - x2);
        candidates.emplace_back(point2);
        break;
    }
  }
}

void HTreeBuilder::checkLegalityAndCost(const Point<double>& oldLoc,
                                        const Point<double>& newLoc,
                                        const Point<double>& parentPoint,
                                        double targetDist,
                                        const std::vector<Point<double>>& sinks,
                                        int scalingFactor,
                                        Point<double>& bestLoc,
                                        double& sinkDist,
                                        double& bestSinkDist)
{
  if (fuzzyEqual(computeDist(newLoc, parentPoint), targetDist)
      && checkLegalityLoc(newLoc, scalingFactor)) {
    sinkDist = weightedDistance(newLoc, oldLoc, sinks);
    if (sinkDist < bestSinkDist) {
      bestLoc = newLoc;
      bestSinkDist = sinkDist;
    }
    // clang-format off
    debugPrint(logger_, CTS, "legalizer", 3, "adjustBeyondBlockage: branchPt "
	       "move:{}=>{} is legal, dist={:0.3f}, sinkDist={:0.3f}",
	       oldLoc, newLoc, targetDist, sinkDist);
  } else {
    debugPrint(logger_, CTS, "legalizer", 3, "adjustBeyondBlockage: branchPt "
	       "move:{}=>{} is illegal or dist {:0.3f} != {:0.3f}",
	       oldLoc, newLoc, computeDist(newLoc, parentPoint), targetDist);
    // clang-format on
  }
}

void HTreeBuilder::legalizeDummy()
{
  Point<double> topLevelBufferLoc = sinkRegion_.getCenter();
  for (int levelIdx = 0; levelIdx < topologyForEachLevel_.size(); ++levelIdx) {
    LevelTopology& topology = topologyForEachLevel_[levelIdx];

    for (unsigned idx = 0; idx < topology.getBranchingPointSize(); ++idx) {
      Point<double>& branchPoint = topology.getBranchingPoint(idx);
      unsigned parentIdx = topology.getBranchingPointParentIdx(idx);

      // clang-format off
      Point<double> parentPoint
          = (levelIdx == 0)
                ? topLevelBufferLoc
                : topologyForEachLevel_[levelIdx - 1].getBranchingPoint(
                      parentIdx);
      // clang-format on

      const std::vector<Point<double>>& sinks
          = topology.getBranchSinksLocations(idx);

      double leng = topology.getLength();
      Point<double>& sibLoc = findSiblingLoc(topology, idx, parentIdx);

      double d1 = computeDist(branchPoint, sibLoc);
      double d2 = computeDist(branchPoint, parentPoint);
      bool overlap = d1 == 0 || d2 == 0;
      bool dummy = sinks.empty();  // dummy buffers drive no sinks

      // not important, can be removed later?
      if (dummy) {
        setSiblingPosition(sibLoc, branchPoint, parentPoint);
        scalePosition(branchPoint, parentPoint, leng, 0.1);
      } else if (overlap) {
        scalePosition(branchPoint, parentPoint, leng, 0.1);
      } else {
        continue;
      }

      double x1, y1, x2, y2;
      int scalingFactor = wireSegmentUnit_;
      if (!isOccupiedLoc(branchPoint)
          && findBlockage(branchPoint, scalingFactor, x1, y1, x2, y2)) {
        Point<double> legalBranchPoint(branchPoint);
        std::vector<Point<double>> legalLocations;
        findLegalLocations(
            parentPoint, branchPoint, x1, y1, x2, y2, legalLocations);
        legalBranchPoint = findBestLegalLocation(topology.getLength(),
                                                 branchPoint,
                                                 parentPoint,
                                                 legalLocations,
                                                 sinks,
                                                 x1,
                                                 y2,
                                                 x2,
                                                 y2,
                                                 scalingFactor,
                                                 odb::Direction2D::North);
        double d = computeDist(legalBranchPoint, parentPoint);
        // clang-format off
        debugPrint(logger_, CTS, "legalizer", 1,
            "legalizeDummy level index {}: {}->{} d={:0.3f}, leng={:0.3f},"
		   "ratio={:0.3f}", levelIdx, branchPoint, legalBranchPoint,
		   d, leng, d / leng);
        // clang-format on
        commitMoveLoc(branchPoint, legalBranchPoint);
        branchPoint.setX(legalBranchPoint.getX());
        branchPoint.setY(legalBranchPoint.getY());
      }
    }
  }
}

void HTreeBuilder::legalize()
{
  if (logger_->debugCheck(utl::CTS, "legalizer", 3)) {
    logger_->report("HTree before legalization -------");
    printHTree();
  }
  // make sure top level buffer is legal
  Point<double> oldTopBufferLoc = sinkRegion_.getCenter();
  Point<double> newTopBufferLoc
      = legalizeOneBuffer(oldTopBufferLoc, options_->getRootBuffer());
  sinkRegion_.setCenter(newTopBufferLoc);
  commitMoveLoc(oldTopBufferLoc, newTopBufferLoc);
  // clang-format off
  debugPrint(logger_, CTS, "legalizer", 3, "legalize: top buf loc:{}->{}",
	     oldTopBufferLoc, newTopBufferLoc);
  // clang-format on
  for (int levelIdx = 0; levelIdx < topologyForEachLevel_.size(); ++levelIdx) {
    LevelTopology& topology = topologyForEachLevel_[levelIdx];

    for (unsigned bufferIdx = 0; bufferIdx < topology.getBranchingPointSize();
         ++bufferIdx) {
      // bufferIdx is the buffer id at level levelIdx
      Point<double>& branchPoint = topology.getBranchingPoint(bufferIdx);
      unsigned parentIdx = topology.getBranchingPointParentIdx(bufferIdx);

      // clang-format off
      Point<double> parentPoint
          = (levelIdx == 0)
                ? newTopBufferLoc
                : topologyForEachLevel_[levelIdx - 1].getBranchingPoint(
                      parentIdx);
      // clang-format on

      odb::Direction2D::Value branch_point_dir;
      if (isHorizontal(levelIdx + 1)) {
        if (branchPoint.getX() - parentPoint.getX() > 0) {
          branch_point_dir = odb::Direction2D::East;
        } else {
          branch_point_dir = odb::Direction2D::West;
        }
      } else {
        if (branchPoint.getY() - parentPoint.getY() > 0) {
          branch_point_dir = odb::Direction2D::North;
        } else {
          branch_point_dir = odb::Direction2D::South;
        }
      }

      const std::vector<Point<double>>& sinks
          = topology.getBranchSinksLocations(bufferIdx);

      double leng = computeDist(branchPoint, parentPoint);
      // clang-format off
      if (logger_->debugCheck(utl::CTS, "legalizer", 3)) {
        logger_->report("  HTree level*{}* bufId*{}*, parent:{}, branch:{}, "
			"leng:{:0.3f}, sinks:{}", levelIdx, bufferIdx,
			parentPoint, branchPoint, leng, sinks.size());
      }
      // clang-format on
      int scalingFactor = wireSegmentUnit_;
      double x1, y1, x2, y2;
      if (!isOccupiedLoc(branchPoint)
          && findBlockage(branchPoint, scalingFactor, x1, y1, x2, y2)) {
        Point<double> legalBranchPoint(branchPoint);
        std::vector<Point<double>> legalLocations;
        // find all the possible locations off the blockage
        findLegalLocations(
            parentPoint, branchPoint, x1, y1, x2, y2, legalLocations);
        // choose the best new location based on desired topology length
        legalBranchPoint = findBestLegalLocation(topology.getLength(),
                                                 branchPoint,
                                                 parentPoint,
                                                 legalLocations,
                                                 sinks,
                                                 x1,
                                                 y1,
                                                 x2,
                                                 y2,
                                                 scalingFactor,
                                                 branch_point_dir);
        // clang-format off
	debugPrint(logger_, CTS, "legalizer", 1,
		   "findBestLegalLocation branchPt:{}=>{} parentPt:{} new "
		   "branchPt is {} blockage", branchPoint, legalBranchPoint,
		   parentPoint,
		   isInsideBbox(legalBranchPoint.getX(), legalBranchPoint.getY(),
				x1, y1, x2, y2)? "inside" : "outside");
        // clang-format on
        // update branchPoint
        commitMoveLoc(branchPoint, legalBranchPoint);
        branchPoint = legalBranchPoint;
      } else if (isOccupiedLoc(branchPoint)
                 || !fuzzyEqual(leng, topology.getLength(), 0.01)) {
        // legal branch point needs adjustment if parent point moved in previous
        // level
        Point<double> newLocation(branchPoint);
        newLocation = adjustBeyondBlockage(branchPoint,
                                           parentPoint,
                                           topology.getLength(),
                                           sinks,
                                           scalingFactor,
                                           branch_point_dir);
        // clang-format off
	debugPrint(logger_, CTS, "legalizer", 3,
		   "adjustBeyondBlockage applied to legal branchPt:"
		   "{}=>{} parentPt:{} newDist={:0.3f}", branchPoint, newLocation,
		   parentPoint, computeDist(newLocation, parentPoint));
        // clang-format on
        commitMoveLoc(branchPoint, newLocation);
        branchPoint = newLocation;
      } else {
        commitLoc(branchPoint);
      }
    }
  }

  // "further" optimize the location of the "dummy" buffers that drive
  // no sinks (still needed?)
  legalizeDummy();

  if (logger_->debugCheck(utl::CTS, "legalizer", 3)) {
    logger_->report("HTree after legalization -------");
    printHTree();
  }
}

void HTreeBuilder::run()
{
  double clusterDiameter = (type_ == TreeType::MacroTree)
                               ? options_->getMacroMaxDiameter()
                               : options_->getMaxDiameter();
  unsigned clusterSize = (type_ == TreeType::MacroTree)
                             ? options_->getMacroSinkClusteringSize()
                             : options_->getSinkClusteringSize();
  bool useMaxCap = (type_ == TreeType::MacroTree)
                       ? false
                       : options_->getSinkClusteringUseMaxCap();

  logger_->info(
      CTS, 27, "Generating H-Tree topology for net {}.", clock_.getName());
  logger_->info(CTS, 28, " Total number of sinks: {}.", clock_.getNumSinks());
  if (options_->getSinkClustering()) {
    if (useMaxCap) {
      logger_->info(
          CTS, 90, " Sinks will be clustered based on buffer max cap.");
    } else {
      logger_->info(
          CTS,
          29,
          " {} sinks will be clustered in groups of up to {} and with "
          "maximum cluster diameter of {:.1f} um.",
          type_ == TreeType::MacroTree ? "Macro " : "Register",
          clusterSize,
          clusterDiameter);
    }
  }
  logger_->info(
      CTS, 30, " Number of static layers: {}.", options_->getNumStaticLayers());

  clockTreeMaxDepth_ = options_->getClockTreeMaxDepth();
  minInputCap_ = techChar_->getActualMinInputCap();
  numMaxLeafSinks_ = options_->getNumMaxLeafSinks();
  minLengthSinkRegion_ = techChar_->getMinSegmentLength() * 2;

  initSinkRegion();

  for (int level = 1; level <= clockTreeMaxDepth_; ++level) {
    const unsigned numSinksPerSubRegion
        = computeNumberOfSinksPerSubRegion(level);
    double regionWidth, regionHeight;
    computeSubRegionSize(level, regionWidth, regionHeight);

    if (isSubRegionTooSmall(regionWidth, regionHeight)) {
      if (options_->isFakeLutEntriesEnabled()) {
        const unsigned minIndex = 1;
        techChar_->createFakeEntries(minLengthSinkRegion_, minIndex);
        minLengthSinkRegion_ = 1;
      } else {
        logger_->info(
            CTS,
            31,
            " Stop criterion found. Min length of sink region is ({}).",
            minLengthSinkRegion_);
        break;
      }
    }

    computeLevelTopology(level, regionWidth, regionHeight);

    if (isNumberOfSinksTooSmall(numSinksPerSubRegion)) {
      logger_->info(CTS,
                    32,
                    " Stop criterion found. Max number of sinks is {}.",
                    options_->getMaxFanout() ? options_->getMaxFanout()
                                             : numMaxLeafSinks_);
      break;
    }
  }

  if (topologyForEachLevel_.empty()) {
    createSingleBufferClockNet();
    treeBufLevels_++;
    return;
  }

  clock_.setMaxLevel(topologyForEachLevel_.size());

  if (options_->getPlotSolution()
      || logger_->debugCheck(utl::CTS, "HTree", 2)) {
    plotSolution();
  }

  if (CtsObserver* observer = options_->getObserver()) {
    observer->initializeWithClock(this, clock_);
  }

  if (options_->getObstructionAware()) {
    legalize();
  }
  createClockSubNets();
  // clang-format off
  debugPrint(logger_, CTS, "legalizer", 3, "Htree file {} has been generated",
             plotHTree());
  debugPrint(logger_, CTS, "legalizer", 3, "Run 'obsAwareCts.py cts.clk.buffer'"
	     "to produce cts.clk.buffer.png");
  // clang-format on
}

bool HTreeBuilder::isNumberOfSinksTooSmall(unsigned numSinksPerSubRegion) const
{
  if (options_->getMaxFanout()) {
    return numSinksPerSubRegion < options_->getMaxFanout();
  }
  return numSinksPerSubRegion < numMaxLeafSinks_;
}

std::string HTreeBuilder::plotHTree()
{
  auto name = std::string("cts.") + clock_.getName() + ".buffer";
  std::ofstream file(name);

  plotBlockage(file, db_, wireSegmentUnit_);

  Point<double> topLevelBufferLoc = sinkRegion_.getCenter();

  for (int levelIdx = 0; levelIdx < topologyForEachLevel_.size(); ++levelIdx) {
    LevelTopology& topology = topologyForEachLevel_[levelIdx];

    // clang-format off
    topology.forEachBranchingPoint(
        [&](unsigned idx, Point<double> branchPoint) {
          unsigned parentIdx = topology.getBranchingPointParentIdx(idx);

          Point<double> parentPoint
              = (levelIdx == 0)
                    ? topLevelBufferLoc
                    : topologyForEachLevel_[levelIdx - 1].getBranchingPoint(
                          parentIdx);

          const std::vector<Point<double>>& sinks
              = topology.getBranchSinksLocations(idx);

          plotSinks(file, sinks);

          double x1 = parentPoint.getX();
          double y1 = parentPoint.getY();
          double x2 = branchPoint.getX();
          double y2 = branchPoint.getY();
          std::string name = "buffer";
          file << levelIdx << " " << x1 << " " << y1 << " " << x2 << " " << y2;
          file << " " << name << '\n';
        });
    // clang-format on
  }

  LevelTopology& leafTopology = topologyForEachLevel_.back();
  unsigned numSinks = 0;
  leafTopology.forEachBranchingPoint(
      [&](unsigned idx, Point<double> branchPoint) {
        double px = branchPoint.getX();
        double py = branchPoint.getY();

        const std::vector<Point<double>>& sinkLocs
            = leafTopology.getBranchSinksLocations(idx);

        for (const Point<double>& loc : sinkLocs) {
          auto name2 = mapLocationToSink_[loc]->getName();

          file << numSinks << " " << loc.getX() << " " << loc.getY();
          file << " " << px << " " << py << " leafbuffer " << name2;
          file << " z=" << wireSegmentUnit_ << '\n';
          ++numSinks;
        }
      });
  file.close();
  return name;
}

unsigned HTreeBuilder::computeNumberOfSinksPerSubRegion(
    const unsigned level) const
{
  unsigned min_clustering_sinks = (type_ == TreeType::MacroTree)
                                      ? min_clustering_macro_sinks_
                                      : min_clustering_sinks_;
  unsigned totalNumSinks = 0;
  if (clock_.getNumSinks() > min_clustering_sinks
      && options_->getSinkClustering()) {
    totalNumSinks = topLevelSinksClustered_.size();
  } else {
    totalNumSinks = clock_.getNumSinks();
  }
  const unsigned numRoots = std::pow(2, level);
  const double numSinksPerRoot = (double) totalNumSinks / numRoots;
  return std::ceil(numSinksPerRoot);
}

void HTreeBuilder::computeSubRegionSize(const unsigned level,
                                        double& width,
                                        double& height) const
{
  unsigned gridSizeX = 0;
  unsigned gridSizeY = 0;
  if (isVertical(1)) {
    gridSizeY = computeGridSizeX(level);
    gridSizeX = computeGridSizeY(level);
  } else {
    gridSizeX = computeGridSizeX(level);
    gridSizeY = computeGridSizeY(level);
  }
  width = sinkRegion_.getWidth() / gridSizeX;
  height = sinkRegion_.getHeight() / gridSizeY;
}

void HTreeBuilder::computeLevelTopology(const unsigned level,
                                        const double width,
                                        const double height)
{
  const unsigned numSinksPerSubRegion = computeNumberOfSinksPerSubRegion(level);
  logger_->report(" Level {}", level);
  logger_->report("    Direction: {}",
                  (isVertical(level)) ? ("Vertical") : ("Horizontal"));
  logger_->report("    Sinks per sub-region: {}", numSinksPerSubRegion);
  logger_->report("    Sub-region size: {:.4f} X {:.4f}", width, height);

  const unsigned minLength = minLengthSinkRegion_;
  const unsigned clampedMinLength = std::max(minLength, 1u);

  unsigned segmentLength
      = std::round(width / (double) clampedMinLength) * minLength / 2;

  if (isVertical(level)) {
    segmentLength
        = std::round(height / (double) clampedMinLength) * minLength / 2;
  }
  segmentLength = std::max<unsigned>(segmentLength, 1);

  LevelTopology topology(segmentLength);

  logger_->info(CTS, 34, "    Segment length (rounded): {}.", segmentLength);

  const int vertexBufferLength
      = options_->getVertexBufferDistance() / (techChar_->getLengthUnit() * 2);
  int remainingLength
      = options_->getBufferDistance() / (techChar_->getLengthUnit());
  int currWl = 0;
  unsigned inputCap = minInputCap_;
  unsigned inputSlew = 1;
  if (level > 1) {
    const LevelTopology& previousLevel = topologyForEachLevel_[level - 2];
    inputCap = previousLevel.getOutputCap();
    inputSlew = previousLevel.getOutputSlew();
    remainingLength = previousLevel.getRemainingLength();
    currWl = previousLevel.getCurrWl();
  }

  const unsigned kSlewThreshold = options_->getMaxSlew();
  const unsigned kInitTolerance = 1;

  int wirelengthThreshold;
  // If max wirelength is 0, set it as slew threshold  * maximum topology
  // wirelength. This will behave as if there was no max wirelength threshold.
  if (!options_->getMaxWl()) {
    wirelengthThreshold = kSlewThreshold * techChar_->getMaxSegmentLength();
  } else {
    wirelengthThreshold = options_->getMaxWl() / options_->getWireSegmentUnit();
  }

  debugPrint(
      logger_, CTS, "tech char", 1, "slew threshold = {}", kSlewThreshold);
  debugPrint(logger_,
             CTS,
             "tech char",
             1,
             "wirelength threshold = {}",
             wirelengthThreshold);
  unsigned length = 0;
  for (int charSegLength = techChar_->getMaxSegmentLength(); charSegLength >= 1;
       --charSegLength) {
    const unsigned numWires = (segmentLength - length) / charSegLength;

    if (numWires >= 1) {
      for (int wireCount = 0; wireCount < numWires; ++wireCount) {
        debugPrint(logger_, CTS, "tech char", 1, "curr wl = {}", currWl);
        unsigned outCap = 0, outSlew = 0;
        unsigned key = 0;
        if (options_->isSimpleSegmentEnabled()) {
          remainingLength -= charSegLength;

          if (segmentLength >= vertexBufferLength && (wireCount + 1 >= numWires)
              && options_->isVertexBuffersEnabled()) {
            remainingLength = 0;
            key = computeMinDelaySegment(charSegLength,
                                         inputSlew,
                                         inputCap,
                                         kSlewThreshold,
                                         kInitTolerance,
                                         outSlew,
                                         outCap,
                                         true,
                                         remainingLength);
            remainingLength
                += options_->getBufferDistance() / (techChar_->getLengthUnit());
          } else {
            if (remainingLength <= 0) {
              key = computeMinDelaySegment(charSegLength,
                                           inputSlew,
                                           inputCap,
                                           kSlewThreshold,
                                           kInitTolerance,
                                           outSlew,
                                           outCap,
                                           true,
                                           remainingLength);
              remainingLength += options_->getBufferDistance()
                                 / (techChar_->getLengthUnit());
            } else {
              key = computeMinDelaySegment(charSegLength,
                                           inputSlew,
                                           inputCap,
                                           kSlewThreshold,
                                           kInitTolerance,
                                           outSlew,
                                           outCap,
                                           false,
                                           remainingLength);
            }
          }
        } else {
          key = computeMinDelaySegment(charSegLength,
                                       inputSlew,
                                       inputCap,
                                       kSlewThreshold,
                                       wirelengthThreshold,
                                       kInitTolerance,
                                       outSlew,
                                       outCap,
                                       currWl);
        }

        if (key == std::numeric_limits<unsigned>::max()) {
          // No tech char entry found.
          continue;
        }

        length += charSegLength;
        techChar_->reportSegment(key);

        inputCap = std::max(outCap, minInputCap_);
        inputSlew = outSlew;
        topology.addWireSegment(key);
        topology.setRemainingLength(remainingLength);
      }

      if (length == segmentLength) {
        break;
      }
    }
  }

  topology.setOutputSlew(inputSlew);
  topology.setOutputCap(inputCap);
  topology.setCurrWl(currWl);

  computeBranchingPoints(level, topology);
  topologyForEachLevel_.push_back(topology);
}

unsigned HTreeBuilder::computeMinDelaySegment(const unsigned length) const
{
  unsigned minKey = std::numeric_limits<unsigned>::max();
  unsigned minDelay = std::numeric_limits<unsigned>::max();

  techChar_->forEachWireSegment(
      length, 1, 1, [&](unsigned key, const WireSegment& seg) {
        if (!seg.isBuffered()) {
          return;
        }
        if (seg.getDelay() < minDelay) {
          minKey = key;
          minDelay = seg.getDelay();
        }
      });

  return minKey;
}

unsigned HTreeBuilder::computeMinDelaySegment(const unsigned length,
                                              const unsigned inputSlew,
                                              const unsigned inputCap,
                                              const unsigned slewThreshold,
                                              const int wirelengthThreshold,
                                              const unsigned tolerance,
                                              unsigned& outputSlew,
                                              unsigned& outputCap,
                                              int& currWl) const
{
  unsigned minKey = std::numeric_limits<unsigned>::max();
  unsigned minDelay = std::numeric_limits<unsigned>::max();
  unsigned minBufKey = std::numeric_limits<unsigned>::max();
  unsigned minBufDelay = std::numeric_limits<unsigned>::max();

  for (int load = 1; load <= techChar_->getMaxCapacitance(); ++load) {
    for (int outSlew = 1; outSlew <= techChar_->getMaxSlew(); ++outSlew) {
      techChar_->forEachWireSegment(
          length, load, outSlew, [&](unsigned key, const WireSegment& seg) {
            if (std::abs((int) seg.getInputCap() - (int) inputCap) > tolerance
                || std::abs((int) seg.getInputSlew() - (int) inputSlew)
                       > tolerance) {
              return;
            }

            if (seg.isBuffered()
                && (currWl + seg.getWl2FirstBuffer() > wirelengthThreshold)) {
              return;
            }

            if (!seg.isBuffered()
                && (currWl + seg.getWl2FirstBuffer()
                    > wirelengthThreshold - techChar_->getMinSegmentLength())) {
              return;
            }

            if (seg.getDelay() < minDelay) {
              minDelay = seg.getDelay();
              minKey = key;
            }

            if (seg.isBuffered() && seg.getDelay() < minBufDelay) {
              minBufDelay = seg.getDelay();
              minBufKey = key;
            }
          });
    }
  }

  const unsigned MAX_TOLERANCE = 10;
  if (inputSlew >= slewThreshold) {
    if (minBufKey < std::numeric_limits<unsigned>::max()) {
      const WireSegment& bestBufSegment = techChar_->getWireSegment(minBufKey);
      outputSlew = bestBufSegment.getOutputSlew();
      outputCap = bestBufSegment.getLoad();
      currWl = bestBufSegment.getLastWl();
      return minBufKey;
    }
    if (tolerance < MAX_TOLERANCE) {
      // Increasing tolerance
      return computeMinDelaySegment(length,
                                    inputSlew,
                                    inputCap,
                                    slewThreshold,
                                    wirelengthThreshold,
                                    tolerance + 1,
                                    outputSlew,
                                    outputCap,
                                    currWl);
    }
  }

  if (minKey == std::numeric_limits<unsigned>::max()) {
    if (tolerance >= MAX_TOLERANCE) {
      return minKey;
    }
    // Increasing tolerance
    return computeMinDelaySegment(length,
                                  inputSlew,
                                  inputCap,
                                  slewThreshold,
                                  wirelengthThreshold,
                                  tolerance + 1,
                                  outputSlew,
                                  outputCap,
                                  currWl);
  }

  const WireSegment& bestSegment = techChar_->getWireSegment(minKey);
  if (bestSegment.isBuffered()) {
    outputSlew = bestSegment.getOutputSlew();
    currWl = bestSegment.getLastWl();
  } else {
    outputSlew
        = std::max((unsigned) bestSegment.getOutputSlew(), inputSlew + 1);
    currWl += bestSegment.getLastWl();
  }
  outputCap = bestSegment.getLoad();

  return minKey;
}

unsigned HTreeBuilder::computeMinDelaySegment(const unsigned length,
                                              const unsigned inputSlew,
                                              const unsigned inputCap,
                                              const unsigned slewThreshold,
                                              const unsigned tolerance,
                                              unsigned& outputSlew,
                                              unsigned& outputCap,
                                              const bool forceBuffer,
                                              const int expectedLength) const
{
  unsigned minKey = std::numeric_limits<unsigned>::max();
  unsigned minDelay = std::numeric_limits<unsigned>::max();
  unsigned minBufKey = std::numeric_limits<unsigned>::max();
  unsigned minBufDelay = std::numeric_limits<unsigned>::max();
  unsigned minBufKeyFallback = std::numeric_limits<unsigned>::max();
  unsigned minDelayFallback = std::numeric_limits<unsigned>::max();

  for (int load = 1; load <= techChar_->getMaxCapacitance(); ++load) {
    for (int outSlew = 1; outSlew <= techChar_->getMaxSlew(); ++outSlew) {
      techChar_->forEachWireSegment(
          length, load, outSlew, [&](unsigned key, const WireSegment& seg) {
            // Same as the other functions, however, forces a segment
            // to have a buffer in a specific location.
            const unsigned normalLength = length;
            if (!seg.isBuffered() && seg.getDelay() < minDelay) {
              minDelay = seg.getDelay();
              minKey = key;
            }
            if (seg.isBuffered() && seg.getDelay() < minBufDelay
                && seg.getNumBuffers() == 1) {
              // If buffer is in the range of 10% of the expected location, save
              // its key.
              if (seg.getBufferLocation(0)
                      > ((((double) normalLength + (double) expectedLength)
                          / (double) normalLength)
                         * 0.9)
                  && seg.getBufferLocation(0)
                         < ((((double) normalLength + (double) expectedLength)
                             / (double) normalLength)
                            * 1.1)) {
                minBufDelay = seg.getDelay();
                minBufKey = key;
              }
              if (seg.getDelay() < minDelayFallback) {
                minDelayFallback = seg.getDelay();
                minBufKeyFallback = key;
              }
            }
          });
    }
  }

  if (forceBuffer) {
    if (minBufKey != std::numeric_limits<unsigned>::max()) {
      return minBufKey;
    }

    if (minBufKeyFallback != std::numeric_limits<unsigned>::max()) {
      return minBufKeyFallback;
    }
  }

  return minKey;
}

void HTreeBuilder::computeBranchingPoints(const unsigned level,
                                          LevelTopology& topology)
{
  if (level == 1) {
    const Point<double> clockRoot(sinkRegion_.getCenter());
    Point<double> low(clockRoot);
    Point<double> high(clockRoot);
    if (isHorizontal(level)) {
      low.setX(low.getX() - topology.getLength());
      high.setX(high.getX() + topology.getLength());
    } else {
      low.setY(low.getY() - topology.getLength());
      high.setY(high.getY() + topology.getLength());
    }
    const unsigned branchPtIdx1
        = topology.addBranchingPoint(low, LevelTopology::NO_PARENT);
    const unsigned branchPtIdx2
        = topology.addBranchingPoint(high, LevelTopology::NO_PARENT);

    refineBranchingPointsWithClustering(topology,
                                        level,
                                        branchPtIdx1,
                                        branchPtIdx2,
                                        clockRoot,
                                        topLevelSinksClustered_);
    return;
  }

  LevelTopology& parentTopology = topologyForEachLevel_[level - 2];
  parentTopology.forEachBranchingPoint(
      [&](unsigned idx, Point<double> clockRoot) {
        Point<double> low(clockRoot);
        Point<double> high(clockRoot);
        if (isHorizontal(level)) {
          low.setX(low.getX() - topology.getLength());
          high.setX(high.getX() + topology.getLength());
        } else {
          low.setY(low.getY() - topology.getLength());
          high.setY(high.getY() + topology.getLength());
        }
        const unsigned branchPtIdx1 = topology.addBranchingPoint(low, idx);
        const unsigned branchPtIdx2 = topology.addBranchingPoint(high, idx);

        std::vector<std::pair<float, float>> sinks;
        computeBranchSinks(parentTopology, idx, sinks);
        refineBranchingPointsWithClustering(
            topology, level, branchPtIdx1, branchPtIdx2, clockRoot, sinks);
      });
}

void HTreeBuilder::initTopLevelSinks(
    std::vector<std::pair<float, float>>& sinkLocations,
    std::vector<const ClockInst*>& sinkInsts)
{
  sinkLocations.clear();
  clock_.forEachSink([&](const ClockInst& sink) {
    sinkLocations.emplace_back((float) sink.getX() / wireSegmentUnit_,
                               (float) sink.getY() / wireSegmentUnit_);
    sinkInsts.emplace_back(&sink);
  });
}

void HTreeBuilder::initSecondLevelSinks(
    std::vector<std::pair<float, float>>& sinkLocations,
    std::vector<const ClockInst*>& sinkInsts)
{
  sinkLocations.clear();
  for (const auto& buf : topLevelSinksClustered_) {
    sinkLocations.emplace_back(buf.first, buf.second);
    const Point<double> bufPos(buf.first, buf.second);
    sinkInsts.emplace_back(mapLocationToSink_[bufPos]);
  }
}

void HTreeBuilder::computeBranchSinks(
    const LevelTopology& topology,
    const unsigned branchIdx,
    std::vector<std::pair<float, float>>& sinkLocations) const
{
  sinkLocations.clear();
  for (const Point<double>& point :
       topology.getBranchSinksLocations(branchIdx)) {
    sinkLocations.emplace_back(point.getX(), point.getY());
  }
}

void HTreeBuilder::refineBranchingPointsWithClustering(
    LevelTopology& topology,
    const unsigned level,
    const unsigned branchPtIdx1,
    const unsigned branchPtIdx2,
    const Point<double>& rootLocation,
    const std::vector<std::pair<float, float>>& sinks)
{
  CKMeans::Clustering clusteringEngine(
      sinks, rootLocation.getX(), rootLocation.getY(), logger_);

  Point<double>& branchPt1 = topology.getBranchingPoint(branchPtIdx1);
  Point<double>& branchPt2 = topology.getBranchingPoint(branchPtIdx2);

  std::vector<std::pair<float, float>> means;
  means.emplace_back(branchPt1.getX(), branchPt1.getY());
  means.emplace_back(branchPt2.getX(), branchPt2.getY());

  const unsigned cap
      = options_->getMaxFanout()
            ? (unsigned) (sinks.size() * 0.5)
            : (unsigned) (sinks.size() * options_->getClusteringCapacity());
  clusteringEngine.iterKmeans(
      1, means.size(), cap, 5, options_->getClusteringPower(), means);

  if (((int) options_->getNumStaticLayers() - (int) level) < 0) {
    branchPt1 = Point<double>(means[0].first, means[0].second);
    branchPt2 = Point<double>(means[1].first, means[1].second);
  }

  std::vector<std::vector<unsigned>> clusters;
  clusteringEngine.getClusters(clusters);
  unsigned movedSinks = 0;
  const double errorFactor = 1.2;
  for (int clusterIdx = 0; clusterIdx < clusters.size(); ++clusterIdx) {
    for (int elementIdx = 0; elementIdx < clusters[clusterIdx].size();
         ++elementIdx) {
      const unsigned sinkIdx = clusters[clusterIdx][elementIdx];
      const Point<double> sinkLoc(sinks[sinkIdx].first, sinks[sinkIdx].second);
      const double dist = clusterIdx == 0 ? computeDist(branchPt1, sinkLoc)
                                          : computeDist(branchPt2, sinkLoc);
      const double distOther = clusterIdx == 0
                                   ? computeDist(branchPt2, sinkLoc)
                                   : computeDist(branchPt1, sinkLoc);
      if (clusterIdx == 0) {
        topology.addSinkToBranch(branchPtIdx1, sinkLoc);
      } else {
        topology.addSinkToBranch(branchPtIdx2, sinkLoc);
      }

      if (dist >= distOther * errorFactor) {
        movedSinks++;
      }
    }
  }

  if (movedSinks > 0) {
    debugPrint(logger_,
               CTS,
               "clustering",
               1,
               " Out of {} sinks, {} sinks closer to other cluster.",
               sinks.size(),
               movedSinks);
  }
}

void HTreeBuilder::createClockSubNets()
{
  Point<double> center = sinkRegion_.getCenter();
  Point<double> legalCenter
      = legalizeOneBuffer(center, options_->getRootBuffer());
  sinkRegion_.setCenter(legalCenter);
  commitMoveLoc(center, legalCenter);
  const int centerX = legalCenter.getX() * wireSegmentUnit_;
  const int centerY = legalCenter.getY() * wireSegmentUnit_;

  ClockInst& rootBuffer = clock_.addClockBuffer(
      "clkbuf_0", options_->getRootBuffer(), centerX, centerY);

  if (topBufferName_.empty()) {
    topBufferName_ = rootBuffer.getName();
  }

  // clang-format off
  if (center != legalCenter) {
    debugPrint(logger_, CTS, "legalizer", 2, "createClockSubNets: "
	       "root clkbuf_0: {} => {}", center, legalCenter);
  } else {
    debugPrint(logger_, CTS, "legalizer", 2, "createClockSubNets: "
	       "root clkbuf_0: {}", center);
  }
  // clang-format on

  addTreeLevelBuffer(&rootBuffer);
  ClockSubNet& rootClockSubNet = clock_.addSubNet("clknet_0");
  rootClockSubNet.setTreeLevel(0);
  rootClockSubNet.addInst(rootBuffer);
  treeBufLevels_++;

  // First level...
  LevelTopology& topLevelTopology = topologyForEachLevel_[0];
  bool isFirstPoint = true;
  topLevelTopology.forEachBranchingPoint([&](unsigned idx,
                                             Point<double> branchPoint) {
    // If the branch point has no sinks that will be connected to
    // it don't create a clock sub net for it
    if (topLevelTopology.getBranchSinksLocations(idx).empty()) {
      return;
    }
    Point<double> legalBranchPoint
        = legalizeOneBuffer(branchPoint, options_->getRootBuffer());
    commitMoveLoc(branchPoint, legalBranchPoint);

    // clang-format off
    if (branchPoint != legalBranchPoint) {
      debugPrint(logger_, CTS, "legalizer", 2, 
		 "createClockSubNets level 1 clk_buf_1_{}_ : {} => {}",
		 std::to_string(idx), branchPoint, legalBranchPoint);
    } else {
      debugPrint(logger_, CTS, "legalizer", 2, 
		 "createClockSubNets level 1 clk_buf_1_{}_ : {}",
		 std::to_string(idx), branchPoint);
    }
    // clang-format on

    SegmentBuilder builder("clkbuf_1_" + std::to_string(idx) + "_",
                           "clknet_1_" + std::to_string(idx) + "_",
                           legalCenter,  // center may have moved, don't use
                                         // sinkRegion_.getCenter()
                           legalBranchPoint,
                           topLevelTopology.getWireSegments(),
                           clock_,
                           rootClockSubNet,
                           *techChar_,
                           wireSegmentUnit_,
                           this);
    if (!options_->getTreeBuffer().empty()) {
      builder.build(options_->getTreeBuffer());
    } else {
      builder.build();
    }
    if (topologyForEachLevel_.size() == 1) {
      builder.forceBufferInSegment(options_->getRootBuffer());
    }
    if (isFirstPoint) {
      treeBufLevels_ += builder.getNumBufferLevels();
      isFirstPoint = false;
    }
    topLevelTopology.setBranchDrivingSubNet(idx, *builder.getDrivingSubNet());
  });

  // Others...
  for (int levelIdx = 1; levelIdx < topologyForEachLevel_.size(); ++levelIdx) {
    LevelTopology& topology = topologyForEachLevel_[levelIdx];
    isFirstPoint = true;
    topology.forEachBranchingPoint([&](unsigned idx,
                                       Point<double> branchPoint) {
      // If the branch point has no sinks that will be connected
      // to it don't create a clock sub net for it
      if (topology.getBranchSinksLocations(idx).empty()) {
        return;
      }
      unsigned parentIdx = topology.getBranchingPointParentIdx(idx);
      LevelTopology& parentTopology = topologyForEachLevel_[levelIdx - 1];
      Point<double> parentPoint = parentTopology.getBranchingPoint(parentIdx);

      Point<double> legalBranchPoint
          = legalizeOneBuffer(branchPoint, options_->getRootBuffer());
      commitMoveLoc(branchPoint, legalBranchPoint);

      // clang-format off
      if (branchPoint != legalBranchPoint) {
	debugPrint(logger_, CTS, "legalizer", 2, "createClockSubNets level {} "
		   "{} : {} => {}", levelIdx+1, "clkbuf_" +
		   std::to_string(levelIdx+1) + "_" + std::to_string(idx) + "_",
		   branchPoint, legalBranchPoint);
      } else {
	debugPrint(logger_, CTS, "legalizer", 2, "createClockSubNets level {} "
		   "{} : {}", levelIdx+1, "clkbuf_" +
		   std::to_string(levelIdx+1) + "_" + std::to_string(idx) + "_",
		   branchPoint);
      }
      // clang-format on

      SegmentBuilder builder("clkbuf_" + std::to_string(levelIdx + 1) + "_"
                                 + std::to_string(idx) + "_",
                             "clknet_" + std::to_string(levelIdx + 1) + "_"
                                 + std::to_string(idx) + "_",
                             parentPoint,
                             legalBranchPoint,
                             topology.getWireSegments(),
                             clock_,
                             *parentTopology.getBranchDrivingSubNet(parentIdx),
                             *techChar_,
                             wireSegmentUnit_,
                             this);

      // Set clock tree level the first time only.
      if (builder.getDrivingSubNet()->getTreeLevel() < 0) {
        builder.getDrivingSubNet()->setTreeLevel(levelIdx);
      }

      if (!options_->getTreeBuffer().empty()) {
        builder.build(options_->getTreeBuffer());
      } else {
        builder.build();
      }
      if (levelIdx == topologyForEachLevel_.size() - 1) {
        builder.forceBufferInSegment(options_->getRootBuffer());
      }
      if (isFirstPoint) {
        treeBufLevels_ += builder.getNumBufferLevels();
        isFirstPoint = false;
      }
      topology.setBranchDrivingSubNet(idx, *builder.getDrivingSubNet());
    });
  }

  LevelTopology& leafTopology = topologyForEachLevel_.back();
  unsigned numSinks = 0;
  leafTopology.forEachBranchingPoint(
      [&](unsigned idx, Point<double> branchPoint) {
        ClockSubNet* subNet = leafTopology.getBranchDrivingSubNet(idx);
        // If no clock sub net was created for a leaf branch point no sinks
        // connect to it, so just skip.
        if (subNet == nullptr) {
          return;
        }

        subNet->setLeafLevel(true);

        const std::vector<Point<double>>& sinkLocs
            = leafTopology.getBranchSinksLocations(idx);
        for (const Point<double>& loc : sinkLocs) {
          if (mapLocationToSink_.find(loc) == mapLocationToSink_.end()) {
            logger_->error(CTS, 80, "Sink not found.");
          }

          subNet->addInst(*mapLocationToSink_[loc]);
          ++numSinks;
        }
      });

  logger_->info(CTS, 35, " Number of sinks covered: {}.", numSinks);
}

void HTreeBuilder::createSingleBufferClockNet()
{
  logger_->report(" Building single-buffer clock net.");

  Point<double> center = sinkRegion_.getCenter();
  Point<double> legalCenter
      = legalizeOneBuffer(center, options_->getRootBuffer());
  sinkRegion_.setCenter(legalCenter);
  commitMoveLoc(center, legalCenter);
  const int centerX = legalCenter.getX() * wireSegmentUnit_;
  const int centerY = legalCenter.getY() * wireSegmentUnit_;
  ClockInst& rootBuffer = clock_.addClockBuffer(
      "clkbuf_0", options_->getRootBuffer(), centerX, centerY);

  // clang-format off
  if (center != legalCenter) {
    debugPrint(logger_, CTS, "legalizer", 2, "createSingleBufferClockNet "
	       "legalizeOneBuffer clkbuf_0: {} => {}", center, legalCenter);
  }
  // clang-format on

  addTreeLevelBuffer(&rootBuffer);
  ClockSubNet& clockSubNet = clock_.addSubNet("clknet_0");
  clockSubNet.setTreeLevel(0);
  clockSubNet.addInst(rootBuffer);

  clock_.forEachSink([&](ClockInst& inst) { clockSubNet.addInst(inst); });
}

void HTreeBuilder::plotSolution()
{
  auto name = std::string("plot_") + clock_.getName() + ".py";
  std::ofstream file(name);
  file << "import numpy as np\n";
  file << "import matplotlib.pyplot as plt\n";
  file << "import matplotlib.path as mpath\n";
  file << "import matplotlib.lines as mlines\n";
  file << "import matplotlib.patches as mpatches\n";
  file << "from matplotlib.collections import PatchCollection\n\n";

  clock_.forEachSink([&](const ClockInst& sink) {
    file << "plt.scatter(" << (double) sink.getX() / wireSegmentUnit_ << ", "
         << (double) sink.getY() / wireSegmentUnit_ << ", s=1)\n";
  });

  LevelTopology& topLevelTopology = topologyForEachLevel_.front();
  Point<double> topLevelBufferLoc = sinkRegion_.getCenter();
  topLevelTopology.forEachBranchingPoint(
      [&](unsigned idx, Point<double> branchPoint) {
        if (topLevelBufferLoc.getX() < branchPoint.getX()) {
          file << "plt.plot([" << topLevelBufferLoc.getX() << ", "
               << branchPoint.getX() << "], [" << topLevelBufferLoc.getY()
               << ", " << branchPoint.getY() << "], c = 'r')\n";
        } else {
          file << "plt.plot([" << branchPoint.getX() << ", "
               << topLevelBufferLoc.getX() << "], [" << branchPoint.getY()
               << ", " << topLevelBufferLoc.getY() << "], c = 'r')\n";
        }
      });

  for (int levelIdx = 1; levelIdx < topologyForEachLevel_.size(); ++levelIdx) {
    const LevelTopology& topology = topologyForEachLevel_[levelIdx];
    topology.forEachBranchingPoint([&](unsigned idx,
                                       Point<double> branchPoint) {
      unsigned parentIdx = topology.getBranchingPointParentIdx(idx);
      Point<double> parentPoint
          = topologyForEachLevel_[levelIdx - 1].getBranchingPoint(parentIdx);
      std::string color = "orange";
      if (levelIdx % 2 == 0) {
        color = "red";
      }

      if (parentPoint.getX() < branchPoint.getX()) {
        file << "plt.plot([" << parentPoint.getX() << ", " << branchPoint.getX()
             << "], [" << parentPoint.getY() << ", " << branchPoint.getY()
             << "], c = '" << color << "')\n";
      } else {
        file << "plt.plot([" << branchPoint.getX() << ", " << parentPoint.getX()
             << "], [" << branchPoint.getY() << ", " << parentPoint.getY()
             << "], c = '" << color << "')\n";
      }
    });
  }

  file << "plt.show()\n";
  file.close();
}

// print structures of Htree from top level buffer
// incluiding branch point locations, topology length and weighted sink lengths
void HTreeBuilder::printHTree()
{
  Point<double> topLevelBufferLoc = sinkRegion_.getCenter();
  logger_->report("HTree: top buf loc:{}", topLevelBufferLoc);
  for (int levelIdx = 0; levelIdx < topologyForEachLevel_.size(); ++levelIdx) {
    LevelTopology& topology = topologyForEachLevel_[levelIdx];

    for (unsigned idx = 0; idx < topology.getBranchingPointSize(); ++idx) {
      Point<double>& branchPoint = topology.getBranchingPoint(idx);
      unsigned parentIdx = topology.getBranchingPointParentIdx(idx);

      // clang-format off
      Point<double> parentPoint
          = (levelIdx == 0)
                ? topLevelBufferLoc
                : topologyForEachLevel_[levelIdx - 1].getBranchingPoint(
                      parentIdx);
      // clang-format on

      const std::vector<Point<double>>& sinks
          = topology.getBranchSinksLocations(idx);

      double leng = topology.getLength();
      // clang-format off
      logger_->report("HTree: level*{}* bufId*{}*: branchPt:{} topo len:{:0.3f}"
		      " dist to parent:{:0.3f} weighted sink len:{:0.3f} "
		      "parentPt:{}", levelIdx, idx, branchPoint, leng,
		      computeDist(branchPoint, parentPoint),
		      weightedDistance(branchPoint, branchPoint, sinks),
		      parentPoint);
      // clang-format on
    }
    logger_->report("-------------------------------------------------");
  }
}

SegmentBuilder::SegmentBuilder(const std::string& instPrefix,
                               const std::string& netPrefix,
                               const Point<double>& root,
                               const Point<double>& target,
                               const std::vector<unsigned>& techCharWires,
                               Clock& clock,
                               ClockSubNet& drivingSubNet,
                               const TechChar& techChar,
                               const unsigned techCharDistUnit,
                               TreeBuilder* tree)
    : instPrefix_(instPrefix),
      netPrefix_(netPrefix),
      root_(root),
      target_(target),
      techCharWires_(techCharWires),
      techChar_(&techChar),
      techCharDistUnit_(techCharDistUnit),
      clock_(&clock),
      drivingSubNet_(&drivingSubNet),
      tree_(tree)
{
}

void SegmentBuilder::build(const std::string& forceBuffer)
{
  const double lengthX = std::abs(root_.getX() - target_.getX());
  const bool isLowToHiX = root_.getX() < target_.getX();
  const bool isLowToHiY = root_.getY() < target_.getY();

  double connectionLength = 0.0;
  for (unsigned techCharWireIdx : techCharWires_) {
    const WireSegment& wireSegment = techChar_->getWireSegment(techCharWireIdx);
    const unsigned wireSegLen = wireSegment.getLength();
    for (int buffer = 0; buffer < wireSegment.getNumBuffers(); ++buffer) {
      const double location
          = wireSegment.getBufferLocation(buffer) * wireSegLen;
      connectionLength += location;

      double x = std::numeric_limits<double>::max();
      double y = std::numeric_limits<double>::max();
      if (connectionLength < lengthX) {
        y = root_.getY();
        x = (isLowToHiX) ? (root_.getX() + connectionLength)
                         : (root_.getX() - connectionLength);
      } else {
        x = target_.getX();
        y = (isLowToHiY) ? (root_.getY() + (connectionLength - lengthX))
                         : (root_.getY() - (connectionLength - lengthX));
      }

      const std::string buffMaster = !forceBuffer.empty()
                                         ? forceBuffer
                                         : wireSegment.getBufferMaster(buffer);
      Point<double> bufferLoc(x, y);
      Point<double> legalBufferLoc
          = tree_->legalizeOneBuffer(bufferLoc, buffMaster);
      tree_->commitMoveLoc(bufferLoc, legalBufferLoc);
      ClockInst& newBuffer = clock_->addClockBuffer(
          instPrefix_ + std::to_string(numBufferLevels_),
          buffMaster,
          legalBufferLoc.getX() * techCharDistUnit_,
          legalBufferLoc.getY() * techCharDistUnit_);

      // clang-format off
      if (bufferLoc != legalBufferLoc) {
	// adjust for cell movement
	connectionLength -= tree_->computeDist(bufferLoc, legalBufferLoc);
	debugPrint(getTree()->getLogger(), CTS, "legalizer", 2,
		   " SegmentBuilder::build {} TCId:{} bufId:{} connLen:{:0.1f}: "
		   "{} => {}", instPrefix_  + std::to_string(numBufferLevels_),
		   techCharWireIdx, buffer, connectionLength,
		   bufferLoc, legalBufferLoc);
      } else {
	debugPrint(getTree()->getLogger(), CTS, "legalizer", 2,
		   " SegmentBuilder::build {} TCId:{} bufId:{} connLen:{:0.1f}: "
		   "{}", instPrefix_  + std::to_string(numBufferLevels_),
		   techCharWireIdx, buffer, connectionLength, bufferLoc);
      }
      // clang-format on

      tree_->addTreeLevelBuffer(&newBuffer);

      drivingSubNet_->addInst(newBuffer);
      drivingSubNet_
          = &clock_->addSubNet(netPrefix_ + std::to_string(numBufferLevels_));
      drivingSubNet_->addInst(newBuffer);

      ++numBufferLevels_;
    }
    connectionLength += wireSegLen;
  }
}

void SegmentBuilder::forceBufferInSegment(const std::string& master)
{
  if (numBufferLevels_ != 0) {
    return;
  }

  ClockInst& newBuffer
      = clock_->addClockBuffer(instPrefix_ + "_f",
                               master,
                               target_.getX() * techCharDistUnit_,
                               target_.getY() * techCharDistUnit_);
  tree_->addTreeLevelBuffer(&newBuffer);
  // clang-format off
  debugPrint(getTree()->getLogger(), CTS, "legalizer", 2,
	     "  forceBufferInSegment {}: {}", instPrefix_ + "_f", target_);
  // clang-format on

  drivingSubNet_->addInst(newBuffer);
  drivingSubNet_ = &clock_->addSubNet(netPrefix_ + "_leaf");
  drivingSubNet_->addInst(newBuffer);
  numBufferLevels_++;
}

}  // namespace cts
