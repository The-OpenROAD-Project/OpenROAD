// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#include "RerouteCandidate.hh"

#include <functional>
#include <unordered_set>
#include <utility>

#include "MoveCandidate.hh"
#include "OptimizerTypes.hh"
#include "est/EstimateParasitics.h"
#include "grt/GlobalRouter.h"
#include "odb/db.h"
#include "rsz/Resizer.hh"
#include "sta/Network.hh"
#include "utl/Logger.h"

namespace rsz {

using utl::RSZ;

RerouteCandidate::RerouteCandidate(
    Resizer& resizer,
    const Target& target,
    sta::Pin* driver_pin,
    sta::Instance* driver_inst,
    odb::dbNet* db_net,
    const float current_resistance,
    const float estimated_resistance,
    const int promoted_min_layer,
    std::unordered_set<odb::dbNet*>* attempted_nets,
    std::function<void(odb::dbNet*)> on_applied)
    : MoveCandidate(resizer, target),
      driver_pin_(driver_pin),
      driver_inst_(driver_inst),
      db_net_(db_net),
      current_resistance_(current_resistance),
      estimated_resistance_(estimated_resistance),
      promoted_min_layer_(promoted_min_layer),
      attempted_nets_(attempted_nets),
      on_applied_(std::move(on_applied))
{
}

Estimate RerouteCandidate::estimate()
{
  return {.legal = true, .score = current_resistance_ - estimated_resistance_};
}

MoveResult RerouteCandidate::apply()
{
  grt::GlobalRouter* global_router = resizer_.globalRouter();
  if (global_router == nullptr || global_router->isUseCUGR()
      || promoted_min_layer_ <= 0
      || (attempted_nets_ != nullptr && attempted_nets_->contains(db_net_))) {
    return rejectedMove();
  }

  if (on_applied_) {
    on_applied_(db_net_);
  } else if (attempted_nets_ != nullptr) {
    attempted_nets_->insert(db_net_);
  }

  global_router->setNetPromotedMinLayer(db_net_, promoted_min_layer_);
  global_router->setResistanceAware(true);
  global_router->requestNetReroute(db_net_);
  resizer_.estimateParasitics()->parasiticsInvalid(db_net_);

  debugPrint(resizer_.logger(),
             RSZ,
             "reroute_move",
             1,
             "ACCEPT RerouteMove {}: Rerouted net {} (resistance {} -> {} "
             "estimated, promoted_min_layer={})",
             resizer_.network()->pathName(driver_pin_),
             db_net_->getName(),
             current_resistance_,
             estimated_resistance_,
             promoted_min_layer_);
  return {
      .accepted = true,
      .type = MoveType::kReroute,
      .move_count = 1,
      .touched_instances = {driver_inst_},
  };
}

}  // namespace rsz
