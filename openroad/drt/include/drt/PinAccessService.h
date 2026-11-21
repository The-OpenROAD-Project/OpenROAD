// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#pragma once

#include <vector>

namespace odb {
class dbInst;
class dbITerm;
class Point;
}  // namespace odb

namespace drt {

// Service interface published by the detailed router. Consumers look
// it up through utl::ServiceRegistry and do not depend on the concrete
// drt::TritonRoute type.
class PinAccessService
{
 public:
  virtual ~PinAccessService() = default;

  // Successful full pin access is available for the current live block.
  virtual bool hasPinAccess() const = 0;
  virtual bool hasMinimumPinAccess(int min_access_points) const = 0;
  // Check required pins at the candidate lower-left without changing state.
  // Tests native PA escapes and signal-pin supply overlap, not full placement
  // DRC.
  virtual bool hasUsablePinAccess(odb::dbInst* inst,
                                  const odb::Point& candidate_location) const
      = 0;
  // Return failures at current lower-left locations, preserving input order.
  virtual std::vector<odb::dbInst*> findPinAccessFailures(
      const std::vector<odb::dbInst*>& instances) const = 0;

  // Recovery keeps partial native catalogues private until finish publishes
  // them.
  virtual std::vector<odb::dbITerm*> beginPinAccessRecovery(
      int min_access_points) = 0;
  virtual std::vector<odb::dbITerm*> updatePinAccessRecovery() = 0;
  virtual void finishPinAccessRecovery() = 0;
  virtual void cancelPinAccessRecovery() noexcept = 0;

  // Recompute pin access for any instances that have become dirty.
  virtual void updateDirtyPinAccess() = 0;
};

}  // namespace drt
