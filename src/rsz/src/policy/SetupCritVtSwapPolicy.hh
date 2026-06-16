// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#pragma once

#include <unordered_map>
#include <unordered_set>

#include "SetupLegacyBase.hh"
#include "sta/Delay.hh"
#include "sta/Graph.hh"
#include "sta/NetworkClass.hh"

namespace rsz {

class SetupCritVtSwapPolicy : public SetupLegacyBase
{
 public:
  using SetupLegacyBase::SetupLegacyBase;

  const char* name() const override { return "SetupCritVtSwapPolicy"; }
  void iterate() override;

 private:
  bool swapVTCritCells(int& num_viols);
  sta::Pin* outputPin(sta::Instance* inst);
  void traverseFaninCone(sta::Vertex* endpoint,
                         std::unordered_map<sta::Instance*, float>& crit_insts,
                         std::unordered_set<sta::Vertex*>& visited,
                         std::unordered_set<sta::Instance*>& notSwappable);
  sta::Slack getInstanceSlack(sta::Instance* inst);
};

}  // namespace rsz
