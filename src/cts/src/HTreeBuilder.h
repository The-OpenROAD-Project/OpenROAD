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

#include <cmath>
#include <limits>

#include "CtsObserver.h"
#include "CtsOptions.h"
#include "TreeBuilder.h"

namespace cts {
class Graphics;

class SegmentBuilder
{
 public:
  SegmentBuilder(const std::string& instPrefix,
                 const std::string& netPrefix,
                 const Point<double>& root,
                 const Point<double>& target,
                 const std::vector<unsigned>& techCharWires,
                 Clock& clock,
                 Clock::SubNet& drivingSubNet,
                 const TechChar& techChar,
                 const unsigned techCharDistUnit,
                 TreeBuilder* tree);

  void build(const std::string& forceBuffer = "");
  void forceBufferInSegment(const std::string& master);

  Clock::SubNet* getDrivingSubNet() const { return drivingSubNet_; }
  unsigned getNumBufferLevels() const { return numBufferLevels_; }
  TreeBuilder* getTree() const { return tree_; }

 private:
  const std::string instPrefix_;
  const std::string netPrefix_;
  const Point<double> root_;
  const Point<double> target_;
  const std::vector<unsigned> techCharWires_;
  const TechChar* techChar_;
  const unsigned techCharDistUnit_;
  Clock* clock_;
  Clock::SubNet* drivingSubNet_;
  TreeBuilder* tree_;
  unsigned numBufferLevels_ = 0;
};

//-----------------------------------------------------------------------------
class HTreeBuilder : public TreeBuilder
{
  class LevelTopology
  {
   public:
    static constexpr unsigned NO_PARENT = std::numeric_limits<unsigned>::max();

    explicit LevelTopology(double length) : length_(length){};

    void addWireSegment(unsigned idx) { wireSegments_.push_back(idx); }

    unsigned addBranchingPoint(const Point<double>& loc, unsigned parent)
    {
      branchPointLoc_.push_back(loc);
      parents_.push_back(parent);
      branchSinkLocs_.resize(branchPointLoc_.size());
      branchDrivingSubNet_.resize(branchPointLoc_.size(), nullptr);
      return branchPointLoc_.size() - 1;
    }

    void addSinkToBranch(unsigned branchIdx, const Point<double>& sinkLoc)
    {
      branchSinkLocs_[branchIdx].push_back(sinkLoc);
    }

    unsigned getBranchingPointSize() { return branchPointLoc_.size(); }

    Point<double>& getBranchingPoint(unsigned idx)
    {
      return branchPointLoc_[idx];
    }

    unsigned getBranchingPointParentIdx(unsigned idx) const
    {
      return parents_[idx];
    }

    double getLength() const { return length_; }
    void setLength(double x) { length_ = x; }

    void forEachBranchingPoint(
        const std::function<void(unsigned, Point<double>)>& func) const
    {
      for (unsigned idx = 0; idx < branchPointLoc_.size(); ++idx) {
        func(idx, branchPointLoc_[idx]);
      }
    }

    Clock::SubNet* getBranchDrivingSubNet(unsigned idx) const
    {
      return branchDrivingSubNet_[idx];
    }

    void setBranchDrivingSubNet(unsigned idx, Clock::SubNet& subNet)
    {
      branchDrivingSubNet_[idx] = &subNet;
    }

    const std::vector<unsigned>& getWireSegments() const
    {
      return wireSegments_;
    }

    const std::vector<Point<double>>& getBranchSinksLocations(
        unsigned branchIdx) const
    {
      return branchSinkLocs_[branchIdx];
    }

    void setOutputSlew(unsigned slew) { outputSlew_ = slew; }
    unsigned getOutputSlew() const { return outputSlew_; }
    void setOutputCap(unsigned cap) { outputCap_ = cap; }
    unsigned getOutputCap() const { return outputCap_; }
    void setRemainingLength(unsigned length) { remainingLength_ = length; }
    unsigned getRemainingLength() const { return remainingLength_; }

   private:
    double length_;
    unsigned outputSlew_ = 0;
    unsigned outputCap_ = 0;
    unsigned remainingLength_ = 0;
    std::vector<unsigned> wireSegments_;
    std::vector<Point<double>> branchPointLoc_;
    std::vector<unsigned> parents_;
    std::vector<Clock::SubNet*> branchDrivingSubNet_;
    std::vector<std::vector<Point<double>>> branchSinkLocs_;
  };

 public:
  HTreeBuilder(CtsOptions* options,
               Clock& net,
               TreeBuilder* parent,
               utl::Logger* logger,
               odb::dbDatabase* db)
      : TreeBuilder(options, net, parent, logger, db)
  {
  }

  void run() override;
  bool moveAlongBlockageBoundary(const Point<double>& parentPoint,
                                 Point<double>& branchPoint,
                                 double x1,
                                 double y1,
                                 double x2,
                                 double y2);
  void legalize();
  void legalizeDummy();
  void plotSolution();
  std::string plotHTree();
  unsigned findSibling(LevelTopology& topology, unsigned i, unsigned par);
  Point<double>& findSiblingLoc(LevelTopology& topology,
                                unsigned i,
                                unsigned par)
  {
    unsigned j = findSibling(topology, i, par);
    return topology.getBranchingPoint(j);
  }

  std::vector<LevelTopology> getTopologyVector() const
  {
    return topologyForEachLevel_;
  }

  Box<double> getSinkRegion() const { return sinkRegion_; }

  int getWireSegmentUnit() const { return wireSegmentUnit_; }

  unsigned computeMinDelaySegment(unsigned length,
                                  unsigned inputSlew,
                                  unsigned inputCap,
                                  unsigned slewThreshold,
                                  unsigned tolerance,
                                  unsigned& outputSlew,
                                  unsigned& outputCap) const;

 private:
  void initSinkRegion();
  void computeLevelTopology(unsigned level, double width, double height);
  unsigned computeNumberOfSinksPerSubRegion(unsigned level) const;
  void computeSubRegionSize(unsigned level,
                            double& width,
                            double& height) const;
  unsigned computeMinDelaySegment(unsigned length) const;
  unsigned computeMinDelaySegment(unsigned length,
                                  unsigned inputSlew,
                                  unsigned inputCap,
                                  unsigned slewThreshold,
                                  unsigned tolerance,
                                  unsigned& outputSlew,
                                  unsigned& outputCap,
                                  bool forceBuffer,
                                  int expectedLength) const;
  void reportWireSegment(unsigned key) const;
  void createClockSubNets();
  void createSingleBufferClockNet();
  void initTopLevelSinks(std::vector<std::pair<float, float>>& sinkLocations,
                         std::vector<const ClockInst*>& sinkInsts);
  void initSecondLevelSinks(std::vector<std::pair<float, float>>& sinkLocations,
                            std::vector<const ClockInst*>& sinkInsts);
  void computeBranchSinks(
      const LevelTopology& topology,
      unsigned branchIdx,
      std::vector<std::pair<float, float>>& sinkLocations) const;

  bool isVertical(unsigned level) const
  {
    return level % 2 == (sinkRegion_.getHeight() >= sinkRegion_.getWidth());
  }
  bool isHorizontal(unsigned level) const { return !isVertical(level); }

  unsigned computeGridSizeX(unsigned level) const
  {
    return std::pow(2, (level + 1) / 2);
  }
  unsigned computeGridSizeY(unsigned level) const
  {
    return std::pow(2, level / 2);
  }

  void computeBranchingPoints(unsigned level, LevelTopology& topology);
  void refineBranchingPointsWithClustering(
      LevelTopology& topology,
      unsigned level,
      unsigned branchPtIdx1,
      unsigned branchPtIdx2,
      const Point<double>& rootLocation,
      const std::vector<std::pair<float, float>>& sinks);
  void preClusteringOpt(const std::vector<std::pair<float, float>>& sinks,
                        std::vector<std::pair<float, float>>& points,
                        std::vector<unsigned>& mapSinkToPoint);
  void preSinkClustering(const std::vector<std::pair<float, float>>& sinks,
                         const std::vector<const ClockInst*>& sinkInsts,
                         float maxDiameter,
                         unsigned clusterSize,
                         bool secondLevel = false);
  void assignSinksToBranches(
      LevelTopology& topology,
      unsigned branchPtIdx1,
      unsigned branchPtIdx2,
      const std::vector<std::pair<float, float>>& sinks,
      const std::vector<std::pair<float, float>>& points,
      const std::vector<unsigned>& mapSinkToPoint,
      const std::vector<std::vector<unsigned>>& clusters);

  bool isSubRegionTooSmall(double width, double height) const
  {
    return width < minLengthSinkRegion_ || height < minLengthSinkRegion_;
  }

  bool isNumberOfSinksTooSmall(unsigned numSinksPerSubRegion) const
  {
    return numSinksPerSubRegion < numMaxLeafSinks_;
  }

 private:
  Box<double> sinkRegion_;
  std::vector<LevelTopology> topologyForEachLevel_;
  std::map<Point<double>, ClockInst*> mapLocationToSink_;
  std::vector<std::pair<float, float>> topLevelSinksClustered_;

  int wireSegmentUnit_ = 0;
  unsigned minInputCap_ = 0;
  unsigned numMaxLeafSinks_ = 0;
  unsigned minLengthSinkRegion_ = 0;
  unsigned clockTreeMaxDepth_ = 0;
  static constexpr int min_clustering_sinks_ = 200;
};

}  // namespace cts
