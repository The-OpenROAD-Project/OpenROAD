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

#include <memory>
#include <string>
#include <vector>

#include "odb/dbTypes.h"

namespace odb {
class dbBlock;
class dbNet;
class dbRegion;
class dbRow;
class Rect;
}  // namespace odb

namespace utl {
class Logger;
}

namespace pdn {
class Grid;
class PdnGen;

class VoltageDomain
{
 public:
  VoltageDomain(PdnGen* pdngen,
                const std::string& name,
                odb::dbBlock* block,
                odb::dbNet* power,
                odb::dbNet* ground,
                const std::vector<odb::dbNet*>& secondary_nets,
                odb::dbRegion* region,
                utl::Logger* logger);

  VoltageDomain(PdnGen* pdngen,
                odb::dbBlock* block,
                odb::dbNet* power,
                odb::dbNet* ground,
                const std::vector<odb::dbNet*>& secondary_nets,
                utl::Logger* logger);  // Core

  const std::string& getName() const { return name_; }

  odb::dbBlock* getBlock() const { return block_; }
  utl::Logger* getLogger() const { return logger_; }
  PdnGen* getPDNGen() const { return pdngen_; }

  odb::dbNet* getPower() const;
  odb::dbNet* getGround() const { return ground_; }
  odb::dbNet* getAlwaysOnPower() const { return power_; }
  odb::dbNet* getSwitchedPower() const { return switched_power_; }

  void setSwitchedPower(odb::dbNet* switched_power)
  {
    switched_power_ = switched_power;
  }
  bool hasSwitchedPower() const { return switched_power_ != nullptr; }

  // returns the order in which the nets should be arranged in the grid shapes
  std::vector<odb::dbNet*> getNets(bool start_with_power = true) const;

  bool hasRegion() const { return region_ != nullptr; }
  odb::dbRegion* getRegion() const { return region_; }

  // returns the area of the region or core
  odb::Rect getDomainArea() const;

  void addGrid(std::unique_ptr<Grid> grid);
  void resetGrids();
  void clearGrids() { grids_.clear(); }
  void removeGrid(Grid* grid);
  const std::vector<std::unique_ptr<Grid>>& getGrids() const { return grids_; }

  // get the rows associated with the core or region
  std::vector<odb::dbRow*> getRows() const;

  void report() const;

  void checkSetup() const;

 private:
  std::string name_;
  PdnGen* pdngen_;
  odb::dbBlock* block_;
  odb::dbNet* power_;
  odb::dbNet* switched_power_;
  odb::dbNet* ground_;
  std::vector<odb::dbNet*> secondary_;

  odb::dbRegion* region_;

  utl::Logger* logger_;

  std::vector<std::unique_ptr<Grid>> grids_;

  int getRegionRectCount(odb::dbRegion* region) const;
  odb::Rect getRegionBoundary(odb::dbRegion* region) const;
  // returns just the rows associated with the region
  std::vector<odb::dbRow*> getRegionRows() const;
  // returns only rows that are not associated with a region
  std::vector<odb::dbRow*> getDomainRows() const;

  // find power and ground nets if they are not specified
  void determinePowerGroundNets();

  odb::dbNet* findDomainNet(const odb::dbSigType& type) const;
};

}  // namespace pdn
