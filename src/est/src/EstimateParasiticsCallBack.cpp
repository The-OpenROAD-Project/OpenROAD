// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024-2025, The OpenROAD Authors

#include "EstimateParasiticsCallBack.h"

#include "est/EstimateParasitics.h"
#include "grt/GlobalRouter.h"

namespace est {

void EstimateParasiticsCallBack::onEstimateParasiticsRequired()
{
  estimate_parasitics_->clearParasitics();
  auto routes = estimate_parasitics_->getGlobalRouter()->getPartialRoutes();
  for (auto& [db_net, route] : routes) {
    estimate_parasitics_->estimateGlobalRouteParasitics(db_net, route);
  }
}

}  // namespace est