// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#pragma once

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

// Marks one driver net for resistance-aware incremental global reroute.
class RerouteCandidate : public MoveCandidate
{
 public:
  RerouteCandidate(Resizer& resizer,
                   const Target& target,
                   sta::Pin* driver_pin,
                   sta::Instance* driver_inst,
                   odb::dbNet* db_net,
                   float current_resistance,
                   float estimated_resistance);

  Estimate estimate() override;
  MoveResult apply() override;
  MoveType type() const override { return MoveType::kReroute; }

 private:
  sta::Pin* driver_pin_;
  sta::Instance* driver_inst_;
  odb::dbNet* db_net_;
  float current_resistance_;
  float estimated_resistance_;
};

}  // namespace rsz
