// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#pragma once

#include "grt/GRoute.h"

namespace est {

// Service interface published by the parasitics estimator. Consumers
// look it up through utl::ServiceRegistry and do not depend on the
// concrete est::EstimateParasitics type.
class ParasiticsService
{
 public:
  virtual ~ParasiticsService() = default;

  // Clear all existing parasitics and re-estimate them from the
  // current global routing state (partial routes). With threads > 1 the
  // nets are estimated in parallel; the result is identical to one
  // thread. Only a caller that knows nothing else reads or writes
  // parasitics or delays meanwhile may pass threads > 1.
  virtual void estimateAllGlobalRouteParasitics(int threads = 1) = 0;

  // Re-estimate one net's parasitics from the given route; no-op if empty.
  virtual void updateGlobalRouteParasitics(odb::dbNet* net, grt::GRoute& route)
      = 0;
};

}  // namespace est
