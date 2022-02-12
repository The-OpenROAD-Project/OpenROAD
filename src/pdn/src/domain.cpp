//////////////////////////////////////////////////////////////////////////////
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

#include "domain.h"

#include "grid.h"
#include "odb/db.h"
#include "utl/Logger.h"

namespace pdn {

VoltageDomain::VoltageDomain(const std::string& name,
                             odb::dbBlock* block,
                             odb::dbNet* power,
                             odb::dbNet* ground,
                             const std::vector<odb::dbNet*>& secondary_nets,
                             odb::dbRegion* region,
                             utl::Logger* logger)
    : name_(name),
      block_(block),
      power_(power),
      ground_(ground),
      secondary_(secondary_nets),
      region_(region),
      logger_(logger)
{
  if (hasRegion()) {
    const int rect_count = getRegionRectCount(region_);
    if (rect_count == 0) {
      logger_->error(
          utl::PDN, 103, "{} region must have a shape.", region_->getName());
    } else if (rect_count > 1) {
      logger_->error(utl::PDN,
                     104,
                     "{} region contains {} shapes, but only one is supported.",
                     region_->getName(),
                     rect_count);
    }
  }
  determinePowerGroundNets();
}

std::vector<odb::dbNet*> VoltageDomain::getNets(bool start_with_power) const
{
  std::vector<odb::dbNet*> nets;
  if (start_with_power) {
    nets.push_back(power_);
  }
  nets.push_back(ground_);
  if (!start_with_power) {
    nets.push_back(power_);
  }
  nets.insert(nets.end(), secondary_.begin(), secondary_.end());
  return nets;
}

void VoltageDomain::addGrid(std::unique_ptr<Grid> grid)
{
  grids_.push_back(std::move(grid));
}

void VoltageDomain::resetGrids()
{
  for (const auto& grid : grids_) {
    grid->resetShapes();
  }
}

const std::vector<odb::dbRow*> VoltageDomain::getRows() const
{
  if (hasRegion()) {
    return getRegionRows();
  } else {
    return getCoreRows();
  }
}

const odb::Rect VoltageDomain::getCoreArea() const
{
  if (hasRegion()) {
    return getRegionBoundary(region_);
  } else {
    odb::Rect core;
    block_->getCoreArea(core);
    return core;
  }
}

int VoltageDomain::getRegionRectCount(odb::dbRegion* region) const
{
  if (region == nullptr) {
    return 0;
  }

  auto boundaries = region->getBoundaries();
  if (boundaries.empty()) {
    return getRegionRectCount(region->getParent());
  } else {
    return boundaries.size();
  }
}

const odb::Rect VoltageDomain::getRegionBoundary(odb::dbRegion* region) const
{
  if (region == nullptr) {
    return {};
  }

  odb::Rect region_box;

  for (auto* box : region->getBoundaries()) {
    odb::Rect box_rect;
    box->getBox(box_rect);
    return box_rect;
  }

  return getRegionBoundary(region->getParent());
}

const std::vector<odb::dbRow*> VoltageDomain::getRegionRows() const
{
  std::vector<odb::dbRow*> rows;

  const odb::Rect region = getRegionBoundary(region_);

  for (auto* row : block_->getRows()) {
    odb::Rect row_bbox;
    row->getBBox(row_bbox);

    if (row_bbox.overlaps(region)) {
      rows.push_back(row);
    }
  }

  return rows;
}

const std::vector<odb::dbRow*> VoltageDomain::getCoreRows() const
{
  std::vector<odb::dbRow*> rows;

  std::set<odb::Rect> regions;
  for (auto* region : block_->getRegions()) {
    regions.insert(getRegionBoundary(region));
  }

  for (auto* row : block_->getRows()) {
    if (regions.empty()) {
      rows.push_back(row);
    } else {
      odb::Rect row_bbox;
      row->getBBox(row_bbox);
      bool belongs_to_region = false;
      for (const auto& region : regions) {
        if (row_bbox.overlaps(region)) {
          belongs_to_region = true;
          break;
        }
      }
      if (!belongs_to_region) {
        rows.push_back(row);
      }
    }
  }

  return rows;
}

void VoltageDomain::report() const
{
  logger_->info(utl::PDN, 10, "Voltage domain: {}", name_);

  if (region_ != nullptr) {
    logger_->info(utl::PDN, 11, "  Floorplan region: {}", region_->getName());
  }

  logger_->info(utl::PDN, 12, "  Power net: {}", power_->getName());
  logger_->info(utl::PDN, 13, "  Ground net: {}", ground_->getName());

  if (!secondary_.empty()) {
    std::string nets;
    for (auto* net : secondary_) {
      nets += net->getName() + " ";
    }
    logger_->info(utl::PDN, 14, "  Secondary nets: {}", nets);
  }

  for (const auto& grid : grids_) {
    grid->report();
  }
}

void VoltageDomain::determinePowerGroundNets()
{
  auto find_net = [this](odb::dbSigType type) -> odb::dbNet* {
    for (auto* net : block_->getNets()) {
      if (net->getSigType() == type) {
        return net;
      }
    }
    logger_->error(utl::PDN,
                   100,
                   "Unable to find {} net for {} domain.",
                   type.getString(),
                   name_);
  };

  // look for power
  if (power_ == nullptr) {
    power_ = find_net(odb::dbSigType::POWER);
    logger_->warn(utl::PDN,
                  101,
                  "Using {} as power net for {} domain.",
                  power_->getName(),
                  name_);
  }
  // look for ground
  if (ground_ == nullptr) {
    ground_ = find_net(odb::dbSigType::GROUND);
    logger_->warn(utl::PDN,
                  102,
                  "Using {} as ground net for {} domain.",
                  ground_->getName(),
                  name_);
  }
}

//////////

CoreVoltageDomain::CoreVoltageDomain(
    odb::dbBlock* block,
    odb::dbNet* power,
    odb::dbNet* ground,
    const std::vector<odb::dbNet*>& secondary_nets,
    utl::Logger* logger)
    : VoltageDomain("Core",
                    block,
                    power,
                    ground,
                    secondary_nets,
                    nullptr,
                    logger)
{
}

}  // namespace pdn
