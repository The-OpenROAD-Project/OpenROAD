// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#pragma once

#include "OptPolicy.hh"

namespace rsz {

class SetupLegacyPolicy;

// Phase-OptPolicy wrappers.  Each class adapts one legacy repair phase to the
// OptPolicy lifecycle: a single iterate() runs the entire repair phase and
// then marks the policy converged.  All of them share a `parent_` pointer
// back to the host SetupLegacyPolicy because the legacy helpers and config
// fields they delegate to currently live on that class.

// Common base for all phase wrappers.  Holds the parent pointer; concrete
// classes only override iterate().
class PhasePolicyBase : public OptPolicy
{
 public:
  PhasePolicyBase(Resizer& resizer,
                  MoveCommitter& committer,
                  SetupLegacyPolicy* parent);

 protected:
  SetupLegacyPolicy* parent_;
};

// LEGACY token  -  runs initializeMainRepair + runMainRepairLoop.
class MainRepairPhasePolicy : public PhasePolicyBase
{
 public:
  using PhasePolicyBase::PhasePolicyBase;
  const char* name() const override { return "MainRepairPhasePolicy"; }
  void iterate() override;
};

// WNS / WNS_PATH / WNS_CONE tokens  -  runs repairSetupWns.  `use_cone`
// distinguishes WNS_CONE from WNS / WNS_PATH.
class WnsPhasePolicy : public PhasePolicyBase
{
 public:
  WnsPhasePolicy(Resizer& resizer,
                 MoveCommitter& committer,
                 SetupLegacyPolicy* parent,
                 bool use_cone);
  const char* name() const override { return "WnsPhasePolicy"; }
  void iterate() override;

 private:
  bool use_cone_{false};
};

// TNS token  -  runs repairSetupTns.
class TnsPhasePolicy : public PhasePolicyBase
{
 public:
  using PhasePolicyBase::PhasePolicyBase;
  const char* name() const override { return "TnsPhasePolicy"; }
  void iterate() override;
};

// ENDPOINT_FANIN / STARTPOINT_FANOUT tokens  -  runs repairSetupDirectional.
// `use_starts` selects startpoint-driven (true) vs endpoint-driven (false).
class DirectionalPhasePolicy : public PhasePolicyBase
{
 public:
  DirectionalPhasePolicy(Resizer& resizer,
                         MoveCommitter& committer,
                         SetupLegacyPolicy* parent,
                         bool use_starts);
  const char* name() const override { return "DirectionalPhasePolicy"; }
  void iterate() override;

 private:
  bool use_starts_{false};
};

// LAST_GASP token  -  runs repairSetupLastGasp.  Honors skip_last_gasp.
class LastGaspPhasePolicy : public PhasePolicyBase
{
 public:
  using PhasePolicyBase::PhasePolicyBase;
  const char* name() const override { return "LastGaspPhasePolicy"; }
  void iterate() override;
};

// CRIT_VT_SWAP token  -  runs the legacy critical fanin-cone VT sweep.
class CritVtSwapPhasePolicy : public PhasePolicyBase
{
 public:
  using PhasePolicyBase::PhasePolicyBase;
  const char* name() const override { return "CritVtSwapPhasePolicy"; }
  void iterate() override;
};

}  // namespace rsz
