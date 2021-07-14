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

#include "utl/Logger.h"

#include <iostream>

namespace cts {

using utl::CTS;

void PostCtsOpt::run()
{
  initSourceSinkDists();
  fixSourceSinkDists();
}

void PostCtsOpt::initSourceSinkDists()
{
  _clock->forEachSubNet([&](const Clock::SubNet& subNet) {
    if (!subNet.isLeafLevel()) {
      return;
    }

    computeNetSourceSinkDists(subNet);
  });

  _avgSourceSinkDist /= _clock->getNumSinks();
  _logger->info(CTS, 36, " Average source sink dist: {:.2f} dbu.", _avgSourceSinkDist);
}

void PostCtsOpt::computeNetSourceSinkDists(const Clock::SubNet& subNet)
{
  const ClockInst* driver = subNet.getDriver();
  Point<int> driverLoc = driver->getLocation();

  subNet.forEachSink([&](const ClockInst* sink) {
    Point<int> sinkLoc = sink->getLocation();
    std::string sinkName = sink->getName();
    DBU dist = driverLoc.computeDist(sinkLoc);
    _avgSourceSinkDist += dist;
    _sinkDistMap[sinkName] = dist;
  });
}

void PostCtsOpt::fixSourceSinkDists()
{
  _clock->forEachSubNet([&](Clock::SubNet& subNet) {
    if (!subNet.isLeafLevel()) {
      return;
    }

    fixNetSourceSinkDists(subNet);
  });

  _logger->info(CTS, 37, " Number of outlier sinks: {}", _numViolatingSinks);
}

void PostCtsOpt::fixNetSourceSinkDists(Clock::SubNet& subNet)
{
  ClockInst* driver = subNet.getDriver();

  subNet.forEachSink([&](ClockInst* sink) {
    std::string sinkName = sink->getName();
    unsigned dist = _sinkDistMap[sinkName];
    if (dist > 5 * _avgSourceSinkDist) {
      fixLongWire(subNet, driver, sink);
      ++_numViolatingSinks;
      if (_logger->debugCheck(utl::CTS, "HTree", 3)) {
        _logger->report("Fixing Sink off by dist {}, Name = {}", dist, sinkName);
      }
    }
  });
}

void PostCtsOpt::fixLongWire(Clock::SubNet& net,
                                   ClockInst* driver,
                                   ClockInst* sink)
{
  unsigned inputCap = _techChar->getActualMinInputCap();
  unsigned inputSlew = _techChar->getMaxSlew()/2;
  DBU wireSegmentUnit = _techChar->getLengthUnit() * _options->getDbUnits();
  Point<double> driverLoc((float) driver->getX() / wireSegmentUnit,
                          (float) driver->getY() / wireSegmentUnit);
  Point<double> sinkLoc((float) sink->getX() / wireSegmentUnit,
                        (float) sink->getY() / wireSegmentUnit);
  unsigned wireLength = driverLoc.computeDist(sinkLoc);
  unsigned remainingLength = wireLength;
  const unsigned slewThreshold = _options->getMaxSlew();
  const unsigned tolerance = 1;
  std::vector<unsigned> segments;
  unsigned charSegLength = _techChar->getMaxSegmentLength();
  unsigned numWires = wireLength / charSegLength;

  if (numWires >= 1) {
    for (int wireCount = 0; wireCount < numWires; ++wireCount) {
      unsigned outCap = 0, outSlew = 0;
      unsigned key = _builder->computeMinDelaySegment(charSegLength,
                                      inputSlew,
                                      inputCap,
                                      slewThreshold,
                                      tolerance,
                                      outSlew,
                                      outCap);
      if (_logger->debugCheck(utl::CTS, "HTree", 3)) {
        _techChar->reportSegment(key);
      }
      inputCap = std::max(outCap, inputCap);
      inputSlew = outSlew;
      segments.push_back(key);
    }
  }
  SegmentBuilder builder("clkbuf_opt_" + std::to_string(bufIndex) + "_",
                      "clknet_opt_" + std::to_string(bufIndex) + "_",
                      driverLoc, sinkLoc, segments, *_clock,
                      net, *_techChar, wireSegmentUnit);
  bufIndex++;
  builder.build(_options->getRootBuffer(), sink);
}
void PostCtsOpt::createSubClockNet(Clock::SubNet& net,
                                   ClockInst* driver,
                                   ClockInst* sink)
{
  std::string master = _options->getRootBuffer();

  Point<DBU> bufLoc = computeBufferLocation(driver, sink);
  ClockInst& clkBuffer = _clock->addClockBuffer(
      "clkbuf_opt_" + std::to_string(_numInsertedBuffers),
      master,
      bufLoc.getX(),
      bufLoc.getY());

  net.replaceSink(sink, &clkBuffer);

  Clock::SubNet& newSubNet
      = _clock->addSubNet("clknet_opt_" + std::to_string(_numInsertedBuffers));
  newSubNet.addInst(clkBuffer);
  newSubNet.addInst(*sink);

  ++_numInsertedBuffers;
}

Point<DBU> PostCtsOpt::computeBufferLocation(ClockInst* driver,
                                             ClockInst* sink) const
{
  Point<int> driverLoc = driver->getLocation();
  Point<int> sinkLoc = sink->getLocation();
  unsigned xDist = driverLoc.computeDistX(sinkLoc);
  unsigned yDist = driverLoc.computeDistY(sinkLoc);

  double xBufDistRatio = _bufDistRatio;
  if (sinkLoc.getX() < driverLoc.getX()) {
    xBufDistRatio = -_bufDistRatio;
  }

  double yBufDistRatio = _bufDistRatio;
  if (sinkLoc.getY() < driverLoc.getY()) {
    yBufDistRatio = -_bufDistRatio;
  }

  Point<DBU> bufLoc(driverLoc.getX() + xBufDistRatio * xDist,
                    driverLoc.getY() + yBufDistRatio * yDist);

  return bufLoc;
}

}  // namespace cts
