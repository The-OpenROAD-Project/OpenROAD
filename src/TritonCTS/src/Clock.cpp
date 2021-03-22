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

#include "Clock.h"

#include "utl/Logger.h"

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>

namespace cts {

void Clock::report(utl::Logger* _logger) const
{
  _logger->report(" ************************************");
  _logger->report(" *         Clock net report         *");
  _logger->report(" ************************************");
  _logger->report(" Net name: {}", _netName);
  _logger->report(" Clock pin: {} ({}, {})", _clockPin, _clockPinX, _clockPinY);
  _logger->report(" Number of sinks: ", _sinks.size());
  _logger->report(" ***********************************");

  _logger->report("\tPin name \tPos");
  forEachSink([&](const ClockInst& sink) {
    _logger->report("\t {} \t ({}, {})", sink.getName(), sink.getX(), sink.getY());
  });
}

Box<DBU> Clock::computeSinkRegion()
{
  double percentile = 0.01;

  std::vector<DBU> allPositionsX;
  std::vector<DBU> allPositionsY;
  forEachSink([&](const ClockInst& sink) {
    allPositionsX.push_back(sink.getX());
    allPositionsY.push_back(sink.getY());
  });

  std::sort(allPositionsX.begin(), allPositionsX.end());
  std::sort(allPositionsY.begin(), allPositionsY.end());

  unsigned numSinks = allPositionsX.size();
  unsigned numOutliers = percentile * numSinks;
  DBU xMin = allPositionsX[numOutliers];
  DBU xMax = allPositionsX[numSinks - numOutliers - 1];
  DBU yMin = allPositionsY[numOutliers];
  DBU yMax = allPositionsY[numSinks - numOutliers - 1];

  return Box<DBU>(xMin, yMin, xMax, yMax);
}

Box<double> Clock::computeSinkRegionClustered(
    std::vector<std::pair<float, float>> sinks)
{
  std::vector<double> allPositionsX;
  std::vector<double> allPositionsY;
  for (std::pair<float, float> sinkLocation : sinks) {
    allPositionsX.push_back(sinkLocation.first);
    allPositionsY.push_back(sinkLocation.second);
  }

  std::sort(allPositionsX.begin(), allPositionsX.end());
  std::sort(allPositionsY.begin(), allPositionsY.end());

  unsigned numSinks = allPositionsX.size();
  double xMin = allPositionsX[0];
  double xMax = allPositionsX[(allPositionsX.size() - 1)];
  double yMin = allPositionsY[0];
  double yMax = allPositionsY[(allPositionsY.size() - 1)];

  return Box<double>(xMin, yMin, xMax, yMax);
}

Box<double> Clock::computeNormalizedSinkRegion(double factor)
{
  Box<DBU> sinkRegion = computeSinkRegion();
  return sinkRegion.normalize(factor);
}

void Clock::forEachSink(const std::function<void(const ClockInst&)>& func) const
{
  for (const ClockInst& sink : _sinks) {
    func(sink);
  }
}

void Clock::forEachSink(const std::function<void(ClockInst&)>& func)
{
  for (ClockInst& sink : _sinks) {
    func(sink);
  }
}

void Clock::forEachClockBuffer(
    const std::function<void(const ClockInst&)>& func) const
{
  for (const ClockInst& clockBuffer : _clockBuffers) {
    func(clockBuffer);
  }
}

void Clock::forEachClockBuffer(const std::function<void(ClockInst&)>& func)
{
  for (ClockInst& clockBuffer : _clockBuffers) {
    func(clockBuffer);
  }
}

}  // namespace cts
