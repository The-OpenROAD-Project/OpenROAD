// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#pragma once

#include "RepairTargetCollector.hh"
#include "SetupLegacyBase.hh"

namespace rsz {

class SetupTnsPolicy : public SetupLegacyBase
{
 public:
  using SetupLegacyBase::SetupLegacyBase;

  const char* name() const override { return "SetupTnsPolicy"; }
  void iterate() override;

 private:
  void repairSetupTns(float setup_slack_margin,
                      int max_passes_per_endpoint,
                      int max_repairs_per_pass,
                      bool verbose,
                      rsz::ViolatorSortType sort_type);
};

}  // namespace rsz
