// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <cmath>
#include <functional>
#include <limits>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "Clock.h"
#include "CtsObserver.h"
#include "CtsOptions.h"
#include "TechChar.h"
#include "TreeBuilder.h"
#include "Util.h"
#include "odb/db.h"
#include "odb/isotropy.h"

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
                 ClockSubNet& drivingSubNet,
                 const TechChar& techChar,
                 unsigned techCharDistUnit,
                 TreeBuilder* tree);

  void build(const std::string& forceBuffer = "");
  void forceBufferInSegment(const std::string& master);

  ClockSubNet* getDrivingSubNet() const { return drivingSubNet_; }
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
  ClockSubNet* drivingSubNet_;
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

    explicit LevelTopology(double length) : length_(length) {}

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

    ClockSubNet* getBranchDrivingSubNet(unsigned idx) const
    {
      return branchDrivingSubNet_[idx];
    }

    void setBranchDrivingSubNet(unsigned idx, ClockSubNet& subNet)
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
    void setCurrWl(int wl) { curr_Wl_ = wl; }
    int getCurrWl() const { return curr_Wl_; }

   private:
    double length_;
    unsigned outputSlew_ = 0;
    unsigned outputCap_ = 0;
    unsigned remainingLength_ = 0;
    int curr_Wl_ = 0;
    std::vector<unsigned> wireSegments_;
    std::vector<Point<double>> branchPointLoc_;
    std::vector<unsigned> parents_;
    std::vector<ClockSubNet*> branchDrivingSubNet_;
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
  Point<double> legalizeOneBuffer(Point<double> bufferLoc,
                                  const std::string& bufferName) override;
  void findLegalLocations(const Point<double>& parentPoint,
                          const Point<double>& branchPoint,
                          double x1,
                          double y1,
                          double x2,
                          double y2,
                          std::vector<Point<double>>& points);
  void addCandidateLoc(double x,
                       double y,
                       const Point<double>& parentPoint,
                       double x1,
                       double y1,
                       double x2,
                       double y2,
                       std::vector<Point<double>>& points)
  {
    Point<double> candidate(x, y);
    if ((candidate != parentPoint) && isAlongBbox(x, y, x1, y1, x2, y2)) {
      points.emplace_back(x, y);
    }
  }
  void addCandidatePointsAlongBlockage(const Point<double>& point,
                                       const Point<double>& parentPoint,
                                       double targetDist,
                                       int scalingFactor,
                                       std::vector<Point<double>>& candidates,
                                       odb::Direction2D direction);
  Point<double> findBestLegalLocation(
      double targetDist,
      const Point<double>& branchPoint,
      const Point<double>& parentPoint,
      const std::vector<Point<double>>& legalLocations,
      const std::vector<Point<double>>& sinks,
      double x1,
      double y1,
      double x2,
      double y2,
      int scalingFactor,
      odb::Direction2D direction);
  Point<double> adjustBestLegalLocation(double targetDist,
                                        const Point<double>& currLoc,
                                        const Point<double>& parentPoint,
                                        const std::vector<Point<double>>& sinks,
                                        double x1,
                                        double y1,
                                        double x2,
                                        double y2,
                                        int scalingFactor,
                                        odb::Direction2D direction);
  void checkLegalityAndCostSpecial(const Point<double>& oldLoc,
                                   const Point<double>& newLoc,
                                   const Point<double>& parentPoint,
                                   double targetDist,
                                   const std::vector<Point<double>>& sinks,
                                   int scalingFactor,
                                   double x1,
                                   double y1,
                                   double x2,
                                   double y2,
                                   Point<double>& bestLoc,
                                   double& sinkDist,
                                   double& bestSinkDist);
  bool adjustAlongBlockage(double targetDist,
                           const Point<double>& currLoc,
                           const Point<double>& parentPoint,
                           const std::vector<Point<double>>& sinks,
                           double x1,
                           double y1,
                           double x2,
                           double y2,
                           int scalingFactor,
                           Point<double>& bestLoc);
  Point<double> adjustBeyondBlockage(const Point<double>& branchPoint,
                                     const Point<double>& parentPoint,
                                     double targetDist,
                                     const std::vector<Point<double>>& sinks,
                                     int scalingFactor,
                                     odb::Direction2D direction);
  void checkLegalityAndCost(const Point<double>& oldLoc,
                            const Point<double>& newLoc,
                            const Point<double>& parentPoint,
                            double targetDist,
                            const std::vector<Point<double>>& sinks,
                            int scalingFactor,
                            Point<double>& bestLoc,
                            double& sinkDist,
                            double& bestSinkDist);
  void legalize();
  void legalizeDummy();
  void printHTree();
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
                                  int wirelengthThreshold,
                                  unsigned tolerance,
                                  unsigned& outputSlew,
                                  unsigned& outputCap,
                                  int& currWl) const;

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

  bool isNumberOfSinksTooSmall(unsigned numSinksPerSubRegion) const;

  double weightedDistance(const Point<double>& newLoc,
                          const Point<double>& oldLoc,
                          const std::vector<Point<double>>& sinks);
  void scalePosition(Point<double>& loc,
                     const Point<double>& parLoc,
                     double leng,
                     double scale);
  void adjustToplevelTopology(Point<double>& a,
                              Point<double>& b,
                              const Point<double>& parLoc);
  std::vector<unsigned> clusterDiameters() const { return clusterDiameters_; }
  std::vector<unsigned> clusterSizes() const { return clusterSizes_; }
  Point<double> resolveLocationCollision(
      const Point<double>& legalCenter) const;

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
  static constexpr int min_clustering_macro_sinks_ = 10;
  std::vector<unsigned> clusterDiameters_ = {50, 100, 200};
  std::vector<unsigned> clusterSizes_ = {10, 20, 30};
};

}  // namespace cts
