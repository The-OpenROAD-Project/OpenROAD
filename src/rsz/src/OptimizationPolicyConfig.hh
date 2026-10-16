// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#pragma once

namespace rsz {

// Every tunable of the setup repair policies and their moves.  The default
// member initializer is the shipped value.

// Knobs shared by the policies and their moves.
struct OptimizationPolicyConfig
{
  // --- Delay estimator ------------------------------------------------------

  // Stages scored before and after the target stage; zero scores only the
  // target stage.
  int delay_estimation_levels{1};
  // Correct the table slew of the fanin driver stage with STA slews sampled
  // at three loads.  The accuracy report uses the same setting.
  bool sta_slew_bias{true};
};

}  // namespace rsz
