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

#include "Clock.h"

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>

#include "sta/ParseBus.hh"
#include "utl/Logger.h"

namespace cts {

Clock::Clock(const std::string& netName,
             const std::string& clockPin,
             const std::string& sdcClockName,
             int clockPinX,
             int clockPinY)
    : clockPin_(clockPin),
      sdcClockName_(sdcClockName),
      clockPinX_(clockPinX),
      clockPinY_(clockPinY)
{
  // Hierarchy delimiters in the net name must be escape.  We use
  // the name to construct buffer names later and the delimiters
  // will confuse downstream tools like read_spef.
  netName_ = sta::escapeChars(netName.c_str(), '/', '\0', '\\');
}

void Clock::report(utl::Logger* logger) const
{
  logger->report(" ************************************");
  logger->report(" *         Clock net report         *");
  logger->report(" ************************************");
  logger->report(" Net name: {}", netName_);
  logger->report(" Clock pin: {} ({}, {})", clockPin_, clockPinX_, clockPinY_);
  logger->report(" Number of sinks: ", sinks_.size());
  logger->report(" ***********************************");

  logger->report("\tPin name \tPos");
  forEachSink([&](const ClockInst& sink) {
    logger->report(
        "\t {} \t ({}, {})", sink.getName(), sink.getX(), sink.getY());
  });
}

Box<int> Clock::computeSinkRegion()
{
  const double percentile = 0.01;

  std::vector<int> allPositionsX;
  std::vector<int> allPositionsY;
  forEachSink([&](const ClockInst& sink) {
    allPositionsX.push_back(sink.getX());
    allPositionsY.push_back(sink.getY());
  });

  std::sort(allPositionsX.begin(), allPositionsX.end());
  std::sort(allPositionsY.begin(), allPositionsY.end());

  const unsigned numSinks = allPositionsX.size();
  const unsigned numOutliers = percentile * numSinks;
  const int xMin = allPositionsX[numOutliers];
  const int xMax = allPositionsX[numSinks - numOutliers - 1];
  const int yMin = allPositionsY[numOutliers];
  const int yMax = allPositionsY[numSinks - numOutliers - 1];

  return Box<int>(xMin, yMin, xMax, yMax);
}

Box<double> Clock::computeSinkRegionClustered(
    std::vector<std::pair<float, float>> sinks)
{
  auto xMin = std::numeric_limits<float>::max();
  auto xMax = std::numeric_limits<float>::lowest();
  auto yMin = std::numeric_limits<float>::max();
  auto yMax = std::numeric_limits<float>::lowest();

  for (const auto& [sinkX, sinkY] : sinks) {
    xMin = std::min(xMin, sinkX);
    xMax = std::max(xMax, sinkX);
    yMin = std::min(yMin, sinkY);
    yMax = std::max(yMax, sinkY);
  }

  return Box<double>(xMin, yMin, xMax, yMax);
}

Box<double> Clock::computeNormalizedSinkRegion(double factor)
{
  return computeSinkRegion().normalize(factor);
}

void Clock::forEachSink(const std::function<void(const ClockInst&)>& func) const
{
  for (const ClockInst& sink : sinks_) {
    func(sink);
  }
}

void Clock::forEachSink(const std::function<void(ClockInst&)>& func)
{
  for (ClockInst& sink : sinks_) {
    func(sink);
  }
}

void Clock::forEachClockBuffer(
    const std::function<void(const ClockInst&)>& func) const
{
  for (const ClockInst& clockBuffer : clockBuffers_) {
    func(clockBuffer);
  }
}

void Clock::forEachClockBuffer(const std::function<void(ClockInst&)>& func)
{
  for (ClockInst& clockBuffer : clockBuffers_) {
    func(clockBuffer);
  }
}

}  // namespace cts
