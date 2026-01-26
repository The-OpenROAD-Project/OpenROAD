// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <cstddef>
#include <deque>
#include <functional>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "Clock.h"
#include "CtsOptions.h"
#include "TechChar.h"
#include "Util.h"
#include "boost/functional/hash.hpp"
#include "boost/unordered/unordered_map.hpp"
#include "boost/unordered/unordered_set.hpp"
#include "odb/db.h"
#include "odb/geom.h"
#include "utl/Logger.h"

namespace utl {
class Logger;
}  // namespace utl

namespace cts {

struct pointHash
{
  std::size_t operator()(const cts::Point<double>& point) const
  {
    std::pair<double, double> dPair
        = std::make_pair(point.getX(), point.getY());
    return boost::hash_value(dPair);
  }
};

struct pointEqual
{
  bool operator()(const cts::Point<double>& p1,
                  const cts::Point<double>& p2) const
  {
    return p1 == p2;
  }
};

enum class TreeType
{
  RegularTree = 0,  // regular tree that drives both macros and registers
  MacroTree = 1,    // parent tree that drives only macro cells with ins delays
  RegisterTree = 2  // child tree that drives only registers without ins delays
};

class TreeBuilder
{
 public:
  TreeBuilder(CtsOptions* options,
              Clock& clk,
              TreeBuilder* parent,
              utl::Logger* logger,
              odb::dbDatabase* db = nullptr)
      : options_(options),
        clock_(clk),
        parent_(parent),
        logger_(logger),
        db_(db)
  {
    if (parent) {
      parent->children_.emplace_back(this);
    }
  }
  virtual ~TreeBuilder() = default;

  virtual void run() = 0;
  void mergeBlockages();
  void initBlockages();
  void setTechChar(TechChar& techChar) { techChar_ = &techChar; }
  const Clock& getClock() const { return clock_; }
  Clock& getClock() { return clock_; }
  void addChild(TreeBuilder* child) { children_.emplace_back(child); }
  std::vector<TreeBuilder*> getChildren() const { return children_; }
  TreeBuilder* getParent() const { return parent_; }
  bool isLeafTree();
  unsigned getTreeBufLevels() const { return treeBufLevels_; }
  void addFirstLevelSinkDriver(ClockInst* inst)
  {
    first_level_sink_drivers_.insert(inst);
  }
  void addSecondLevelSinkDriver(ClockInst* inst)
  {
    second_level_sink_drivers_.insert(inst);
  }
  void addTreeLevelBuffer(ClockInst* inst) { tree_level_buffers_.insert(inst); }
  bool isAnyTreeBuffer(ClockInst* inst)
  {
    return isLeafBuffer(inst) || isLevelBuffer(inst);
  }
  bool isLeafBuffer(ClockInst* inst)
  {
    return isFirstLevelSinkDriver(inst) || isSecondLevelSinkDriver(inst);
  }
  bool isFirstLevelSinkDriver(ClockInst* inst)
  {
    return first_level_sink_drivers_.find(inst)
           != first_level_sink_drivers_.end();
  }
  bool isSecondLevelSinkDriver(ClockInst* inst)
  {
    return second_level_sink_drivers_.find(inst)
           != second_level_sink_drivers_.end();
  }
  bool isLevelBuffer(ClockInst* inst)
  {
    return tree_level_buffers_.find(inst) != tree_level_buffers_.end();
  }
  void setDb(odb::dbDatabase* db) { db_ = db; }
  void setLogger(utl::Logger* logger) { logger_ = logger; }
  bool isInsideBbox(double x,
                    double y,
                    double x1,
                    double y1,
                    double x2,
                    double y2)
  {
    if ((x > x1) && (x < x2) && (y > y1) && (y < y2)) {
      return true;
    }
    return false;
  }
  bool isAlongBbox(double x,
                   double y,
                   double x1,
                   double y1,
                   double x2,
                   double y2)
  {
    if ((fuzzyEqual(x, x1) || (fuzzyEqual(x, x2)))
        && (fuzzyEqualOrGreater(y, y1) && fuzzyEqualOrSmaller(y, y2))) {
      return true;
    }
    if ((fuzzyEqual(y, y1) || (fuzzyEqual(y, y2)))
        && (fuzzyEqualOrGreater(x, x1) && fuzzyEqualOrSmaller(x, x2))) {
      return true;
    }
    return false;
  }
  bool checkLegalitySpecial(Point<double> loc,
                            double x1,
                            double y1,
                            double x2,
                            double y2,
                            int scalingFactor);
  bool findBlockage(const Point<double>& bufferLoc,
                    double scalingUnit,
                    double& x1,
                    double& y1,
                    double& x2,
                    double& y2);
  virtual Point<double> legalizeOneBuffer(Point<double> bufferLoc,
                                          const std::string& bufferName);

  void addCandidatePoint(double x,
                         double y,
                         Point<double>& point,
                         std::vector<Point<double>>& candidates)
  {
    point.setX(x);
    point.setY(y);
    candidates.emplace_back(point);
  }
  utl::Logger* getLogger() { return logger_; }
  double getBufferWidth() { return bufferWidth_; }
  double getBufferHeight() { return bufferHeight_; }
  bool checkLegalityLoc(const Point<double>& bufferLoc, int scalingFactor);
  bool isOccupiedLoc(const Point<double>& bufferLoc);
  void commitLoc(const Point<double>& bufferLoc);
  void uncommitLoc(const Point<double>& bufferLoc);
  void commitMoveLoc(const Point<double>& oldLoc, const Point<double>& newLoc);
  bool sinkHasInsertionDelay(const Point<double>& sink)
  {
    return (insertionDelays_.find(sink) != insertionDelays_.end());
  }
  void setSinkInsertionDelay(const Point<double>& sink, double insDelay)
  {
    insertionDelays_[sink] = insDelay;
  }
  double getSinkInsertionDelay(const Point<double>& sink)
  {
    auto it = insertionDelays_.find(sink);
    if (it != insertionDelays_.end()) {
      // clang-format off
      debugPrint(logger_, utl::CTS, "clustering", 4, "sink {} has insertion "
		 "delay {:0.3f}", sink, it->second);
      // clang-format on
      return it->second;
    }
    return 0.0;
  }
  double computeDist(const Point<double>& x, const Point<double>& y)
  {
    return x.computeDist(y) + getSinkInsertionDelay(x)
           + getSinkInsertionDelay(y);
  }
  TreeType getTreeType() const { return type_; }
  void setTreeType(TreeType type) { type_ = type; }
  std::string getTreeTypeAsString() const
  {
    switch (type_) {
      case TreeType::RegularTree:
        return "regular";
      case TreeType::MacroTree:
        return "macro";
      case TreeType::RegisterTree:
        return "register";
    }
    return "unknown";
  }

  float getAveSinkArrival() const { return aveArrival_; }
  void setAveSinkArrival(float arrival) { aveArrival_ = arrival; }
  float getNDummies() const { return nDummies_; }
  void setNDummies(float nDummies) { nDummies_ = nDummies; }
  odb::dbInst* getTopBuffer() const { return topBuffer_; }
  void setTopBuffer(odb::dbInst* inst) { topBuffer_ = inst; }
  std::string getTopBufferName() const { return topBufferName_; }
  void setTopBufferName(std::string name) { topBufferName_ = std::move(name); }
  odb::dbNet* getTopInputNet() const { return topInputNet_; }
  void setTopInputNet(odb::dbNet* net) { topInputNet_ = net; }
  odb::dbNet* getDrivingNet() const { return drivingNet_; }
  void setDrivingNet(odb::dbNet* net) { drivingNet_ = net; }

 protected:
  CtsOptions* options_ = nullptr;
  Clock clock_;
  TechChar* techChar_ = nullptr;
  TreeBuilder* parent_;
  std::vector<TreeBuilder*> children_;
  // Tree buffer levels. Number of buffers inserted in first leg of the HTree
  // is buffer levels (depth) of tree in all legs.
  // This becomes buffer level for whole tree thus
  unsigned treeBufLevels_ = 0;
  std::set<ClockInst*> first_level_sink_drivers_;
  std::set<ClockInst*> second_level_sink_drivers_;
  std::set<ClockInst*> tree_level_buffers_;
  utl::Logger* logger_;
  odb::dbDatabase* db_;
  std::vector<odb::Rect> blockages_;
  double bufferWidth_ = 0.0;
  double bufferHeight_ = 0.0;
  // keep track of occupied cells to avoid overlap violations
  // this only tracks cell origin
  boost::unordered_set<Point<double>, pointHash, pointEqual> occupiedLocations_;
  // keep track of insertion delays at sink pins
  boost::unordered_map<Point<double>, double, pointHash, pointEqual>
      insertionDelays_;
  TreeType type_ = TreeType::RegularTree;
  float aveArrival_ = 0.0;
  int nDummies_ = 0;
  odb::dbInst* topBuffer_ = nullptr;
  std::string topBufferName_;
  odb::dbNet* drivingNet_ = nullptr;
  odb::dbNet* topInputNet_ = nullptr;
};

}  // namespace cts
