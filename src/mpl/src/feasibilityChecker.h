// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <cstdint>
#include <string>

#include "object.h"
#include "odb/db.h"
#include "odb/geom.h"

namespace utl {
class Logger;
}

namespace mpl {

class FeasibilityChecker
{
 public:
  FeasibilityChecker(odb::dbBlock* block,
                     utl::Logger* logger,
                     const InstToHardMap& hard_macros,
                     const odb::Rect& floorplan_shape,
                     float target_utilization);

  void run();

  bool foundMacrosToPlace() const { return found_macros_to_place_; }

 private:
  void checkUnfixedMacros();

  bool validTargetUtilization() const;

  void checkMacrosFitInCore();
  std::string macroAndCoreDimensionsTable(const HardMacro* hard_macro,
                                          const odb::Rect& core) const;

  void checkMovableCellsFit();
  int64_t blockagesArea() const;

  void checkVirtualUtilization();
  int64_t stdCellArea() const;
  int64_t unfixedMacrosArea() const;
  int64_t fixedMacrosArea() const;

  // The annealer may struggle to find a valid result above this.
  static constexpr float kVirtualUtilWarnThreshold = 0.9f;

  odb::dbBlock* block_;
  utl::Logger* logger_;

  const InstToHardMap& hard_macros_;
  odb::Rect floorplan_shape_;
  float target_utilization_{0.0f};

  bool found_macros_to_place_{false};
  bool feasible_{true};
};

}  // namespace mpl
