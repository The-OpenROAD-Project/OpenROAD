// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#include "MoveGenerator.hh"

#include <optional>
#include <string>
#include <tuple>

#include "db_sta/dbNetwork.hh"
#include "odb/db.h"
#include "rsz/Resizer.hh"
#include "sta/Delay.hh"
#include "sta/Liberty.hh"
#include "sta/MinMax.hh"
#include "sta/Transition.hh"

namespace rsz {

MoveGenerator::CellCosts MoveGenerator::cellCosts(sta::LibertyCell* cell) const
{
  CellCosts costs;
  const std::optional<float> leakage = resizer_.cellLeakage(cell);
  if (leakage && *leakage > 0.0f) {
    costs[0] = leakage;
  }
  const odb::dbMaster* master = resizer_.dbNetwork()->staToDb(cell);
  if (master != nullptr && master->getArea() > 0) {
    costs[1] = static_cast<float>(master->getArea());
  }
  if (cell->area() > 0.0f) {
    costs[2] = cell->area();
  }
  return costs;
}

std::optional<float> MoveGenerator::cellCostRatio(const CellCosts& from_costs,
                                                  sta::LibertyCell* to) const
{
  const CellCosts to_costs = cellCosts(to);
  for (size_t metric = 0; metric < from_costs.size(); ++metric) {
    if (from_costs[metric] && to_costs[metric]) {
      return *to_costs[metric] / *from_costs[metric];
    }
  }
  return std::nullopt;
}

const sta::LibertyPort* MoveGenerator::findScenePort(
    const sta::LibertyCell* cell,
    const std::string& port_name,
    const int lib_ap_index) const
{
  if (cell == nullptr) {
    return nullptr;
  }

  const sta::LibertyPort* port = cell->findLibertyPort(port_name);
  return port != nullptr ? port->scenePort(lib_ap_index) : nullptr;
}

bool MoveGenerator::weakerCellFirst(const sta::LibertyCell* lhs,
                                    const sta::LibertyCell* rhs,
                                    const std::string& drvr_port_name,
                                    const int lib_ap_index) const
{
  return weakerCellFirst(
      lhs, rhs, drvr_port_name, lib_ap_index, nullptr, sta::MinMax::max());
}

bool MoveGenerator::weakerCellFirst(const sta::LibertyCell* lhs,
                                    const sta::LibertyCell* rhs,
                                    const std::string& drvr_port_name,
                                    const int lib_ap_index,
                                    const sta::RiseFall* drvr_rf,
                                    const sta::MinMax* min_max) const
{
  const sta::LibertyPort* lhs_port
      = findScenePort(lhs, drvr_port_name, lib_ap_index);
  const sta::LibertyPort* rhs_port
      = findScenePort(rhs, drvr_port_name, lib_ap_index);
  if ((lhs_port != nullptr) != (rhs_port != nullptr)) {
    return lhs_port != nullptr;
  }
  if (lhs_port == nullptr) {
    return lhs->name() < rhs->name();
  }

  const float lhs_drive_resistance
      = lhs_port->driveResistance(drvr_rf, min_max);
  const float rhs_drive_resistance
      = rhs_port->driveResistance(drvr_rf, min_max);
  const float lhs_max_drive_resistance = lhs_port->driveResistance();
  const float rhs_max_drive_resistance = rhs_port->driveResistance();
  const sta::ArcDelay lhs_intrinsic
      = lhs_port->intrinsicDelay(drvr_rf, min_max, resizer_.staState());
  const sta::ArcDelay rhs_intrinsic
      = rhs_port->intrinsicDelay(drvr_rf, min_max, resizer_.staState());
  return std::tie(lhs_drive_resistance, lhs_max_drive_resistance, lhs_intrinsic)
         > std::tie(
             rhs_drive_resistance, rhs_max_drive_resistance, rhs_intrinsic);
}

}  // namespace rsz
