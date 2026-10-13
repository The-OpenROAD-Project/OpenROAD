// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <limits>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "CtsOptions.h"
#include "HTreeBuilder.h"
#include "TechChar.h"
#include "Util.h"
#include "utl/Logger.h"

namespace cts {

class Matching
{
 public:
  Matching(unsigned p0, unsigned p1) : p0_(p0), p1_(p1) {}

  unsigned getP0() const { return p0_; }
  unsigned getP1() const { return p1_; }

 private:
  const unsigned p0_;
  const unsigned p1_;
};

class SinkClustering
{
 public:
  SinkClustering(const CtsOptions* options,
                 TechChar* techChar,
                 HTreeBuilder* HTree);

  void addPoint(double x, double y) { points_.emplace_back(x, y); }
  void addCap(float cap) { pointsCap_.emplace_back(cap); }
  void run(unsigned groupSize,
           float maxDiameter,
           int scaleFactor,
           unsigned& bestSize,
           float& bestDiameter);
  unsigned getNumPoints() const { return points_.size(); }

  const std::vector<Matching>& allMatchings() const { return matchings_; }

  const std::vector<std::vector<unsigned>>& sinkClusteringSolution() const
  {
    return bestSolution_;
  }

  double getWireLength(const std::vector<Point<double>>& points) const;
  int getScaleFactor() const { return scaleFactor_; }
  double getMaxDiameter() const { return max_diameter_; }
  double getMaxSize() const { return max_size_; }

 private:
  void normalizePoints(float maxDiameter = 10);
  void computeAllThetas();
  void sortPoints();
  void writePlotFile();
  void repairClusteringSolution(
      unsigned groupSize,
      std::vector<std::vector<Point<double>>>& solutionPoints,
      std::vector<std::vector<unsigned>>& solutionPointsIdx,
      std::vector<std::vector<unsigned>>& solutions,
      int& single_cluster_count,
      int& solved_cluster_count);
  bool findBestMatching(unsigned groupSize);
  void writePlotFile(unsigned groupSize);

  double computeTheta(double x, double y) const;
  unsigned numVertex(unsigned x, unsigned y) const;

  // Distance cost between two points identified by their indices, equivalent to
  // HTree_->computeDist(points_[idxA], points_[idxB]) but using the precomputed
  // pointsInsDelay_ array instead of per-call hash-map lookups. The
  // floating-point evaluation order is preserved to keep results bit-identical.
  double distCost(unsigned idxA,
                  const Point<double>& a,
                  unsigned idxB,
                  const Point<double>& b) const
  {
    return a.computeDist(b) + pointsInsDelay_[idxA] + pointsInsDelay_[idxB];
  }

  bool isLimitExceeded(unsigned size,
                       double cost,
                       double capCost,
                       unsigned sizeLimit);
  static bool isOne(double pos);
  static bool isZero(double pos);

  const CtsOptions* options_;
  utl::Logger* logger_;
  const TechChar* techChar_;
  std::vector<Point<double>> points_;
  std::vector<float> pointsCap_;
  // Per-point sink insertion delay, precomputed once (same values as
  // HTree_->getSinkInsertionDelay(points_[idx])). Used to avoid a hash-map
  // lookup per distance evaluation in the matching inner loops.
  std::vector<double> pointsInsDelay_;
  std::vector<std::pair<double, unsigned>> thetaIndexVector_;
  std::vector<Matching> matchings_;
  std::map<unsigned, std::vector<Point<double>>> sinkClusters_;
  std::vector<std::vector<unsigned>> bestSolution_;
  float maxInternalDiameter_;
  float capPerUnit_;
  bool use_max_diameter_;
  bool use_max_size_;
  bool useMaxCapLimit_;
  int scaleFactor_;
  static constexpr double kMaxCapFactor = 10;
  HTreeBuilder* HTree_;
  bool firstRun_ = true;
  double xSpan_ = 0.0;
  double ySpan_ = 0.0;
  double max_diameter_ = 0.0;
  double max_size_ = 0.0;
  double bestSolutionCost_ = std::numeric_limits<double>::max();
};

}  // namespace cts
