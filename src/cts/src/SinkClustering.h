/////////////////////////////////////////////////////////////////////////////
//
// BSD 3-Clause License
//
// Copyright (c) 2019, The Regents of the University of California
// All rights reserved.
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

#pragma once

#include <limits>
#include <map>
#include <string>
#include <vector>

#include "CtsOptions.h"
#include "HTreeBuilder.h"
#include "TechChar.h"
#include "Util.h"

namespace utl {
class Logger;
}  // namespace utl

namespace cts {

using utl::Logger;

class Matching
{
 public:
  Matching(unsigned p0, unsigned p1) : p_0(p0), p_1(p1) {}

  unsigned getP0() const { return p_0; }
  unsigned getP1() const { return p_1; }

 private:
  const unsigned p_0;
  const unsigned p_1;
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
  Logger* logger_;
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
  static constexpr double max_cap__factor_ = 10;
  HTreeBuilder* HTree_;
  bool firstRun_ = true;
  double xSpan_ = 0.0;
  double ySpan_ = 0.0;
  double bestSolutionCost_ = std::numeric_limits<double>::max();
};

}  // namespace cts
