// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

#include "domain.h"

#include <algorithm>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "grid.h"
#include "odb/db.h"
#include "odb/dbTypes.h"
#include "pdn/PdnGen.hh"
#include "utl/Logger.h"

namespace pdn {

VoltageDomain::VoltageDomain(PdnGen* pdngen,
                             odb::dbBlock* block,
                             odb::dbNet* power,
                             odb::dbNet* ground,
                             const std::vector<odb::dbNet*>& secondary_power,
                             const std::vector<odb::dbNet*>& secondary_ground,
                             utl::Logger* logger)
    : name_("Core"),
      pdngen_(pdngen),
      block_(block),
      power_(power),
      switched_power_(nullptr),
      ground_(ground),
      secondary_power_(secondary_power),
      secondary_ground_(secondary_ground),
      region_(nullptr),
      logger_(logger)
{
  determinePowerGroundNets();
}

VoltageDomain::VoltageDomain(PdnGen* pdngen,
                             const std::string& name,
                             odb::dbBlock* block,
                             odb::dbNet* power,
                             odb::dbNet* ground,
                             const std::vector<odb::dbNet*>& secondary_power,
                             const std::vector<odb::dbNet*>& secondary_ground,
                             odb::dbRegion* region,
                             utl::Logger* logger)
    : name_(name),
      pdngen_(pdngen),
      block_(block),
      power_(power),
      switched_power_(nullptr),
      ground_(ground),
      secondary_power_(secondary_power),
      secondary_ground_(secondary_ground),
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
}

std::vector<odb::dbNet*> VoltageDomain::getNets(bool start_with_power) const
{
  std::vector<odb::dbNet*> nets;

  if (start_with_power) {
    // Order: primary power, switched power, secondary power nets,
    //        secondary ground nets, primary ground.
    // This groups all supply rails together before the ground rails,
    // fixing the ordering issue for multi-rail designs (e.g., VDDA, VDD, VSS).
    nets.push_back(power_);
    if (switched_power_ != nullptr) {
      nets.push_back(switched_power_);
    }
    nets.insert(nets.end(), secondary_power_.begin(), secondary_power_.end());
    nets.insert(
        nets.end(), secondary_ground_.begin(), secondary_ground_.end());
    nets.push_back(ground_);
  } else {
    // Reverse order: primary ground, secondary ground nets,
    //                secondary power nets, switched power, primary power.
    nets.push_back(ground_);
    nets.insert(
        nets.end(), secondary_ground_.begin(), secondary_ground_.end());
    nets.insert(nets.end(), secondary_power_.begin(), secondary_power_.end());
    if (switched_power_ != nullptr) {
      nets.push_back(switched_power_);
    }
    nets.push_back(power_);
  }

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

std::vector<odb::dbRow*> VoltageDomain::getRows() const
{
  if (hasRegion()) {
    return getRegionRows();
  }
  return getDomainRows();
}

odb::Rect VoltageDomain::getDomainArea() const
{
  if (hasRegion()) {
    return getRegionBoundary(region_);
  }
  return block_->getCoreArea();
}

int VoltageDomain::getRegionRectCount(odb::dbRegion* region) const
{
  if (region == nullptr) {
    return 0;
  }

  auto boundaries = region->getBoundaries();
  return boundaries.size();
}

odb::Rect VoltageDomain::getRegionBoundary(odb::dbRegion* region) const
{
  if (region == nullptr) {
    return {};
  }

  auto boundaries = region->getBoundaries();
  if (!boundaries.empty()) {
    odb::Rect box_rect = boundaries.begin()->getBox();
    return box_rect;
  }
  return {};
}

std::vector<odb::dbRow*> VoltageDomain::getRegionRows() const
{
  std::vector<odb::dbRow*> rows;

  const odb::Rect region = getRegionBoundary(region_);

  for (auto* row : block_->getRows()) {
    if (row->getSite()->getClass() == odb::dbSiteClass::PAD) {
      continue;
    }
    odb::Rect row_bbox = row->getBBox();

    if (row_bbox.overlaps(region)) {
      rows.push_back(row);
    }
  }

  return rows;
}

std::vector<odb::dbRow*> VoltageDomain::getDomainRows() const
{
  std::set<odb::dbRow*> claimed_rows;
  for (auto* domain : pdngen_->getDomains()) {
    if (domain == this) {
      continue;
    }

    auto rows = domain->getRows();
    claimed_rows.insert(rows.begin(), rows.end());
  }
  std::vector<odb::dbRow*> rows;

  std::set<odb::Rect> regions;
  for (auto* region : block_->getRegions()) {
    regions.insert(getRegionBoundary(region));
  }

  for (auto* row : block_->getRows()) {
    if (row->getSite()->getClass() == odb::dbSiteClass::PAD) {
      continue;
    }
    if (claimed_rows.find(row) == claimed_rows.end()) {
      rows.push_back(row);
    }
  }

  return rows;
}

void VoltageDomain::report() const
{
  logger_->report("Voltage domain: {}", name_);

  if (region_ != nullptr) {
    logger_->report("  Floorplan region: {}", region_->getName());
  }

  logger_->report("  Power net: {}", power_->getName());
  logger_->report("  Ground net: {}", ground_->getName());

  if (switched_power_ != nullptr) {
    logger_->report("  Switched power net: {}", switched_power_->getName());
  }

  if (!secondary_power_.empty()) {
    std::string nets;
    for (auto* net : secondary_power_) {
      nets += net->getName() + " ";
    }
    logger_->report("  Secondary power nets: {}", nets);
  }

  if (!secondary_ground_.empty()) {
    std::string nets;
    for (auto* net : secondary_ground_) {
      nets += net->getName() + " ";
    }
    logger_->report("  Secondary ground nets: {}", nets);
  }

  for (const auto& grid : grids_) {
    grid->report();
  }
}

odb::dbNet* VoltageDomain::findDomainNet(const odb::dbSigType& type) const
{
  std::set<odb::dbNet*> nets;

  for (auto* net : block_->getNets()) {
    if (net->getSigType() == type) {
      nets.insert(net);
    }
  }

  if (nets.empty()) {
    logger_->error(utl::PDN,
                   100,
                   "Unable to find {} net for {} domain.",
                   type.getString(),
                   name_);
  }
  if (nets.size() > 1) {
    logger_->error(utl::PDN,
                   181,
                   "Found multiple possible nets for {} net for {} domain.",
                   type.getString(),
                   name_);
  }

  return *nets.begin();
}

void VoltageDomain::determinePowerGroundNets()
{
  // look for power
  if (power_ == nullptr) {
    power_ = findDomainNet(odb::dbSigType::POWER);
    logger_->info(utl::PDN,
                  101,
                  "Using {} as power net for {} domain.",
                  power_->getName(),
                  name_);
  }
  // look for ground
  if (ground_ == nullptr) {
    ground_ = findDomainNet(odb::dbSigType::GROUND);
    logger_->info(utl::PDN,
                  102,
                  "Using {} as ground net for {} domain.",
                  ground_->getName(),
                  name_);
  }
}

void VoltageDomain::removeGrid(Grid* grid)
{
  std::erase_if(grids_,
                [grid](const auto& other) { return other.get() == grid; });
}

void VoltageDomain::checkSetup() const
{
  for (const auto& grid : grids_) {
    grid->checkSetup();
  }
}

odb::dbNet* VoltageDomain::getPower() const
{
  if (switched_power_ != nullptr) {
    return switched_power_;
  }
  return power_;
}

}  // namespace pdn
