// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// GpuDensityGradientBackend — density gradient gather on GPU. Reads the
// region's d_bin_elec_x/y (written by GpuFftBackend::solve) and DeviceState's
// per-inst density params, computes overlap-weighted field sum per inst.
// Filler cells fall back to CPU getDensityGradient (fillers aren't in
// DeviceState).

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
#include "regionDensityField.h"
#include "regionDensityField_kokkos.h"

namespace gpl {

struct GpuDensityGradientBackend::Impl
{
  NesterovBase* nb;
  DeviceState* device_state;
  RegionDensityField* region_field;  // per-region bin field Views
};

GpuDensityGradientBackend::GpuDensityGradientBackend(
    NesterovBase* nb,
    DeviceState* device_state,
    RegionDensityField* region_field)
    : impl_(std::make_unique<Impl>())
{
  impl_->nb = nb;
  impl_->device_state = device_state;
  impl_->region_field = region_field;
}

GpuDensityGradientBackend::~GpuDensityGradientBackend() = default;

void GpuDensityGradientBackend::materializeHostGrad()
{
  DeviceState* ds = impl_->device_state;
  KokkosDeviceState& ks = ds->kokkos();
  RegionDensityField* rf = impl_->region_field;

  densop::launchDensityGather(ks,
                              rf->kokkos(),
                              ds->numInsts(),
                              rf->binCntX(),
                              rf->binCntY(),
                              rf->binSizeX(),
                              rf->binSizeY(),
                              rf->gridLx(),
                              rf->gridLy());
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
  // Instances: read the per-inst gradient from the device gather mirror.
  // Fillers (not in DeviceState) need the host bin-overlap computation, which
  // is a serial hotspot on the GPU path — defer them to the parallel batch
  // pass below instead of computing them one-by-one here.
  mapNbcGrads(
      gCells,
      [&](std::size_t idx) {
        return FloatPoint(ds.h_inst_density_grad_x(idx),
                          ds.h_inst_density_grad_y(idx));
      },
      [](const GCellHandle&) { return FloatPoint(0.0f, 0.0f); },
      out);
  nb->fillFillerDensityGradients(gCells, out);
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
