// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// WirelengthGradientBackend — Strategy interface for the WA wirelength
// gradient (force + per-cell gradient). CpuWirelengthGradientBackend wraps
// the existing OpenMP loops in NesterovBaseCommon; GpuWirelengthGradientBackend
// runs a Kokkos kernel pipeline against the device pool in DeviceState.
//
// Header is plain C++ (no Kokkos, no preprocessor) so nesterovBase.h can hold
// a std::unique_ptr<WirelengthGradientBackend> member.
//
// Phase 2 of the gpl GPU porting — see plan in
// /home/mjkim/.claude/plans/parsed-sprouting-cookie.md.

#pragma once

#include <memory>
#include <vector>

#include "point.h"

namespace gpl {

class NesterovBaseCommon;
class DeviceState;
class GCell;
class GCellHandle;

class WirelengthGradientBackend
{
 public:
  virtual ~WirelengthGradientBackend() = default;

  // Refresh per-pin / per-net WA exponentials (CPU: clearWaVars + the OMP loop
  // in updateWireLengthForceWA; GPU: K1 updateNetBBox, K2 computeAPosNeg,
  // K3 computeBC, K4 computePinWAGrad). After this call, getCellGradient(s)
  // is valid for the same (wlCoefX, wlCoefY).
  virtual void updateForce(float wlCoefX, float wlCoefY) = 0;

  // Bulk gather of per-cell wirelength gradient into `out`, indexed parallel
  // to `gCells` (= nb_gcells_ in the NesterovBase caller — may be a subset
  // of nbc_gcells_ for the multi-region case). Caller pre-sizes `out` to
  // gCells.size(). Hot path of NesterovBase::updateGradients().
  virtual void getCellGradients(const std::vector<GCellHandle>& gCells,
                                std::vector<FloatPoint>& out)
      = 0;

  // Per-cell gradient (cold path: NesterovBase::updateSingleGradient via the
  // db-callback hook). Backend may cache prior bulk results.
  virtual FloatPoint getCellGradient(const GCell* gCell) = 0;

  virtual const char* name() const = 0;
};

// Factory: GpuWirelengthGradientBackend on ENABLE_GPU + gpuEnabled(), else
// CpuWirelengthGradientBackend. `nbc` is the owning common base — both
// backends call back into it for CPU helpers / data access. `device_state`
// may be null for the CPU path.
std::unique_ptr<WirelengthGradientBackend> makeWirelengthGradientBackend(
    int num_threads,
    NesterovBaseCommon* nbc,
    DeviceState* device_state);

}  // namespace gpl
