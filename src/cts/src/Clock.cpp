// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "Clock.h"

#include <algorithm>
#include <functional>
#include <limits>
#include <string>
#include <utility>
#include <vector>

#include "Util.h"
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

  std::ranges::sort(allPositionsX);
  std::ranges::sort(allPositionsY);

  const unsigned numSinks = allPositionsX.size();
  const unsigned numOutliers = percentile * numSinks;
  const int xMin = allPositionsX[numOutliers];
  const int xMax = allPositionsX[numSinks - numOutliers - 1];
  const int yMin = allPositionsY[numOutliers];
  const int yMax = allPositionsY[numSinks - numOutliers - 1];

  return Box<int>(xMin, yMin, xMax, yMax);
}

Box<double> Clock::computeSinkRegionClustered(
    const std::vector<std::pair<float, float>>& sinks)
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
