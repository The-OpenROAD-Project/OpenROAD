// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "feasibilityChecker.h"

#include <cstdint>
#include <string>
#include <vector>

#include "boost/polygon/polygon.hpp"
#include "boost/polygon/polygon_90_set_data.hpp"
#include "fmt/format.h"
#include "object.h"
#include "odb/db.h"
#include "odb/geom.h"
#include "odb/geom_boost.h"  // IWYU pragma: keep (polygon traits for odb::Rect)
#include "utl/Logger.h"

namespace mpl {

using utl::MPL;

FeasibilityChecker::FeasibilityChecker(odb::dbBlock* block,
                                       utl::Logger* logger,
                                       const InstToHardMap& hard_macros,
                                       const odb::Rect& floorplan_shape,
                                       const float target_utilization)
    : block_(block),
      logger_(logger),
      hard_macros_(hard_macros),
      floorplan_shape_(floorplan_shape),
      target_utilization_(target_utilization)
{
}

void FeasibilityChecker::run()
{
  checkUnfixedMacros();

  // Nothing to place.
  if (!found_unfixed_macros_) {
    return;
  }

  checkTargetUtilization();
  checkMacrosFitInCore();
  checkMovableCellsFit();
  checkVirtualUtilization();
}

void FeasibilityChecker::checkUnfixedMacros()
{
  for (const auto& [inst, hard_macro] : hard_macros_) {
    if (!hard_macro->isFixed()) {
      found_unfixed_macros_ = true;
      return;
    }
  }
}

void FeasibilityChecker::checkTargetUtilization() const
{
  if (target_utilization_ <= 0.0f || target_utilization_ > 1.0f) {
    logger_->error(MPL,
                   14,
                   "The target utilization {:.2f} is outside (0, 1].",
                   target_utilization_);
  }
}

void FeasibilityChecker::checkMacrosFitInCore() const
{
  const odb::Rect& core = block_->getCoreArea();
  for (const auto& [inst, hard_macro] : hard_macros_) {
    if (hard_macro->isIgnorable()) {
      continue;
    }

    if (hard_macro->getWidth() > core.dx()
        || hard_macro->getHeight() > core.dy()) {
      logger_->error(MPL,
                     6,
                     "Found macro that does not fit in the core.\nName: {}\n{}",
                     inst->getName(),
                     macroAndCoreDimensionsTable(hard_macro.get(), core));
    }
  }
}

std::string FeasibilityChecker::macroAndCoreDimensionsTable(
    const HardMacro* hard_macro,
    const odb::Rect& core) const
{
  std::string table;

  table += fmt::format("\n          |   Macro + Halos   |   Core   ");
  table += fmt::format("\n-----------------------------------------");
  table += fmt::format("\n   Width  | {:>17.2f} | {:>8.2f}",
                       block_->dbuToMicrons(hard_macro->getWidth()),
                       block_->dbuToMicrons(core.dx()));
  table += fmt::format("\n  Height  | {:>17.2f} | {:>8.2f}\n",
                       block_->dbuToMicrons(hard_macro->getHeight()),
                       block_->dbuToMicrons(core.dy()));

  return table;
}

void FeasibilityChecker::checkMovableCellsFit() const
{
  int64_t occupied_area = 0;
  for (odb::dbInst* inst : block_->getInsts()) {
    const odb::Rect bbox = inst->getBBox()->getBox();

    if (inst->isFixed()) {
      // Note that we can handle fixed cells outside the macro placement area.
      // Also, it may exist macros such as physical markers that can be
      // partially inside the core so we need to compute the intersection.
      odb::Rect intersection;
      bbox.intersection(floorplan_shape_, intersection);
      occupied_area += intersection.area();
      continue;
    }

    if (inst->isBlock()) {
      occupied_area += hard_macros_.at(inst)->getArea();
    } else {
      occupied_area += bbox.area();
    }
  }

  occupied_area += blockagesArea();

  if (occupied_area > floorplan_shape_.area()) {
    logger_->error(
        MPL, 65, "The movable cells do not fit in the macro placement area.");
  }
}

int64_t FeasibilityChecker::blockagesArea() const
{
  namespace gtl = boost::polygon;
  using gtl::operators::operator+=;
  using PolygonSet = gtl::polygon_90_set_data<int>;

  PolygonSet polygons;
  for (odb::dbBlockage* blockage : block_->getBlockages()) {
    const odb::Rect bbox = blockage->getBBox()->getBox();
    odb::Rect intersection;
    bbox.intersection(floorplan_shape_, intersection);
    polygons += intersection;
  }

  std::vector<odb::Rect> blockages_without_overlap;
  polygons.get_rectangles(blockages_without_overlap);

  int64_t area = 0;
  for (const odb::Rect& blockage : blockages_without_overlap) {
    area += blockage.area();
  }
  return area;
}

// Mirrors the check the root cluster placement makes before each run, with
// the macros' halo areas standing in for their tilings.
void FeasibilityChecker::checkVirtualUtilization() const
{
  const int64_t std_cell_area = stdCellArea();
  const int64_t hard_area
      = unfixedMacrosArea() + fixedMacrosArea() + blockagesArea();
  const auto inflated_std_cell_area = static_cast<int64_t>(
      std_cell_area / static_cast<double>(target_utilization_));
  const int64_t floorplan_area = floorplan_shape_.area();
  const double virtual_utilization = (hard_area + inflated_std_cell_area)
                                     / static_cast<double>(floorplan_area);

  if (hard_area + inflated_std_cell_area >= floorplan_area) {
    logger_->error(MPL,
                   7,
                   "The virtual utilization is {:.2f}: the std cells at the "
                   "target utilization {:.2f} ({:.2f}um^2) plus the macros "
                   "with halos, the fixed macros and the blockages "
                   "({:.2f}um^2) exceed the floorplan area ({:.2f}um^2).",
                   virtual_utilization,
                   target_utilization_,
                   block_->dbuAreaToMicrons(inflated_std_cell_area),
                   block_->dbuAreaToMicrons(hard_area),
                   block_->dbuAreaToMicrons(floorplan_area));
  }

  // Without std cells there is no density to adjust.
  if (std_cell_area > 0 && virtual_utilization > kVirtualUtilWarnThreshold) {
    logger_->warn(MPL,
                  9,
                  "The virtual utilization {:.2f} is above {:.1f}. The "
                  "annealer will probably struggle to find a valid result and "
                  "may increase the std cell density to converge.",
                  virtual_utilization,
                  kVirtualUtilWarnThreshold);
  }
}

int64_t FeasibilityChecker::stdCellArea() const
{
  int64_t area = 0;
  for (odb::dbInst* inst : block_->getInsts()) {
    odb::dbMaster* master = inst->getMaster();
    if (inst->isBlock() || master->isPad() || master->isCover()
        || master->isEndCap()) {
      continue;
    }

    area += inst->getBBox()->getBox().area();
  }
  return area;
}

int64_t FeasibilityChecker::unfixedMacrosArea() const
{
  int64_t area = 0;
  for (const auto& [inst, hard_macro] : hard_macros_) {
    if (!hard_macro->isFixed()) {
      area += hard_macro->getArea();
    }
  }
  return area;
}

int64_t FeasibilityChecker::fixedMacrosArea() const
{
  int64_t area = 0;
  for (const auto& [inst, hard_macro] : hard_macros_) {
    if (!hard_macro->isFixed() || hard_macro->isIgnorable()) {
      continue;
    }

    odb::Rect intersection;
    hard_macro->getBBox().intersection(floorplan_shape_, intersection);
    area += intersection.area();
  }
  return area;
}

}  // namespace mpl
