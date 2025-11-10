// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2025, The OpenROAD Authors

#pragma once

#include <cstdint>
#include <map>
#include <optional>
#include <set>
#include <utility>
#include <vector>

#include "odb/db.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"
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
  const std::vector<odb::dbInst*>& getInsts() const { return insts_; }

  int getRowStart() const;
  int getRowWidth() const;
  int getRowEnd() const;
  int getTotalInstWidths() const;

 protected:
  int placeInstance(int index,
                    odb::dbInst* inst,
                    const odb::dbOrientType& base_orient,
                    bool allow_overlap = false,
                    bool allow_shift = false) const;
  std::optional<std::pair<odb::dbInst*, odb::Rect>> checkInstancePlacement(
      odb::dbInst* inst) const;
  int snapToRowSite(int location) const;
  const std::map<odb::dbInst*, int>& getInstWidths() const
  {
    return inst_widths_;
  }

 private:
  void populateInstWidths();

  utl::Logger* logger_;
  odb::dbBlock* block_;
  std::vector<odb::dbInst*> insts_;
  odb::Direction2D::Value edge_;
  odb::dbRow* row_;

  // Computed values
  std::map<odb::dbInst*, int> inst_widths_;
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
             bool allow_overlap = false) const;
};

class UniformPadPlacer : public PadPlacer
{
 public:
  UniformPadPlacer(utl::Logger* logger,
                   odb::dbBlock* block,
                   const std::vector<odb::dbInst*>& insts,
                   const odb::Direction2D::Value& edge,
                   odb::dbRow* row);
  ~UniformPadPlacer() override = default;

  void place() override;
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

}  // namespace pad
