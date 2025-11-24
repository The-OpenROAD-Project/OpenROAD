// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2025, The OpenROAD Authors

#pragma once

#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "boost/geometry/index/parameters.hpp"
#include "boost/geometry/index/rtree.hpp"
#include "gui/gui.h"
#include "odb/db.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"
#include "odb/geom_boost.h"
#include "odb/isotropy.h"

namespace odb {
class dbBlock;
class dbITerm;
class dbTechLayer;
class dbTechVia;
class dbNet;
}  // namespace odb

namespace utl {
class Logger;
}

namespace pad {

class PadPlacer
{
 public:
  PadPlacer(utl::Logger* logger,
            odb::dbBlock* block,
            const std::vector<odb::dbInst*>& insts,
            const odb::Direction2D::Value& edge,
            odb::dbRow* row);
  virtual ~PadPlacer() = default;

  virtual void place() = 0;

  utl::Logger* getLogger() const { return logger_; }
  odb::dbBlock* getBlock() const { return block_; }
  odb::dbRow* getRow() const { return row_; }
  const odb::Direction2D::Value& getRowEdge() const { return edge_; }
  bool isRowHorizontal() const
  {
    return row_->getDirection() == odb::dbRowDir::HORIZONTAL;
  }
  const std::vector<odb::dbInst*>& getInsts() const { return insts_; }

  int getRowStart() const;
  int getRowWidth() const;
  int getRowEnd() const;
  int getTotalInstWidths() const;

 protected:
  using InstObsValue = std::pair<odb::Rect, odb::dbInst*>;
  using TermObsValue = std::tuple<odb::Rect, odb::dbNet*, odb::dbInst*>;
  using BlockageObsTree
      = boost::geometry::index::rtree<odb::Rect,
                                      boost::geometry::index::quadratic<16>>;
  using InstObsTree
      = boost::geometry::index::rtree<InstObsValue,
                                      boost::geometry::index::quadratic<16>>;
  using TermObsTree
      = boost::geometry::index::rtree<TermObsValue,
                                      boost::geometry::index::quadratic<16>>;
  using LayerTermObsTree = std::map<odb::dbTechLayer*, TermObsTree>;

  int placeInstance(int index,
                    odb::dbInst* inst,
                    const odb::dbOrientType& base_orient,
                    bool allow_overlap = false,
                    bool allow_shift = false,
                    bool check = true) const;
  void populateObstructions();
  std::optional<std::pair<odb::dbInst*, odb::Rect>> checkInstancePlacement(
      odb::dbInst* inst,
      bool return_intersect = true) const;
  int snapToRowSite(int location) const;
  int convertRowIndexToPos(int index) const;
  const std::map<odb::dbInst*, int>& getInstWidths() const
  {
    return inst_widths_;
  }
  LayerTermObsTree getInstanceObstructions(odb::dbInst* inst,
                                           bool bloat = false) const;
  void addInstanceObstructions(odb::dbInst* inst);

 private:
  void populateInstWidths();

  utl::Logger* logger_;
  odb::dbBlock* block_;
  std::vector<odb::dbInst*> insts_;
  odb::Direction2D::Value edge_;
  odb::dbRow* row_;

  // Computed values
  std::map<odb::dbInst*, int> inst_widths_;

  // Fixed obstructions
  BlockageObsTree blockage_obstructions_;
  InstObsTree instance_obstructions_;
  LayerTermObsTree term_obstructions_;
};

class CheckerOnlyPadPlacer : public PadPlacer
{
 public:
  CheckerOnlyPadPlacer(utl::Logger* logger,
                       odb::dbBlock* block,
                       odb::dbRow* row);
  ~CheckerOnlyPadPlacer() override = default;

  void place() override {};

  bool check(odb::dbInst* inst) const;
};

class SingleInstPadPlacer : public PadPlacer
{
 public:
  SingleInstPadPlacer(utl::Logger* logger,
                      odb::dbBlock* block,
                      const odb::Direction2D::Value& edge,
                      odb::dbRow* row);
  ~SingleInstPadPlacer() override = default;

  int snapToRowSite(int location) const
  {
    return PadPlacer::snapToRowSite(location);
  }

  void place() override {};
  void place(odb::dbInst* inst,
             int location,
             const odb::dbOrientType& base_orient,
             bool allow_overlap = false);
};

class UniformPadPlacer : public PadPlacer
{
 public:
  UniformPadPlacer(utl::Logger* logger,
                   odb::dbBlock* block,
                   const std::vector<odb::dbInst*>& insts,
                   const odb::Direction2D::Value& edge,
                   odb::dbRow* row,
                   std::optional<int> max_spacing = {});
  ~UniformPadPlacer() override = default;

  void place() override;

 private:
  std::optional<int> max_spacing_;
};

class BumpAlignedPadPlacer : public PadPlacer
{
 public:
  BumpAlignedPadPlacer(utl::Logger* logger,
                       odb::dbBlock* block,
                       const std::vector<odb::dbInst*>& insts,
                       const odb::Direction2D::Value& edge,
                       odb::dbRow* row);
  ~BumpAlignedPadPlacer() override = default;

  void place() override;

  void setConnections(
      const std::map<odb::dbInst*, std::set<odb::dbITerm*>>& iterm_connections)
  {
    iterm_connections_ = iterm_connections;
  }

 private:
  int64_t estimateWirelengths(odb::dbInst* inst,
                              const std::set<odb::dbITerm*>& iterms) const;
  int64_t computePadBumpDistance(odb::dbInst* inst,
                                 int inst_width,
                                 odb::dbITerm* bump,
                                 int center_pos) const;
  std::map<odb::dbInst*, odb::dbITerm*> getBumpAlignmentGroup(
      int offset,
      const std::vector<odb::dbInst*>::const_iterator& itr,
      const std::vector<odb::dbInst*>::const_iterator& inst_end) const;
  void performPadFlip(odb::dbInst* inst) const;

  std::map<odb::dbInst*, std::set<odb::dbITerm*>> iterm_connections_;
};

class PlacerPadPlacer : public PadPlacer
{
 public:
  PlacerPadPlacer(utl::Logger* logger,
                  odb::dbBlock* block,
                  const std::vector<odb::dbInst*>& insts,
                  const odb::Direction2D::Value& edge,
                  odb::dbRow* row);
  ~PlacerPadPlacer() override = default;

  void place() override;

  void setConnections(
      const std::map<odb::dbInst*, std::set<odb::dbITerm*>>& iterm_connections)
  {
    iterm_connections_ = iterm_connections;
  }

 private:
  struct InstAnchors
  {
    int min;
    int center;
    int max;

    int width;

    void setLocation(int pos)
    {
      const int half = width / 2;
      min = pos;
      center = min + half;
      max = min + width;
    }

    int overlap(InstAnchors* other) const;
    int overlap(const std::unique_ptr<InstAnchors>& other) const;
  };

  void computeIdealPostions();
  std::map<odb::dbInst*, int> initialPoolMapping() const;
  std::map<odb::dbInst*, int> poolAdjacentViolators(
      const std::map<odb::dbInst*, int>& initial_positions) const;
  std::map<odb::dbInst*, int> padSpreading(
      const std::map<odb::dbInst*, int>& initial_positions) const;
  bool padSpreading(
      std::map<odb::dbInst*, std::unique_ptr<InstAnchors>>& positions,
      const std::map<odb::dbInst*, int>& initial_positions,
      int itr,
      float spring,
      float repel,
      float damper) const;

  void placeInstanceSimple(odb::dbInst* inst,
                           int position,
                           bool center_ref) const;
  void placeInstances(const std::map<odb::dbInst*, int>& positions,
                      bool center_ref) const;
  void placeInstances(
      const std::map<odb::dbInst*, std::unique_ptr<InstAnchors>>& positions)
      const;
  int64_t estimateWirelengths() const;
  int getNumberOfRoutes() const;
  void debugPause(const std::string& msg) const;
  void addChartData(int itr, int64_t rdl_length, int move) const;
  int getNearestLegalPosition(odb::dbInst* inst,
                              int target,
                              bool round_down = false,
                              bool round_up = false) const;
  int getRowStart(odb::dbInst* inst) const;
  int getRowEnd(odb::dbInst* inst) const;
  std::pair<int, int> getTunnelingPosition(odb::dbInst* inst,
                                           int target,
                                           bool move_up,
                                           int low_bound,
                                           int curr_pos,
                                           int high_bound,
                                           int itr) const;
  void debugCheckPlacement() const;
  std::string instNameList(const std::vector<odb::dbInst*>& insts) const;

  std::map<odb::dbInst*, std::set<odb::dbITerm*>> iterm_connections_;
  std::map<odb::dbInst*, int> ideal_positions_;

  // debug
  gui::Chart* chart_{nullptr};

  // constants
  static constexpr int kMaxIterations = 5000;
  static constexpr float kSpringStart = 0.1;
  static constexpr float kSpringEnd = 0.0;
  // First iteration where spring force starts to change
  static constexpr int kSpringIterInfluence = 0.2 * kMaxIterations;
  // Last iteration where spring force is considered
  static constexpr int kSpringIterEnd = 0.5 * kMaxIterations;
  static constexpr int kSpringItrRange = kSpringIterEnd - kSpringIterInfluence;
  static constexpr float kRepelStart = 0.5;
  static constexpr float kRepelEnd = 0.5;
  static constexpr float kDamper = 0.2;
};

}  // namespace pad
