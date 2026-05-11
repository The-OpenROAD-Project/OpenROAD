// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#pragma once

#include "SetupLegacyBase.hh"

namespace rsz {

class SetupWnsPolicy : public SetupLegacyBase
{
 public:
  SetupWnsPolicy(Resizer& resizer,
                 MoveCommitter& committer,
                 RepairSetupContext& setup_context,
                 const OptimizerRunConfig& config,
                 bool use_cone)
      : SetupLegacyBase(resizer, committer, setup_context, config),
        use_cone_(use_cone)
  {
  }

  const char* name() const override { return "SetupWnsPolicy"; }
  void iterate() override;

 private:
  void repairSetupWns(float setup_slack_margin,
                      int max_passes_per_endpoint,
                      int max_repairs_per_pass,
                      bool verbose,
                      bool use_cone_collection,
                      rsz::ViolatorSortType sort_type);

  bool use_cone_{false};
};

}  // namespace rsz
