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

 private:
  void normalizePoints(float maxDiameter = 10);
  void computeAllThetas();
  void sortPoints();
  void writePlotFile();
  bool findBestMatching(unsigned groupSize);
  void writePlotFile(unsigned groupSize);

  double computeTheta(double x, double y) const;
  unsigned numVertex(unsigned x, unsigned y) const;

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
  std::vector<std::pair<double, unsigned>> thetaIndexVector_;
  std::vector<Matching> matchings_;
  std::map<unsigned, std::vector<Point<double>>> sinkClusters_;
  std::vector<std::vector<unsigned>> bestSolution_;
  float maxInternalDiameter_;
  float capPerUnit_;
  bool useMaxCapLimit_;
  int scaleFactor_;
  static constexpr double kMaxCapFactor = 10;
  HTreeBuilder* HTree_;
  bool firstRun_ = true;
  double xSpan_ = 0.0;
  double ySpan_ = 0.0;
  double bestSolutionCost_ = std::numeric_limits<double>::max();
};

}  // namespace cts
