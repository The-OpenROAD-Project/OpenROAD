// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#pragma once

namespace drt {

// Service interface published by the detailed router. Consumers look
// it up through utl::ServiceRegistry and do not depend on the concrete
// drt::TritonRoute type.
class PinAccessService
{
 public:
  virtual ~PinAccessService() = default;

  // Recompute pin access for any instances that have become dirty.
  virtual void updateDirtyPinAccess() = 0;
};

}  // namespace drt
