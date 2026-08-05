// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// WirelengthGradientBackend — Strategy interface for the WA wirelength
// gradient (force + per-cell gradient). CpuWirelengthGradientBackend wraps
// the existing OpenMP loops in NesterovBaseCommon; GpuWirelengthGradientBackend
// runs a Kokkos kernel pipeline against the device pool in DeviceState.
//
// Header is plain C++ (no Kokkos, no preprocessor) so nesterovBase.h can hold
// a std::unique_ptr<WirelengthGradientBackend> member.

#pragma once

#include <memory>
#include <type_traits>
#include <vector>

#include "point.h"

namespace gpl {

class NesterovBaseCommon;
class DeviceState;
class GCell;
class GCellHandle;
struct BackendContext;

class WirelengthGradientBackend
{
 public:
  virtual ~WirelengthGradientBackend() = default;
  WirelengthGradientBackend(const WirelengthGradientBackend&) = delete;
  WirelengthGradientBackend& operator=(const WirelengthGradientBackend&)
      = delete;
  WirelengthGradientBackend(WirelengthGradientBackend&&) = delete;
  WirelengthGradientBackend& operator=(WirelengthGradientBackend&&) = delete;

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

  // Make the per-inst gradients available on the DEVICE without a host
  // round-trip. Only meaningful for the GPU backend (runs the K5 gather so
  // NesterovDeviceContext::scatterWLGradsToNB can read d_inst_wl_grad_*);
  // the CPU backend has no device buffer and this is a no-op.
  virtual void prepareDeviceGradients() {}

  virtual const char* name() const = 0;

 protected:
  WirelengthGradientBackend() = default;
};

// Factory: GpuWirelengthGradientBackend on ENABLE_GPU + gpuEnabled(), else
// CpuWirelengthGradientBackend. Consumes ctx.nbc (required — both backends
// call back into it for CPU helpers / data access), ctx.num_threads (CPU
// path), and ctx.device_state (GPU path; may be null for the CPU path).
std::unique_ptr<WirelengthGradientBackend> makeWirelengthGradientBackend(
    const BackendContext& ctx);

static_assert(!std::is_copy_constructible_v<WirelengthGradientBackend>);
static_assert(!std::is_move_constructible_v<WirelengthGradientBackend>);

}  // namespace gpl
