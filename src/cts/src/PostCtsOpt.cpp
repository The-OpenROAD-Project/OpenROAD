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

#include "PostCtsOpt.h"

#include <iostream>

#include "utl/Logger.h"

namespace cts {

using utl::CTS;

PostCtsOpt::PostCtsOpt(TreeBuilder* builder,
                       CtsOptions* options,
                       TechChar* techChar,
                       Logger* logger)
    : clock_(&(builder->getClock())),
      options_(options),
      techChar_(techChar),
      logger_(logger),
      builder_((HTreeBuilder*) builder)
{
  bufDistRatio_ = options_->getBufDistRatio();
}

void PostCtsOpt::run()
{
  initSourceSinkDists();
  fixSourceSinkDists();
}

void PostCtsOpt::initSourceSinkDists()
{
  clock_->forEachSubNet([&](const Clock::SubNet& subNet) {
    if (!subNet.isLeafLevel()) {
      return;
    }

    computeNetSourceSinkDists(subNet);
  });

  avgSourceSinkDist_ /= clock_->getNumSinks();
  logger_->info(
      CTS, 36, " Average source sink dist: {:.2f} dbu.", avgSourceSinkDist_);
}

void PostCtsOpt::computeNetSourceSinkDists(const Clock::SubNet& subNet)
{
  const ClockInst* driver = subNet.getDriver();
  const Point<int> driverLoc = driver->getLocation();

  subNet.forEachSink([&](const ClockInst* sink) {
    const Point<int> sinkLoc = sink->getLocation();
    const std::string sinkName = sink->getName();
    const int dist = driverLoc.computeDist(sinkLoc);
    avgSourceSinkDist_ += dist;
    sinkDistMap_[sinkName] = dist;
  });
}

void PostCtsOpt::fixSourceSinkDists()
{
  clock_->forEachSubNet([&](Clock::SubNet& subNet) {
    if (!subNet.isLeafLevel()) {
      return;
    }

    fixNetSourceSinkDists(subNet);
  });

  logger_->info(CTS, 37, " Number of outlier sinks: {}.", numViolatingSinks_);
}

void PostCtsOpt::fixNetSourceSinkDists(Clock::SubNet& subNet)
{
  ClockInst* driver = subNet.getDriver();

  subNet.forEachSink([&](ClockInst* sink) {
    const std::string sinkName = sink->getName();
    const unsigned dist = sinkDistMap_[sinkName];
    if (dist > 5 * avgSourceSinkDist_) {
      fixLongWire(subNet, driver, sink);
      ++numViolatingSinks_;
      if (logger_->debugCheck(utl::CTS, "HTree", 3)) {
        logger_->report(
            "Fixing Sink off by dist {}, Name = {}", dist, sinkName);
      }
    }
  });
}

void PostCtsOpt::fixLongWire(Clock::SubNet& net,
                             ClockInst* driver,
                             ClockInst* sink)
{
  unsigned inputCap = techChar_->getActualMinInputCap();
  unsigned inputSlew = techChar_->getMaxSlew() / 2;
  const int wireSegmentUnit = techChar_->getLengthUnit() * options_->getDbUnits();
  const Point<double> driverLoc((float) driver->getX() / wireSegmentUnit,
                          (float) driver->getY() / wireSegmentUnit);
  const Point<double> sinkLoc((float) sink->getX() / wireSegmentUnit,
                        (float) sink->getY() / wireSegmentUnit);
  const unsigned wireLength = driverLoc.computeDist(sinkLoc);
  const unsigned slewThreshold = options_->getMaxSlew();
  const unsigned tolerance = 1;
  std::vector<unsigned> segments;
  const unsigned charSegLength = techChar_->getMaxSegmentLength();
  const unsigned numWires = wireLength / charSegLength;

  if (numWires >= 1) {
    for (int wireCount = 0; wireCount < numWires; ++wireCount) {
      unsigned outCap = 0, outSlew = 0;
      const unsigned key = builder_->computeMinDelaySegment(charSegLength,
                                                      inputSlew,
                                                      inputCap,
                                                      slewThreshold,
                                                      tolerance,
                                                      outSlew,
                                                      outCap);
      if (logger_->debugCheck(utl::CTS, "HTree", 3)) {
        techChar_->reportSegment(key);
      }
      inputCap = std::max(outCap, inputCap);
      inputSlew = outSlew;
      segments.push_back(key);
    }
  }
  SegmentBuilder builder("clkbuf_opt_" + std::to_string(bufIndex) + "_",
                         "clknet_opt_" + std::to_string(bufIndex) + "_",
                         driverLoc,
                         sinkLoc,
                         segments,
                         *clock_,
                         net,
                         *techChar_,
                         wireSegmentUnit,
                         builder_);
  bufIndex++;
  builder.build(options_->getRootBuffer(), sink);
}
void PostCtsOpt::createSubClockNet(Clock::SubNet& net,
                                   ClockInst* driver,
                                   ClockInst* sink)
{
  const std::string master = options_->getRootBuffer();

  const Point<int> bufLoc = computeBufferLocation(driver, sink);
  ClockInst& clkBuffer = clock_->addClockBuffer(
      "clkbuf_opt_" + std::to_string(numInsertedBuffers_),
      master,
      bufLoc.getX(),
      bufLoc.getY());
  builder_->addTreeLevelBuffer(&clkBuffer);

  net.replaceSink(sink, &clkBuffer);

  Clock::SubNet& newSubNet
      = clock_->addSubNet("clknet_opt_" + std::to_string(numInsertedBuffers_));
  newSubNet.addInst(clkBuffer);
  newSubNet.addInst(*sink);

  ++numInsertedBuffers_;
}

Point<int> PostCtsOpt::computeBufferLocation(ClockInst* driver,
                                             ClockInst* sink) const
{
  const Point<int> driverLoc = driver->getLocation();
  const Point<int> sinkLoc = sink->getLocation();
  const unsigned xDist = driverLoc.computeDistX(sinkLoc);
  const unsigned yDist = driverLoc.computeDistY(sinkLoc);

  double xBufDistRatio = bufDistRatio_;
  if (sinkLoc.getX() < driverLoc.getX()) {
    xBufDistRatio = -bufDistRatio_;
  }

  double yBufDistRatio = bufDistRatio_;
  if (sinkLoc.getY() < driverLoc.getY()) {
    yBufDistRatio = -bufDistRatio_;
  }

  const Point<int> bufLoc(driverLoc.getX() + xBufDistRatio * xDist,
                          driverLoc.getY() + yBufDistRatio * yDist);

  return bufLoc;
}

}  // namespace cts
