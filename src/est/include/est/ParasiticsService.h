// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#pragma once

namespace est {

// Service interface published by the parasitics estimator. Consumers
// look it up through utl::ServiceRegistry and do not depend on the
// concrete est::EstimateParasitics type.
class ParasiticsService
{
 public:
  virtual ~ParasiticsService() = default;

  // Clear all existing parasitics and re-estimate them from the
  // current global routing state (partial routes).
  virtual void estimateAllGlobalRouteParasitics() = 0;
};

}  // namespace est
