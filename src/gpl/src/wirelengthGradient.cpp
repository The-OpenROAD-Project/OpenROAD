// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// WA wirelength gradient backends + dispatch. Mirrors hpwl.cpp.
//
// CpuWirelengthGradientBackend wraps the existing OMP loops in
// NesterovBaseCommon. GpuWirelengthGradientBackend (a 5-kernel Kokkos
// pipeline) is added on ENABLE_GPU. makeWirelengthGradientBackend() picks
// per-process at run time via gpl::gpuEnabled().

#include <cassert>
#include <cstddef>
#include <memory>
#include <vector>

#include "backendContext.h"
#include "nesterovBase.h"
#include "point.h"
#include "wirelengthGradientBackend.h"

#ifdef ENABLE_GPU
#include "gpu/deviceState.h"
#include "gpu/gpuRuntime.h"
#include "gpu/gpuWirelengthGradientBackend.h"
#endif

namespace gpl {

namespace {

// CPU backend: thin wrapper around the existing nbc methods. The OMP loops
// live in NesterovBaseCommon::updateWireLengthForceWA_native.
class CpuWirelengthGradientBackend : public WirelengthGradientBackend
{
 public:
  explicit CpuWirelengthGradientBackend(NesterovBaseCommon* nbc) : nbc_(nbc) {}

  void updateForce(float wlCoefX, float wlCoefY) override
  {
    last_wl_coef_x_ = wlCoefX;
    last_wl_coef_y_ = wlCoefY;
    nbc_->updateWireLengthForceWA_native(wlCoefX, wlCoefY);
  }

  void getCellGradients(const std::vector<GCellHandle>& gCells,
                        std::vector<FloatPoint>& out) override
  {
    assert(out.size() == gCells.size());
#pragma omp parallel for num_threads(static_cast<int>(nbc_->getNumThreads()))
    for (std::size_t i = 0; i < gCells.size(); ++i) {
      const GCell* gCell = gCells[i];
      out[i] = nbc_->getWireLengthGradientWA(
          gCell, last_wl_coef_x_, last_wl_coef_y_);
    }
  }

  FloatPoint getCellGradient(const GCell* gCell) override
  {
    return nbc_->getWireLengthGradientWA(
        gCell, last_wl_coef_x_, last_wl_coef_y_);
  }

  const char* name() const override { return "CPU (OpenMP)"; }

 private:
  NesterovBaseCommon* nbc_;
  // Backend contract: updateForce() must precede getCellGradient(s); the
  // CPU helper takes (coefX, coefY) per call so we replay the last values.
  float last_wl_coef_x_ = 0;
  float last_wl_coef_y_ = 0;
};

}  // namespace

std::unique_ptr<WirelengthGradientBackend> makeWirelengthGradientBackend(
    const BackendContext& ctx)
{
#ifdef ENABLE_GPU
  if (gpuEnabled()) {
    ensureKokkosInitialized();
    return std::make_unique<GpuWirelengthGradientBackend>(ctx.nbc,
                                                          ctx.device_state);
  }
#endif
  return std::make_unique<CpuWirelengthGradientBackend>(ctx.nbc);
}

//
// NesterovBaseCommon hooks. Defined out-of-line here so this TU owns the
// backend dispatch in one place. The native CPU body
// (updateWireLengthForceWA_native) and per-cell helpers stay in
// nesterovBase.cpp.
//
void NesterovBaseCommon::updateWireLengthForceWA(float wlCoeffX, float wlCoeffY)
{
#ifdef ENABLE_GPU
  // Sync the device-resident pin coords on the GPU path. ensureCoordsFresh
  // skips the host→device round-trip when NB has already scattered fresh
  // inst coords this iteration (e.g. init paths before nb_device_ctx_
  // exists fall through to the actual sync).
  if (device_state_) {
    device_state_->ensureCoordsFresh(gCellStor_);
  }
#endif
  wl_grad_backend_->updateForce(wlCoeffX, wlCoeffY);
}

void NesterovBaseCommon::getAllWireLengthGradientsWA(
    const std::vector<GCellHandle>& gCells,
    std::vector<FloatPoint>& out)
{
  wl_grad_backend_->getCellGradients(gCells, out);
}

void NesterovBaseCommon::prepareDeviceWlGradients()
{
  wl_grad_backend_->prepareDeviceGradients();
}

FloatPoint NesterovBaseCommon::getSingleWireLengthGradientWA(const GCell* gCell)
{
  return wl_grad_backend_->getCellGradient(gCell);
}

}  // namespace gpl
