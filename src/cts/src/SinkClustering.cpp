// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "SinkClustering.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <fstream>
#include <iostream>
#include <limits>
#include <sstream>
#include <vector>

#include "CtsObserver.h"
#include "CtsOptions.h"
#include "HTreeBuilder.h"
#include "TechChar.h"
#include "TreeBuilder.h"
#include "Util.h"
#include "stt/SteinerTreeBuilder.h"
#include "utl/Logger.h"

namespace cts {

using std::vector;
using utl::CTS;

SinkClustering::SinkClustering(const CtsOptions* options,
                               TechChar* techChar,
                               HTreeBuilder* HTree)
    : options_(options),
      logger_(options->getLogger()),
      techChar_(techChar),
      maxInternalDiameter_(10),
      capPerUnit_(0.0),
      useMaxCapLimit_((HTree->getTreeType() == TreeType::MacroTree)
                          ? false
                          : options->getSinkClusteringUseMaxCap()),
      scaleFactor_(1),
      HTree_(HTree)
{
}

void SinkClustering::normalizePoints(float maxDiameter)
{
  if (firstRun_) {
    double xMax = -std::numeric_limits<double>::infinity();
    double xMin = std::numeric_limits<double>::infinity();
    double yMax = -std::numeric_limits<double>::infinity();
    double yMin = std::numeric_limits<double>::infinity();
    for (const Point<double>& p : points_) {
      xMax = std::max(p.getX(), xMax);
      yMax = std::max(p.getY(), yMax);
      xMin = std::min(p.getX(), xMin);
      yMin = std::min(p.getY(), yMin);
    }

    const double xSpan = xMax - xMin;
    const double ySpan = yMax - yMin;
    xSpan_ = xSpan;
    ySpan_ = ySpan;
    for (Point<double>& p : points_) {
      const double x = p.getX();
      const double xNorm = xSpan ? (x - xMin) / xSpan : 0;
      const double y = p.getY();
      const double yNorm = ySpan ? (y - yMin) / ySpan : 0;
      p = Point<double>(xNorm, yNorm);
    }
  }
  double span;
  if (xSpan_ == 0 && ySpan_ == 0) {
    span = 1;  // arbitrary
  } else if (xSpan_ == 0) {
    span = ySpan_;
  } else if (ySpan_ == 0) {
    span = xSpan_;
  } else {
    span = std::min(xSpan_, ySpan_);
  }
  maxInternalDiameter_ = maxDiameter / span;
  capPerUnit_ = techChar_->getCapPerDBU() * scaleFactor_ * span;

  // clang-format off
  debugPrint(logger_, CTS, "clustering", 1, "normalizePoints: "
             "maxInternalDiameter_:{} xSpan:{} ySpan:{}",
             maxInternalDiameter_, xSpan_, ySpan_);
  // clang-format on
}

void SinkClustering::computeAllThetas()
{
  if (firstRun_) {
    for (unsigned idx = 0; idx < points_.size(); ++idx) {
      const Point<double>& p = points_[idx];
      const double theta = computeTheta(p.getX(), p.getY());
      thetaIndexVector_.emplace_back(theta, idx);
    }
    // clang-format off
    debugPrint(logger_, CTS, "clustering", 1, "SinkClustering::computeAllThetas:"
               " thetaIndexVector_ has {} elems", thetaIndexVector_.size());
    // clang-format on
  }
}

void SinkClustering::sortPoints()
{
  if (firstRun_) {
    std::ranges::sort(thetaIndexVector_);
  }
}

/* static */
bool SinkClustering::isOne(const double pos)
{
  return (1 - pos) < std::numeric_limits<double>::epsilon();
}

/* static */
bool SinkClustering::isZero(const double pos)
{
  return pos < std::numeric_limits<double>::epsilon();
}

double SinkClustering::computeTheta(const double x, const double y) const
{
  if (isOne(x) && isOne(y)) {
    return 0.5;
  }

  const unsigned quad = numVertex(std::min(unsigned(2.0 * x), (unsigned) 1),
                                  std::min(unsigned(2.0 * y), (unsigned) 1));

  double t = computeTheta(2 * std::fabs(x - 0.5), 2 * std::fabs(y - 0.5));

  if (quad % 2 == 1) {
    t = 1 - t;
  }

  double integral;
  const double fractal = std::modf((quad + t) / 4.0 + 7.0 / 8.0, &integral);
  return fractal;
}

unsigned SinkClustering::numVertex(const unsigned x, const unsigned y) const
{
  if ((x == 0) && (y == 0)) {
    return 0;
  }
  if ((x == 0) && (y == 1)) {
    return 1;
  }
  if ((x == 1) && (y == 1)) {
    return 2;
  }
  if ((x == 1) && (y == 0)) {
    return 3;
  }

  logger_->error(CTS, 58, "Invalid parameters in {}.", __func__);

  // avoid warn message
  return 4;
}

void SinkClustering::run(const unsigned groupSize,
                         const float maxDiameter,
                         const int scaleFactor,
                         unsigned& bestSize,
                         float& bestDiameter)
{
  scaleFactor_ = scaleFactor;

  // clang-format off
  debugPrint(logger_, CTS, "clustering", 1, "SinkClustering::run points_ "
             "has {} elems", points_.size());
  // clang-format on
  if (firstRun_) {
    const auto& original_points = points_;
    if (CtsObserver* observer = options_->getObserver()) {
      observer->initializeWithPoints(this, original_points);
    }
  }
  normalizePoints(maxDiameter);
  computeAllThetas();
  sortPoints();
  bool bestSolutionFound = findBestMatching(groupSize);
  if (logger_->debugCheck(CTS, "Stree", 1)) {
    writePlotFile(groupSize);
  }

  firstRun_ = false;
  if (bestSolutionFound) {
    bestSize = groupSize;
    bestDiameter = maxDiameter;
  }
}

bool SinkClustering::findBestMatching(const unsigned groupSize)
{
  // Counts how many clusters are in each solution.
  vector<unsigned> clusters(groupSize, 0);
  // Keeps track of the total cost of each solution.
  vector<double> costs(groupSize, 0);
  vector<double> previousCosts(groupSize, 0);
  // Has the points for each cluster of each solution.
  vector<vector<vector<Point<double>>>> solutionPoints;
  // Has the points index for each cluster of each solution.
  // example: solutionsPointsIdx[solutionId][clusterIdx][pointIdx]
  vector<vector<vector<unsigned>>> solutionPointsIdx;
  // Has the sink indexes for each cluster of each solution.
  vector<vector<vector<unsigned>>> solutions;

  if (useMaxCapLimit_) {
    debugPrint(logger_,
               CTS,
               "Stree",
               1,
               "Clustering with max cap limit of {:.3e}",
               options_->getSinkBufferInputCap() * kMaxCapFactor);
  }
  // Iterates over the theta vector.
  for (unsigned i = 0; i < thetaIndexVector_.size(); ++i) {
    // The - groupSize is because each solution will start on a different index.
    // There is groupSize solutions.
    for (unsigned j = 0; j < groupSize; ++j) {
      if ((i + j) < thetaIndexVector_.size()) {
        // Add vectors in case they are no allocated yet.
        if (solutions.size() < (j + 1)) {
          solutions.emplace_back();
          solutionPoints.emplace_back();
          solutionPointsIdx.emplace_back();
        }
        if (solutions[j].size() < (clusters[j] + 1)) {
          solutions[j].emplace_back();
          solutionPoints[j].emplace_back();
          solutionPointsIdx[j].emplace_back();
        }
        // Get the current point
        const unsigned idx = thetaIndexVector_[i + j].second;
        const Point<double>& p = points_[idx];
        double distanceCost = 0;
        double capCost = pointsCap_[idx];
        unsigned pointIdx = 0;
        // Check the distance from the current point to others in the cluster,
        // if there are any.
        for (Point<double> comparisonPoint : solutionPoints[j][clusters[j]]) {
          const double cost = HTree_->computeDist(p, comparisonPoint);
          if (useMaxCapLimit_) {
            capCost
                += cost * capPerUnit_
                   + pointsCap_[solutionPointsIdx[j][clusters[j]][pointIdx]];
          }
          pointIdx++;
          distanceCost = std::max(cost, distanceCost);
        }
        // If the cluster size is higher than groupSize,
        // or the distance is higher than maxInternalDiameter_
        //-> start another cluster and save the cost of the current one.
        if (isLimitExceeded(solutionPoints[j][clusters[j]].size(),
                            distanceCost,
                            capCost,
                            groupSize)) {
          debugPrint(logger_,
                     CTS,
                     "Stree",
                     4,
                     "Created cluster of size {}, dia {:.3}, cap {:.3e}",
                     solutionPoints[j][clusters[j]].size(),
                     distanceCost,
                     capCost);
          // The cost is computed as the highest cost found on the current
          // cluster
          if (previousCosts[j] == 0) {
            previousCosts[j] = maxInternalDiameter_;
          }
          costs[j] += previousCosts[j];
          // A new cluster is defined
          clusters[j] = clusters[j] + 1;
          // The cost was already saved, so the same structure can be used for
          // the next cluster.
          previousCosts[j] = 0;
        } else {
          // Node will be a part of the current cluster, thus, save the highest
          // cost.
          previousCosts[j] = std::max(distanceCost, previousCosts[j]);
        }
        // Add vectors in case they are no allocated yet. (Depends if a new
        // cluster was defined above)
        if (solutions[j].size() < (clusters[j] + 1)) {
          solutions[j].emplace_back();
          solutionPoints[j].emplace_back();
          solutionPointsIdx[j].emplace_back();
        }
        // Save the current Point in it's respective cluster. (Depends if a new
        // cluster was defined above)
        solutionPoints[j][clusters[j]].push_back(p);
        solutionPointsIdx[j][clusters[j]].push_back(idx);
        solutions[j][clusters[j]].push_back(idx);
      }
    }
  }

  // Same computation as above, however, only for the first groupSize Points.
  for (unsigned i = 0; i < groupSize; ++i) {
    // This is because every solution after the first one skips a Point (starts
    // one late).
    for (unsigned j = (i + 1); j < groupSize; ++j) {
      if (solutions[j].size() < (clusters[j] + 1)) {
        solutions[j].emplace_back();
        solutionPoints[j].emplace_back();
        solutionPointsIdx[j].emplace_back();
      }
      // Thus here we will assign the Points missing from those solutions.
      const unsigned idx = thetaIndexVector_[i].second;
      const Point<double>& p = points_[idx];
      unsigned pointIdx = 0;
      double distanceCost = 0;
      double capCost = pointsCap_[idx];
      for (Point<double> comparisonPoint : solutionPoints[j][clusters[j]]) {
        const double cost = HTree_->computeDist(p, comparisonPoint);
        if (useMaxCapLimit_) {
          capCost += cost * capPerUnit_
                     + pointsCap_[solutionPointsIdx[j][clusters[j]][pointIdx]];
        }
        pointIdx++;
        distanceCost = std::max(cost, distanceCost);
      }

      if (isLimitExceeded(solutionPoints[j][clusters[j]].size(),
                          distanceCost,
                          capCost,
                          groupSize)) {
        debugPrint(logger_,
                   CTS,
                   "Stree",
                   4,
                   "Created cluster of size {}, dia {:.3}, cap {:.3e}",
                   solutionPoints[j][clusters[j]].size(),
                   distanceCost,
                   capCost);
        if (previousCosts[j] == 0) {
          previousCosts[j] = maxInternalDiameter_;
        }
        costs[j] += previousCosts[j];
        clusters[j] = clusters[j] + 1;
        previousCosts[j] = 0;
      } else {
        previousCosts[j] = std::max(distanceCost, previousCosts[j]);
      }
      if (solutions[j].size() < (clusters[j] + 1)) {
        solutions[j].emplace_back();
        solutionPoints[j].emplace_back();
        solutionPointsIdx[j].emplace_back();
      }
      solutionPoints[j][clusters[j]].push_back(p);
      solutionPointsIdx[j][clusters[j]].push_back(idx);
      solutions[j][clusters[j]].push_back(idx);
    }
  }

  unsigned bestSolution = 0;
  bool bestSolutionFound = false;

  // Find the solution with minimum cost.
  for (unsigned j = 1; j < groupSize; ++j) {
    if (logger_->debugCheck(CTS, "clustering", 1)) {
      // clang-format off
      logger_->report("Solution from group has {:0.3f} cost and {}"
                      " clustered sinks", j, costs[j], solutions[j].size());
      // clang-format on
    }
    if (costs[j] < bestSolutionCost_) {
      bestSolution = j;
      bestSolutionCost_ = costs[j];
      bestSolutionFound = true;
    }
  }
  debugPrint(logger_,
             CTS,
             "Stree",
             2,
             "Best solution cost = {:.3}",
             bestSolutionCost_);
  // Save the solution for the Tree Builder.
  if (bestSolutionFound) {
    bestSolution_ = solutions[bestSolution];
    // clang-format off
    debugPrint(logger_, CTS, "clustering", 1, "Best solution from group "
               "{} has cost of {:0.3f} and size of {}", bestSolution,
               bestSolutionCost_, bestSolution_.size());
    // clang-format on
  }
  if (logger_->debugCheck(CTS, "clustering", 2)) {
    size_t solnIndex = 0;
    for (const std::vector<unsigned>& soln : bestSolution_) {
      std::ostringstream s;
      s << "Solution " << solnIndex << "[" << soln.size() << "]: ";
      for (const unsigned i : soln) {
        s << i << " ";
      }
      logger_->debug(CTS, "clustering", "{}", s.str());
      ++solnIndex;
    }
  }

  return bestSolutionFound;
}

bool SinkClustering::isLimitExceeded(const unsigned size,
                                     const double cost,
                                     const double capCost,
                                     const unsigned sizeLimit)
{
  if (useMaxCapLimit_) {
    return (capCost > options_->getSinkBufferInputCap() * kMaxCapFactor);
  }

  return (size >= sizeLimit || cost > maxInternalDiameter_);
}

void SinkClustering::writePlotFile(unsigned groupSize)
{
  std::ofstream file("plot_clustering.py");
  file << "import numpy as np\n";
  file << "import matplotlib.pyplot as plt\n";
  file << "import matplotlib.path as mpath\n";
  file << "import matplotlib.lines as mlines\n";
  file << "import matplotlib.patches as mpatches\n";
  file << "from matplotlib.collections import PatchCollection\n\n";
  const vector<const char*> colors{"tab:blue",
                                   "tab:orange",
                                   "tab:green",
                                   "tab:red",
                                   "tab:purple",
                                   "tab:brown",
                                   "tab:pink",
                                   "tab:gray",
                                   "tab:olive",
                                   "tab:cyan"};
  const vector<char> markers{'*', 'o', 'x', '+', 'v', '^', '<', '>'};

  unsigned clusterCounter = 0;
  double totalWL = 0;
  for (const vector<unsigned>& clusters : bestSolution_) {
    const unsigned color = clusterCounter % colors.size();
    const unsigned marker = (clusterCounter / colors.size()) % markers.size();
    vector<Point<double>> clusterNodes;
    for (unsigned idx : clusters) {
      const Point<double>& point = points_[idx];
      clusterNodes.emplace_back(points_[idx]);
      file << "plt.scatter(" << point.getX() << ", " << point.getY() << ", c=\""
           << colors[color] << "\", marker='" << markers[marker] << "')\n";
    }
    const double wl = getWireLength(clusterNodes);
    totalWL += wl;
    clusterCounter++;
  }
  logger_->report(
      "Total cluster WL = {:.3} for {} clusters.", totalWL, clusterCounter);
  file << "plt.show()\n";
  file.close();
}

double SinkClustering::getWireLength(const vector<Point<double>>& points) const
{
  vector<int> vecX;
  vector<int> vecY;
  double driverX = 0;
  double driverY = 0;
  for (const auto& point : points) {
    driverX += point.getX();
    driverY += point.getY();
  }
  driverX /= points.size();
  driverY /= points.size();
  vecX.emplace_back(driverX * options_->getDbUnits());
  vecY.emplace_back(driverY * options_->getDbUnits());

  for (const auto& point : points) {
    vecX.emplace_back(point.getX() * options_->getDbUnits());
    vecY.emplace_back(point.getY() * options_->getDbUnits());
  }
  stt::SteinerTreeBuilder* sttBuilder = options_->getSttBuilder();
  const stt::Tree pdTree = sttBuilder->makeSteinerTree(vecX, vecY, 0);
  const int wl = pdTree.length;
  return wl / double(options_->getDbUnits());
}

}  // namespace cts
