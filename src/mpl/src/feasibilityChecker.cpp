// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "feasibilityChecker.h"

#include <cstdint>
#include <string>
#include <utility>
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

  if (!found_macros_to_place_) {
    return;
  }

  checkMacrosFitInCore();
  checkMovableCellsFit();

  // Inflating the std cell area needs a valid target utilization.
  if (validTargetUtilization()) {
    checkVirtualUtilization();
  } else {
    logger_->warn(MPL,
                  14,
                  "The target utilization {:.2f} is outside (0, 1].",
                  target_utilization_);

    feasible_ = false;
  }

  if (!feasible_) {
    logger_->error(MPL, 48, "The macro placement problem is infeasible.");
  }
}

void FeasibilityChecker::checkUnfixedMacros()
{
  for (const auto& [inst, hard_macro] : hard_macros_) {
    if (!hard_macro->isFixed()) {
      found_macros_to_place_ = true;
      return;
    }
  }
}

bool FeasibilityChecker::validTargetUtilization() const
{
  return target_utilization_ > 0.0f && target_utilization_ <= 1.0f;
}

void FeasibilityChecker::checkMacrosFitInCore()
{
  const odb::Rect& core = block_->getCoreArea();
  for (const auto& [inst, hard_macro] : hard_macros_) {
    if (hard_macro->isIgnorable()) {
      continue;
    }

    if (hard_macro->getWidth() > core.dx()
        || hard_macro->getHeight() > core.dy()) {
      logger_->warn(MPL,
                    6,
                    "Found macro that does not fit in the core.\nName: {}\n{}",
                    inst->getName(),
                    macroAndCoreDimensionsTable(hard_macro.get(), core));

      feasible_ = false;
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

void FeasibilityChecker::checkMovableCellsFit()
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
    logger_->warn(
        MPL, 65, "The movable cells do not fit in the macro placement area.");

    feasible_ = false;
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
void FeasibilityChecker::checkVirtualUtilization()
{
  const int64_t std_cell_area = stdCellArea();
  const auto inflated_std_cell_area = static_cast<int64_t>(
      std_cell_area / static_cast<double>(target_utilization_));
  const int64_t unfixed_macros_area = unfixedMacrosArea();
  const int64_t fixed_macros_area = fixedMacrosArea();
  const int64_t blockages_area = blockagesArea();
  const int64_t total_area = inflated_std_cell_area + unfixed_macros_area
                             + fixed_macros_area + blockages_area;
  const int64_t floorplan_area = floorplan_shape_.area();
  const double virtual_utilization
      = total_area / static_cast<double>(floorplan_area);

  if (total_area >= floorplan_area) {
    const std::string table = areasTable(
        {{fmt::format("Std cells at utilization {:.2f}", target_utilization_),
          inflated_std_cell_area},
         {"Macros with halos", unfixed_macros_area},
         {"Fixed macros", fixed_macros_area},
         {"Blockages", blockages_area},
         {"Total", total_area},
         {"Floorplan", floorplan_area}});
    logger_->warn(MPL,
                  7,
                  "The virtual utilization is {:.2f}: the total area exceeds "
                  "the floorplan area.\n{}",
                  virtual_utilization,
                  table);

    feasible_ = false;
  } else if (std_cell_area > 0
             && virtual_utilization > kVirtualUtilWarnThreshold) {
    // Without std cells there is no density to adjust.
    logger_->warn(MPL,
                  9,
                  "The virtual utilization {:.2f} is above {:.1f}. The "
                  "annealer will probably struggle to find a valid result and "
                  "may increase the std cell density to converge.",
                  virtual_utilization,
                  kVirtualUtilWarnThreshold);
  }
}

std::string FeasibilityChecker::areasTable(
    const std::vector<std::pair<std::string, int64_t>>& rows) const
{
  std::string table;

  table += fmt::format("\n  {:<30} | {:>12}", "", "Area (um^2)");
  table += fmt::format("\n-----------------------------------------------");
  for (const auto& [label, area] : rows) {
    table += fmt::format(
        "\n  {:<30} | {:>12.2f}", label, block_->dbuAreaToMicrons(area));
  }
  table += "\n";

  return table;
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
