// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025, The OpenROAD Authors

#include "RerouteMove.hh"

#include "db_sta/dbSta.hh"
#include "grt/GlobalRouter.h"
#include "odb/db.h"
#include "rsz/Resizer.hh"
#include "sta/Path.hh"
#include "utl/Logger.h"

namespace rsz {

using utl::RSZ;

bool RerouteMove::doMove(const sta::Path* drvr_path, float setup_slack_margin)
{
  sta::Pin* drvr_pin = drvr_path->pin(this);
  if (!drvr_pin) {
    debugPrint(
        logger_, RSZ, "reroute_move", 2, "REJECT RerouteMove: No driver pin");
    return false;
  }

  sta::Instance* drvr_inst = network_->instance(drvr_pin);
  if (resizer_->dontTouch(drvr_inst)) {
    debugPrint(logger_,
               RSZ,
               "reroute_move",
               2,
               "REJECT RerouteMove {}: {} is \"don't touch\"",
               network_->pathName(drvr_pin),
               network_->pathName(drvr_inst));
    return false;
  }

  sta::Net* net = network_->net(drvr_pin);
  if (!net) {
    debugPrint(logger_,
               RSZ,
               "reroute_move",
               2,
               "REJECT RerouteMove {}: No net found for driver pin",
               network_->pathName(drvr_pin));
    return false;
  }

  odb::dbNet* db_net = db_network_->staToDb(net);
  if (!db_net) {
    debugPrint(logger_,
               RSZ,
               "reroute_move",
               2,
               "REJECT RerouteMove {}: No db net found",
               network_->pathName(drvr_pin));
    return false;
  }

  if (db_net->isSpecial()) {
    debugPrint(logger_,
               RSZ,
               "reroute_move",
               2,
               "REJECT RerouteMove {}: Net is special",
               network_->pathName(drvr_pin));
    return false;
  }

  // Check if we should even try to reroute
  if (resizer_->global_router_->isNetResAware(db_net)) {
    debugPrint(logger_,
               RSZ,
               "reroute_move",
               2,
               "REJECT RerouteMove {}: Net is already resistance-aware routed",
               network_->pathName(drvr_pin));
    return false;
  }

  // Fast pre-filter: estimate resistance on the minimum clock layer using the
  // existing Steiner tree topology, avoiding an expensive incremental reroute.
  // Reject the move if the expected resistance reduction is below 40%.
  float resistance = resizer_->global_router_->getFRNetResistance(db_net);
  float estimated_resistance
      = resizer_->global_router_->getFRNetResistanceOnMinClockLayer(db_net);

  constexpr float kMinResistanceReduction = 0.40f;
  const float reduction_ratio
      = (resistance > 0.0f) ? (resistance - estimated_resistance) / resistance
                            : 0.0f;
  if (reduction_ratio < kMinResistanceReduction) {
    debugPrint(logger_,
               RSZ,
               "reroute_move",
               2,
               "REJECT RerouteMove {}: Expected resistance reduction {:.1f}% "
               "below threshold {:.1f}% ({} -> {} estimated)",
               network_->pathName(drvr_pin),
               100.0f * reduction_ratio,
               100.0f * kMinResistanceReduction,
               resistance,
               estimated_resistance);
    return false;
  }

  resizer_->global_router_->setResistanceAware(true);
  resizer_->global_router_->addDirtyNet(db_net);
  resizer_->global_router_->setNetIsResAware(db_net, true);
  estimate_parasitics_->parasiticsInvalid(db_net);

  debugPrint(logger_,
             RSZ,
             "reroute_move",
             1,
             "ACCEPT RerouteMove {}: Rerouted net {} (resistance {} -> {} "
             "estimated)",
             network_->pathName(drvr_pin),
             db_net->getName(),
             resistance,
             estimated_resistance);
  countMove(drvr_inst);
  return true;
}

}  // namespace rsz
