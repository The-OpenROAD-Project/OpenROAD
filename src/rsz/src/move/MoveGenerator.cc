// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#include "MoveGenerator.hh"

#include <string>
#include <tuple>

#include "rsz/Resizer.hh"
#include "sta/Delay.hh"
#include "sta/Liberty.hh"

namespace rsz {

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

bool MoveGenerator::strongerCellFirst(const sta::LibertyCell* lhs,
                                      const sta::LibertyCell* rhs,
                                      const std::string& drvr_port_name,
                                      const int lib_ap_index) const
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

  const float lhs_drive_resistance = lhs_port->driveResistance();
  const float rhs_drive_resistance = rhs_port->driveResistance();
  const sta::ArcDelay lhs_intrinsic
      = lhs_port->intrinsicDelay(resizer_.staState());
  const sta::ArcDelay rhs_intrinsic
      = rhs_port->intrinsicDelay(resizer_.staState());
  return std::tie(lhs_drive_resistance, lhs_intrinsic)
         < std::tie(rhs_drive_resistance, rhs_intrinsic);
}

}  // namespace rsz
