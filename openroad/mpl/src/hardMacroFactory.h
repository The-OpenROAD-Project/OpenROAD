// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <string>
#include <utility>

#include "mpl-util.h"
#include "object.h"
#include "odb/db.h"
#include "odb/geom.h"

namespace utl {
class Logger;
}

namespace mpl {

class HardMacroFactory
{
 public:
  HardMacroFactory(odb::dbBlock* block,
                   utl::Logger* logger,
                   const odb::Rect& floorplan_shape,
                   Channel min_channel,
                   bool pin_aware_channels);

  InstToHardMap create() const;

 private:
  HardMacro::Halo buildHalo(odb::dbInst* inst, int minimum_spacing) const;
  HardMacro::Halo buildPinAwareHalo(odb::dbInst* inst,
                                    int minimum_spacing) const;
  int getMinimumSpacing() const;
  bool isEquidistantDifferentDirections(
      std::pair<int, Boundary> candidate,
      std::pair<int, Boundary> second_candidate) const;
  std::string generateMacroAndCoreDimensionsTable(const HardMacro* hard_macro,
                                                  const odb::Rect& core) const;

  odb::dbBlock* block_;
  utl::Logger* logger_;
  odb::Rect floorplan_shape_;
  Channel min_channel_;
  bool pin_aware_channels_{false};
};

}  // namespace mpl
