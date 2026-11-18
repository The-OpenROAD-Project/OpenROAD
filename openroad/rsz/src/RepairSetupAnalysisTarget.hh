// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#pragma once

#include <optional>

#include "OptimizerTypes.hh"

namespace rsz {

class Resizer;

// Refreshes a serial repair target for non-intrusive mode-3 observations.
// The policy continues to use the original target for its decisions.
class RepairSetupAnalysisTarget
{
 public:
  RepairSetupAnalysisTarget(Resizer& resizer, const Target& source);
  // Rebuild the observed target from current STA after prior ECOs.
  bool refresh();
  // Return the refreshed observation target, or the source before refresh.
  const Target& target() const
  {
    return observed_target_ ? *observed_target_ : source_;
  }
  // Return the source path endpoint when a refreshed target is available.
  const sta::Pin* originalEndpoint() const
  {
    return observed_target_ ? source_.endpoint_pin : nullptr;
  }

 private:
  Resizer& resizer_;
  // Policy-owned target whose decisions are unchanged by observation.
  const Target& source_;
  // Refreshed copy consumed only by the analysis observer.
  std::optional<Target> observed_target_;
};

}  // namespace rsz
