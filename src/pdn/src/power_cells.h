// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

#pragma once

#include <array>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "odb/db.h"
#include "odb/dbTypes.h"
#include "pdn/PdnGen.hh"
#include "shape.h"

namespace utl {
class Logger;
}

namespace pdn {
class Grid;
class Straps;

class PowerCell
{
 public:
  PowerCell(utl::Logger* logger,
            odb::dbMaster* master,
            odb::dbMTerm* control,
            odb::dbMTerm* acknowledge,
            odb::dbMTerm* switched_power,
            odb::dbMTerm* alwayson_power,
            odb::dbMTerm* ground);

  std::string getName() const;

  utl::Logger* getLogger() const { return logger_; }

  odb::dbMaster* getMaster() const { return master_; }
  odb::dbMTerm* getControlPin() const { return control_; }
  odb::dbMTerm* getAcknowledgePin() const { return acknowledge_; }
  odb::dbMTerm* getSwitchedPowerPin() const { return switched_power_; }
  odb::dbMTerm* getAlwaysOnPowerPin() const { return alwayson_power_; }
  odb::dbMTerm* getGroundPin() const { return ground_; }

  bool hasAcknowledge() const { return acknowledge_ != nullptr; }

  bool appliesToRow(odb::dbRow* row) const;

  // returns the site positions that overlap the power pin
  const std::set<int>& getAlwaysOnPowerPinPositions() const
  {
    return alwayson_power_positions_;
  }

  void report() const;

  void populateAlwaysOnPinPositions(int site_width);
  static std::set<int> getRectAsSiteWidths(const odb::Rect& rect,
                                           int site_width,
                                           int offset);

 private:
  utl::Logger* logger_;
  odb::dbMaster* master_;
  odb::dbMTerm* control_;
  odb::dbMTerm* acknowledge_;
  odb::dbMTerm* switched_power_;
  odb::dbMTerm* alwayson_power_;
  odb::dbMTerm* ground_;

  std::set<int> alwayson_power_positions_;
};

class GridSwitchedPower
{
 public:
  GridSwitchedPower(Grid* grid,
                    PowerCell* cell,
                    odb::dbNet* control,
                    PowerSwitchNetworkType network);

  void setGrid(Grid* grid) { grid_ = grid; }

  void report() const;

  static std::string toString(PowerSwitchNetworkType type);
  static PowerSwitchNetworkType fromString(const std::string& type,
                                           utl::Logger* logger);

  void build();
  void ripup();

  Shape::ShapeTreeMap getShapes() const;

 private:
  Grid* grid_;
  PowerCell* cell_;
  odb::dbNet* control_;
  PowerSwitchNetworkType network_;

  // instances with additional information;
  struct InstanceInfo
  {
    std::set<int> sites;
    std::set<odb::dbRow*> rows;
  };
  std::map<odb::dbInst*, InstanceInfo> insts_;

  Straps* getLowestStrap() const;

  std::set<int> computeLocations(const odb::Rect& strap,
                                 int site_width,
                                 const odb::Rect& corearea) const;

  struct InstIndexableGetter
  {
    using result_type = odb::Rect;
    odb::Rect operator()(odb::dbInst* inst) const
    {
      return inst->getBBox()->getBox();
    }
  };
  using InstTree
      = bgi::rtree<odb::dbInst*, bgi::quadratic<16>, InstIndexableGetter>;
  InstTree buildInstanceSearchTree() const;
  odb::dbInst* checkOverlappingInst(odb::dbInst* cell,
                                    const InstTree& insts) const;
  void checkAndFixOverlappingInsts(const InstTree& insts);

  Shape::ShapeTree buildStrapTargetList(Straps* target) const;

  struct RowIndexableGetter
  {
    using result_type = odb::Rect;
    odb::Rect operator()(odb::dbRow* row) const { return row->getBBox(); }
  };
  using RowTree
      = bgi::rtree<odb::dbRow*, bgi::quadratic<16>, RowIndexableGetter>;
  RowTree buildRowTree() const;
  std::set<odb::dbRow*> getInstanceRows(odb::dbInst* inst,
                                        const RowTree& row_search) const;

  bool checkInstanceOverlap(odb::dbInst* inst0, odb::dbInst* inst1) const;
  void updateControlNetwork();
  void updateControlNetworkSTAR();
  void updateControlNetworkDAISY(bool order_by_x);

  static constexpr const char* inst_prefix_ = "PSW_";
};

}  // namespace pdn
