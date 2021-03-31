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

#include "tritoncts/TritonCTS.h"
#include "Clock.h"
#include "CtsOptions.h"
#include "TreeBuilder.h"
#include "LevelBalancer.h"

#include "opendb/db.h"


namespace cts {

using utl::CTS;

void LevelBalancer::run()
{
  debugPrint(_logger, CTS, "levelizer", 1, "Computing Max Tree Depth");
  unsigned maxTreeDepth = computeMaxTreeDepth(_root);
  _logger->info(CTS, 93, "Fixing tree levels for max depth {}", maxTreeDepth);
  fixTreeLevels(_root, 0, maxTreeDepth);
}

unsigned LevelBalancer::computeMaxTreeDepth(TreeBuilder* parent)
{
  unsigned maxDepth = 0;
  for (auto child : parent->getChildren()) {
    unsigned depth = computeMaxTreeDepth(child) + 1; //also count itself - non sink inst
    odb::dbObject* driverPin = child->getClock().getDriverPin();
    if (driverPin && driverPin->getObjectType() == odb::dbITermObj) {
      odb::dbInst* drivingInst = (static_cast<odb::dbITerm*> (driverPin))->getInst();
      debugPrint(_logger, CTS, "levelizer", 1,
          "Downstream depth is {} from driver {}", depth, child->getClock().getName());
      cgcLevelMap_[drivingInst] = std::make_pair(depth, child);
    }
    if (depth > maxDepth)
      maxDepth = depth;
  }
  return parent->getTreeBufLevels() + maxDepth;
}

void LevelBalancer::addBufferLevels(TreeBuilder* builder, std::vector<ClockInst*> cluster,
          Clock::SubNet* driverNet, unsigned bufLevels, const std::string nameSuffix)
{
  Clock::SubNet* prevLevelSubNet = driverNet;

  // Compute driver, receiver locations
  cts::DBU totalX = 0, totalY = 0;
  for (ClockInst* clockInstObj : cluster) {
    totalX += clockInstObj->getX();
    totalY += clockInstObj->getY();
  }
  cts::DBU centroidX = totalX/cluster.size();
  cts::DBU centroidY = totalY/cluster.size();
  cts::DBU driverX = prevLevelSubNet->getDriver()->getX();
  cts::DBU driverY = prevLevelSubNet->getDriver()->getY();

  for (unsigned level = 0; level < bufLevels; level++) {
    // Add buffer
    ClockInst& levelBuffer
      = builder->getClock().addClockBuffer("clkbuf_level_" + std::to_string(level) + "_" + nameSuffix,
          _options->getSinkBuffer(),
          driverX + (centroidX - driverX) * (level + 1) / (bufLevels + 1),
          driverY + (centroidY - driverY) * (level + 1) / (bufLevels + 1));

    // Add Net
    Clock::SubNet* levelSubNet
      = &(builder->getClock().addSubNet("clknet_level_" + std::to_string(level) + "_" + nameSuffix));

    // Connect to driving and driven nets
    prevLevelSubNet->addInst(levelBuffer);
    levelSubNet->addInst(levelBuffer);
    prevLevelSubNet = levelSubNet;
  }
  // Add sinks to leaf buffer
  for (ClockInst* clockInstObj : cluster) {
    prevLevelSubNet->addInst(*clockInstObj);
  }
  prevLevelSubNet->setLeafLevel(true);
}

void LevelBalancer::fixTreeLevels(TreeBuilder* builder, unsigned parentDepth, unsigned maxTreeDepth)
{
  unsigned currLevel = builder->getTreeBufLevels() + parentDepth;
  if (currLevel >= maxTreeDepth)
    return;

  _logger->report("Fixing from level {} (parent={} + current={}) to max {} for driver {}",
                    parentDepth + builder->getTreeBufLevels(), parentDepth, builder->getTreeBufLevels(),
                                        maxTreeDepth, builder->getClock().getName());
  unsigned clusterCnt = 0;
  builder->getClock().forEachSubNet([&](Clock::SubNet& subNet) {
    std::map<unsigned, std::vector<ClockInst*>> subClusters;
    subNet.forEachSink([&](ClockInst* clkInst) {
      if (!clkInst->getDbInputPin()) {
        subClusters.clear();
        return;
      }
      odb::dbInst* inst = clkInst->getDbInputPin()->getInst();
      if (cgcLevelMap_.find(inst) == cgcLevelMap_.end()) {
        subClusters[currLevel].emplace_back(clkInst);
      } else {
        subClusters[cgcLevelMap_[inst].first + currLevel].emplace_back(clkInst);
      }
    });
    if (!subClusters.size())
      return;

    clusterCnt++;
    subNet.removeSinks();
    subNet.setLeafLevel(false);
    unsigned subClusterCnt = 0;
    for (auto cluster : subClusters) {
      unsigned clusterLevel = cluster.first;
      unsigned bufLevels = maxTreeDepth - clusterLevel;
      subClusterCnt++;
      const std::string suffix = std::to_string(subClusterCnt) + "_" + std::to_string(clusterCnt);
      debugPrint(_logger, CTS, "levelizer", 1, "Adding buffer levels: {} to net {} with {} subClusters",
                          bufLevels, subNet.getName(), subClusterCnt);
      addBufferLevels(builder, cluster.second, &subNet, bufLevels, suffix);

      if (currLevel != clusterLevel) {
        for (ClockInst* clockInstObj : cluster.second) {
          fixTreeLevels(cgcLevelMap_[clockInstObj->getDbInputPin()->getInst()].second,
                        currLevel+bufLevels+1, maxTreeDepth);
        }
      }
    }
  });
}
}