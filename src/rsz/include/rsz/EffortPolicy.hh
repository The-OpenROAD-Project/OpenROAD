// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <string>

namespace rsz {

// Coarse optimization policy for repair_timing, analogous to compiler
// driver -O flags (POLA): the caller states intent, the tool owns the
// mapping to internal heuristics. Directives apply left to right; a later
// directive overrides an earlier one, as in "gcc -O0 -O2".
//
// Defined directives:
//   tapeout  historical behavior: unbounded effort toward the timing
//            targets, hold repaired (default).
//   explore  design-space exploration: setup repair may stop as soon as
//            its marginal progress plateaus, hold repair is not run at
//            all (hold closure is period-independent and adds no
//            exploration information, only runtime).
//   -hold    position marker for an explicit repair_timing -hold flag:
//            to the right of "explore" it re-enables hold repair.
//
// Graded effort levels between the two are intentionally not defined here.
struct EffortPolicy
{
  // Setup repair's marginal-progress stop may fire once the optimization
  // iteration exceeds plateau_start_iteration and the incremental TNS fix
  // rate drops below min_inc_fix_rate. The defaults reproduce the
  // historical hardcoded 1000 / 0.0001 gate.
  int plateau_start_iteration = 1000;
  float min_inc_fix_rate = 0.0001f;
  bool repair_hold = true;

  bool operator==(const EffortPolicy&) const = default;

  // Returns false on an unknown directive.
  bool apply(const std::string& directive)
  {
    if (directive == "tapeout") {
      *this = EffortPolicy();
      return true;
    }
    if (directive == "explore") {
      plateau_start_iteration = 0;
      min_inc_fix_rate = 0.05f;
      repair_hold = false;
      return true;
    }
    if (directive == "-hold") {
      repair_hold = true;
      return true;
    }
    return false;
  }
};

}  // namespace rsz
