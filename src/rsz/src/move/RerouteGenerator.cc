// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#include "RerouteGenerator.hh"

#include <memory>
#include <vector>

#include "MoveGenerator.hh"
#include "OptimizerTypes.hh"
#include "RerouteCandidate.hh"
#include "db_sta/dbSta.hh"
#include "grt/GlobalRouter.h"
#include "odb/db.h"
#include "rsz/Resizer.hh"
#include "sta/Network.hh"
#include "utl/Logger.h"

namespace rsz {

using utl::RSZ;

namespace {

constexpr float kMinResistanceReduction = 0.40f;

}  // namespace

RerouteGenerator::RerouteGenerator(const GeneratorContext& context)
    : MoveGenerator(context)
{
}

bool RerouteGenerator::isApplicable(const Target& target) const
{
  grt::GlobalRouter* global_router = resizer_.globalRouter();
  return MoveGenerator::isApplicable(target) && global_router != nullptr
         && global_router->haveRoutes();
}

std::vector<std::unique_ptr<MoveCandidate>> RerouteGenerator::generate(
    const Target& target)
{
  std::vector<std::unique_ptr<MoveCandidate>> candidates;
  if (!isApplicable(target)) {
    return candidates;
  }

  sta::Pin* driver_pin = target.resolvedPin(resizer_);
  sta::Instance* driver_inst = target.inst(resizer_);
  if (driver_pin == nullptr) {
    debugPrint(resizer_.logger(),
               RSZ,
               "reroute_move",
               2,
               "REJECT RerouteMove: No driver pin");
    return candidates;
  }

  if (resizer_.dontTouch(driver_inst)) {
    debugPrint(resizer_.logger(),
               RSZ,
               "reroute_move",
               2,
               "REJECT RerouteMove {}: {} is \"don't touch\"",
               resizer_.network()->pathName(driver_pin),
               resizer_.network()->pathName(driver_inst));
    return candidates;
  }

  sta::Net* net = resizer_.network()->net(driver_pin);
  if (net == nullptr) {
    debugPrint(resizer_.logger(),
               RSZ,
               "reroute_move",
               2,
               "REJECT RerouteMove {}: No net found for driver pin",
               resizer_.network()->pathName(driver_pin));
    return candidates;
  }

  odb::dbNet* db_net = resizer_.dbNetwork()->staToDb(net);
  if (db_net == nullptr) {
    debugPrint(resizer_.logger(),
               RSZ,
               "reroute_move",
               2,
               "REJECT RerouteMove {}: No db net found",
               resizer_.network()->pathName(driver_pin));
    return candidates;
  }

  if (db_net->isSpecial()) {
    debugPrint(resizer_.logger(),
               RSZ,
               "reroute_move",
               2,
               "REJECT RerouteMove {}: Net is special",
               resizer_.network()->pathName(driver_pin));
    return candidates;
  }

  grt::GlobalRouter* global_router = resizer_.globalRouter();
  if (global_router->isNetResAware(db_net)) {
    debugPrint(resizer_.logger(),
               RSZ,
               "reroute_move",
               2,
               "REJECT RerouteMove {}: Net is already resistance-aware routed",
               resizer_.network()->pathName(driver_pin));
    return candidates;
  }

  const float resistance = global_router->getFRNetResistance(db_net);
  const float estimated_resistance
      = global_router->getFRNetResistanceOnMinClockLayer(db_net);
  const float reduction_ratio
      = (resistance > 0.0f) ? (resistance - estimated_resistance) / resistance
                            : 0.0f;
  if (reduction_ratio < kMinResistanceReduction) {
    debugPrint(resizer_.logger(),
               RSZ,
               "reroute_move",
               2,
               "REJECT RerouteMove {}: Expected resistance reduction {:.1f}% "
               "below threshold {:.1f}% ({} -> {} estimated)",
               resizer_.network()->pathName(driver_pin),
               100.0f * reduction_ratio,
               100.0f * kMinResistanceReduction,
               resistance,
               estimated_resistance);
    return candidates;
  }

  candidates.push_back(
      std::make_unique<RerouteCandidate>(resizer_,
                                         target,
                                         driver_pin,
                                         driver_inst,
                                         db_net,
                                         resistance,
                                         estimated_resistance));
  return candidates;
}

}  // namespace rsz
