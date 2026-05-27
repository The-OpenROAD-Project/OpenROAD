// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// GpuWirelengthGradientBackend — Kokkos 5-kernel pipeline porting of the
// CPU WA wirelength gradient. Algorithm 1:1 from DG-RePlAce
// (gpl2/src/wirelengthOp.cu); maps naturally to Kokkos
// parallel_for + KOKKOS_LAMBDA.
//
// Compiled only when ENABLE_GPU=ON; the kernel bodies live in wirelengthOp.cpp
// (also a CUDA TU).
//
// Determinism: no atomics. K3 (per-net BC) and K5 (per-inst gather) use
// parallel_for over the outer dim with a serial inner CSR loop; the inner
// summation order matches the CPU OMP loop. Float results within a few ULP
// of CPU.

#include "gpuWirelengthGradientBackend.h"

#include <Kokkos_Core.hpp>
#include <cstddef>
#include <memory>
#include <vector>

#include "cellHandleHelpers.h"
#include "deviceState.h"
#include "deviceState_kokkos.h"
#include "gpuRuntime.h"
#include "nesterovBase.h"
#include "point.h"
#include "wirelengthOp.h"

namespace gpl {

struct GpuWirelengthGradientBackend::Impl
{
  NesterovBaseCommon* nbc;    // borrowed
  DeviceState* device_state;  // borrowed
  // Set true after a getCellGradients/getCellGradient call has read the
  // device gradient buffer into the host mirror — single-cell reads can
  // then re-use the mirror. Reset by updateForce.
  bool host_grad_valid = false;
};

GpuWirelengthGradientBackend::GpuWirelengthGradientBackend(
    NesterovBaseCommon* nbc,
    DeviceState* device_state)
    : impl_(std::make_unique<Impl>())
{
  impl_->nbc = nbc;
  impl_->device_state = device_state;
}

GpuWirelengthGradientBackend::~GpuWirelengthGradientBackend() = default;

void GpuWirelengthGradientBackend::updateForce(float wlCoefX, float wlCoefY)
{
  ensureKokkosInitialized();
  Impl& s = *impl_;
  // Caller (NesterovBaseCommon::updateWireLengthForceWA) is responsible for
  // refreshing d_pin_cx/cy via DeviceState::syncInstCoordsFromHost +
  // updatePinLocations before this entry. Mirrors the hpwl.cpp split.

  KokkosDeviceState& ds = s.device_state->kokkos();
  const int n_pins = s.device_state->numPins();
  const int n_nets = s.device_state->numNets();

  // K1: net bbox.
  wlop::launchUpdateNetBBox(ds, n_nets);
  // K2: per-pin A_pos/neg exponentials.
  wlop::launchComputeAPosNeg(ds, n_pins, wlCoefX, wlCoefY);
  // K3: per-net B, C reductions over CSR.
  wlop::launchComputeBC(ds, n_nets);
  // K4: per-pin gradient (already net-weight multiplied).
  wlop::launchComputePinWAGrad(ds, n_pins, wlCoefX, wlCoefY);

  s.host_grad_valid = false;
}

// Pull device per-inst gradients into the host mirror. Idempotent for the
// same updateForce call (cached via Impl::host_grad_valid) so single-cell
// follow-up reads skip the K5 + copy.
void GpuWirelengthGradientBackend::materializeHostGrad()
{
  Impl& s = *impl_;
  if (s.host_grad_valid) {
    return;
  }
  KokkosDeviceState& ds = s.device_state->kokkos();
  const int n_insts = s.device_state->numInsts();
  // K5: gather per-pin → per-inst with net-weight already folded in K4.
  wlop::launchGatherInstGrad(ds, n_insts);
  Kokkos::deep_copy(ds.h_inst_wl_grad_x, ds.d_inst_wl_grad_x);
  Kokkos::deep_copy(ds.h_inst_wl_grad_y, ds.d_inst_wl_grad_y);
  s.host_grad_valid = true;
}

void GpuWirelengthGradientBackend::getCellGradients(
    const std::vector<GCellHandle>& gCells,
    std::vector<FloatPoint>& out)
{
  materializeHostGrad();
  KokkosDeviceState& ds = impl_->device_state->kokkos();
  // nb_gcells_ mixes (a) NesterovBaseCommon cells, whose storage index ==
  // gCellStor_ index == DeviceState inst index, and (b) NesterovBase-local
  // fillers (fillerStor_) which have no pins and contribute no wirelength
  // gradient — return (0, 0) for those.
  mapNbcGrads(
      gCells,
      [&](std::size_t idx) {
        return FloatPoint(ds.h_inst_wl_grad_x(idx), ds.h_inst_wl_grad_y(idx));
      },
      [](const GCellHandle&) { return FloatPoint(0.0f, 0.0f); },
      out);
}

FloatPoint GpuWirelengthGradientBackend::getCellGradient(const GCell* gCell)
{
  if (gCell->isFiller()) {
    return FloatPoint(0, 0);
  }
  materializeHostGrad();
  KokkosDeviceState& ds = impl_->device_state->kokkos();
  const std::size_t idx = impl_->nbc->getGCellIndex(gCell);
  return FloatPoint(ds.h_inst_wl_grad_x(idx), ds.h_inst_wl_grad_y(idx));
}

}  // namespace gpl
