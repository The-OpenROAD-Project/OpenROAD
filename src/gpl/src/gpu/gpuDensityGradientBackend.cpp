// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// GpuDensityGradientBackend — density gradient gather on GPU. Reads
// DeviceState's d_bin_elec_x/y (written by GpuFftBackend::solve) and per-inst
// density params, computes overlap-weighted field sum per inst. Filler cells
// fall back to CPU getDensityGradient (fillers aren't in DeviceState).

#include "gpuDensityGradientBackend.h"

#include <Kokkos_Core.hpp>
#include <cstddef>
#include <memory>
#include <vector>

#include "cellHandleHelpers.h"
#include "densityOp.h"
#include "deviceState.h"
#include "deviceState_kokkos.h"
#include "gpuRuntime.h"
#include "nesterovBase.h"
#include "point.h"

namespace gpl {

struct GpuDensityGradientBackend::Impl
{
  NesterovBase* nb;
  DeviceState* device_state;
};

GpuDensityGradientBackend::GpuDensityGradientBackend(NesterovBase* nb,
                                                     DeviceState* device_state)
    : impl_(std::make_unique<Impl>())
{
  impl_->nb = nb;
  impl_->device_state = device_state;
}

GpuDensityGradientBackend::~GpuDensityGradientBackend() = default;

void GpuDensityGradientBackend::materializeHostGrad()
{
  DeviceState* ds = impl_->device_state;
  KokkosDeviceState& ks = ds->kokkos();

  densop::launchDensityGather(ks,
                              ds->numInsts(),
                              ds->binCntX(),
                              ds->binCntY(),
                              ds->binSizeX(),
                              ds->binSizeY(),
                              ds->gridLx(),
                              ds->gridLy());
  Kokkos::deep_copy(ks.h_inst_density_grad_x, ks.d_inst_density_grad_x);
  Kokkos::deep_copy(ks.h_inst_density_grad_y, ks.d_inst_density_grad_y);
}

void GpuDensityGradientBackend::getCellGradients(
    const std::vector<GCellHandle>& gCells,
    std::vector<FloatPoint>& out)
{
  materializeHostGrad();
  KokkosDeviceState& ds = impl_->device_state->kokkos();
  NesterovBase* nb = impl_->nb;
  // Filler: CPU fallback (filler has non-zero density gradient but isn't in
  // DeviceState). Host bin fields are populated by the FFT unpack.
  mapNbcGrads(
      gCells,
      [&](std::size_t idx) {
        return FloatPoint(ds.h_inst_density_grad_x(idx),
                          ds.h_inst_density_grad_y(idx));
      },
      [&](const GCellHandle& h) { return nb->getDensityGradient(h); },
      out);
}

FloatPoint GpuDensityGradientBackend::getCellGradient(const GCell* gCell)
{
  if (gCell->isFiller()) {
    return impl_->nb->getDensityGradient(gCell);
  }
  materializeHostGrad();
  KokkosDeviceState& ds = impl_->device_state->kokkos();
  const std::size_t idx = impl_->nb->getNbc()->getGCellIndex(gCell);
  return FloatPoint(ds.h_inst_density_grad_x(idx),
                    ds.h_inst_density_grad_y(idx));
}

}  // namespace gpl
