// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#pragma once

#include <functional>
#include <unordered_set>

#include "MoveCandidate.hh"
#include "OptimizerTypes.hh"
#include "rsz/Resizer.hh"

namespace odb {
class dbNet;
}  // namespace odb

namespace sta {
class Instance;
class Pin;
}  // namespace sta

namespace rsz {

// Marks one driver net for resistance-aware incremental global reroute,
// optionally promoting its minimum routing layer onto lower-resistance layers.
class RerouteCandidate : public MoveCandidate
{
 public:
  RerouteCandidate(Resizer& resizer,
                   const Target& target,
                   sta::Pin* driver_pin,
                   sta::Instance* driver_inst,
                   odb::dbNet* db_net,
                   float current_resistance,
                   float estimated_resistance,
                   int promoted_min_layer = 0,
                   std::unordered_set<odb::dbNet*>* attempted_nets = nullptr,
                   std::function<void(odb::dbNet*)> on_applied = {});

  Estimate estimate() override;
  MoveResult apply() override;
  MoveType type() const override { return MoveType::kReroute; }

 private:
  sta::Pin* driver_pin_;
  sta::Instance* driver_inst_;
  odb::dbNet* db_net_;
  float current_resistance_;
  float estimated_resistance_;
  int promoted_min_layer_;
  std::unordered_set<odb::dbNet*>* attempted_nets_;
  std::function<void(odb::dbNet*)> on_applied_;
};

}  // namespace rsz
