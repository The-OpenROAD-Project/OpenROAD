// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#pragma once

#include <vector>

namespace odb {
class dbITerm;
}

namespace drt {

// Service interface published by the detailed router. Consumers look
// it up through utl::ServiceRegistry and do not depend on the concrete
// drt::TritonRoute type.
class PinAccessService
{
 public:
  virtual ~PinAccessService() = default;

  // A full pin access or recovery pass must have completed for the live block.
  virtual bool hasPinAccess() const = 0;
  virtual bool isPinAccessRequired(odb::dbITerm* term) const = 0;

  // Recompute pin access for any instances that have become dirty.
  virtual void updateDirtyPinAccess() = 0;

  // Recovery invalidates readiness until successful publication.
  virtual void beginPinAccessRecovery(int min_access_points = 1) = 0;
  // Required terminals below the recovery minimum; requires an active session.
  virtual std::vector<odb::dbITerm*> getMissingPinAccess() const = 0;
  virtual void updatePinAccessRecovery() = 0;
  virtual void finishPinAccessRecovery() = 0;
  // Cancellation or generation failure leaves pin access unready.
  virtual void cancelPinAccessRecovery() = 0;
};

}  // namespace drt
