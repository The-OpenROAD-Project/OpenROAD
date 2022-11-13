///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2022, The Regents of the University of California
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

#pragma once

#include <array>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "odb/dbTypes.h"
#include "shape.h"

namespace odb {
class dbMaster;
class dbMTerm;
class dbNet;
class dbInst;
class Rect;
}  // namespace odb

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

  const std::string getName() const;

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
  enum NetworkType
  {
    STAR,
    DAISY
  };

  GridSwitchedPower(Grid* grid,
                    PowerCell* cell,
                    odb::dbNet* control,
                    NetworkType network);

  void setGrid(Grid* grid) { grid_ = grid; }

  void report() const;

  static const std::string toString(NetworkType type);
  static NetworkType fromString(const std::string& type, utl::Logger* logger);

  void build();
  void ripup();

  const ShapeTreeMap getShapes() const;

 private:
  Grid* grid_;
  PowerCell* cell_;
  odb::dbNet* control_;
  NetworkType network_;

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

  using InstValue = std::pair<Box, odb::dbInst*>;
  using InstTree = bgi::rtree<InstValue, bgi::quadratic<16>>;
  InstTree buildInstanceSearchTree() const;
  odb::dbInst* checkOverlappingInst(odb::dbInst* cell,
                                    const InstTree& insts) const;
  void checkAndFixOverlappingInsts(const InstTree& insts);

  ShapeTree buildStrapTargetList(Straps* target) const;

  using RowValue = std::pair<Box, odb::dbRow*>;
  using RowTree = bgi::rtree<RowValue, bgi::quadratic<16>>;
  RowTree buildRowTree() const;
  std::set<odb::dbRow*> getInstanceRows(odb::dbInst* inst,
                                        const RowTree& rows) const;

  bool checkInstanceOverlap(odb::dbInst* inst0, odb::dbInst* inst1) const;
  void updateControlNetwork();
  void updateControlNetworkSTAR();
  void updateControlNetworkDAISY(const bool order_by_x);

  static constexpr const char* inst_prefix_ = "PSW_";
};

}  // namespace pdn
