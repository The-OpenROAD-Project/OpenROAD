// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#pragma once

#include "SetupLegacyPolicy.hh"
#include "sta/Delay.hh"
#include "sta/Path.hh"

namespace rsz {

// REROUTE phase: try resistance-aware incremental global reroute on the
// largest wire-delay nets along violating setup paths.
class SetupReroutePolicy : public SetupLegacyPolicy
{
 public:
  using SetupLegacyPolicy::SetupLegacyPolicy;

  const char* name() const override { return "SetupReroutePolicy"; }

 protected:
  void buildMainMoveSequence(bool log_sequence) override;
  bool repairPath(sta::Path* path,
                  sta::Slack path_slack,
                  bool force_single_repair) override;
  const char* phaseName() const override { return "REROUTE"; }
  const char* phaseSummaryTitle() const override
  {
    return "REROUTE Phase Summary";
  }
  const char* phaseEndpointProfilerTitle() const override
  {
    return "REROUTE Phase Endpoint Profiler";
  }
};

}  // namespace rsz
