/////////////////////////////////////////////////////////////////////////////
//
// BSD 3-Clause License
//
// Copyright (c) 2019, University of California, San Diego.
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

#include "Util.h"
#include "CtsOptions.h"
#include "TechChar.h"

#include <limits>
#include <map>
#include <string>
#include <vector>

namespace utl {
class Logger;
} // namespace utl

namespace cts {

using utl::Logger;

class Matching
{
 public:
  Matching(unsigned p0, unsigned p1) : _p0(p0), _p1(p1) {}

  unsigned getP0() const { return _p0; }
  unsigned getP1() const { return _p1; }

 private:
  unsigned _p0;
  unsigned _p1;
};

class SinkClustering
{
 public:
  SinkClustering(CtsOptions* options, TechChar* techChar): _options(options), _logger(options->getLogger()),
                                   _techChar(techChar),
                                   _maxInternalDiameter(10), _capPerUnit(0.0),
                                  _useMaxCapLimit(options->getSinkClusteringUseMaxCap())
                                  {}

  void addPoint(double x, double y) { _points.emplace_back(x, y); }
  void addCap(float cap) { _pointsCap.emplace_back(cap);}
  void run();
  void run(unsigned groupSize, float maxDiameter, cts::DBU scaleFactor);
  unsigned getNumPoints() const { return _points.size(); }

  const std::vector<Matching>& allMatchings() const { return _matchings; }

  std::vector<std::vector<unsigned>> sinkClusteringSolution()
  {
    return _bestSolution;
  }

  double getWireLength(std::vector<Point<double>> points);
 private:
  void normalizePoints(float maxDiameter = 10);
  void computeAllThetas();
  void sortPoints();
  void findBestMatching();
  void writePlotFile();
  void findBestMatching(unsigned groupSize);
  void writePlotFile(unsigned groupSize);

  double computeTheta(double x, double y) const;
  unsigned numVertex(unsigned x, unsigned y) const;

  bool isLimitExceeded(unsigned size, double cost, double capCost, unsigned sizeLimit);
  bool isOne(double pos) const
  {
    return (1 - pos) < std::numeric_limits<double>::epsilon();
  }
  bool isZero(double pos) const
  {
    return pos < std::numeric_limits<double>::epsilon();
  }

  CtsOptions *_options;
  Logger *_logger;
  TechChar* _techChar;
  std::vector<Point<double>> _points;
  std::vector<float> _pointsCap;
  std::vector<std::pair<double, unsigned>> _thetaIndexVector;
  std::vector<Matching> _matchings;
  std::map<unsigned, std::vector<Point<double>>> _sinkClusters;
  std::vector<std::vector<unsigned>> _bestSolution;
  float _maxInternalDiameter;
  float _capPerUnit;
  bool _useMaxCapLimit;
  cts::DBU _scaleFactor;
};

}  // namespace cts
