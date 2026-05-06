// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#pragma once

#include "SetupLegacyBase.hh"

namespace rsz {

class SetupDirectionalPolicy : public SetupLegacyBase
{
 public:
  SetupDirectionalPolicy(Resizer& resizer,
                         MoveCommitter& committer,
                         RepairSetupContext& setup_context,
                         const OptimizerRunConfig& config,
                         bool use_starts)
      : SetupLegacyBase(resizer, committer, setup_context, config),
        use_starts_(use_starts)
  {
  }

  const char* name() const override { return "SetupDirectionalPolicy"; }
  void iterate() override;

 private:
  void repairSetupDirectional(bool use_startpoints,
                              float setup_slack_margin,
                              int max_passes_per_point,
                              bool verbose);

  bool use_starts_{false};
};

}  // namespace rsz
