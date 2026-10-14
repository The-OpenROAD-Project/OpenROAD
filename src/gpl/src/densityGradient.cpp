// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Density gradient backends + dispatch. Mirrors wirelengthGradient.cpp.

#include <Kokkos_Core.hpp>
#include <cstddef>
#include <memory>
#include <vector>

#include "backendContext.h"
#include "densityGradientBackend.h"
#include "kokkosRuntime.h"
#include "nesterovBase.h"
#include "point.h"

#ifdef ENABLE_GPU
#include "gpu/deviceState.h"
#include "gpu/gpuDensityGradientBackend.h"
#include "gpu/gpuRuntime.h"
#include "gpu/regionDensityField.h"
#endif

namespace gpl {

namespace {

class CpuDensityGradientBackend : public DensityGradientBackend
{
 public:
  explicit CpuDensityGradientBackend(NesterovBase* nb) : nb_(nb) {}

  void getCellGradients(const std::vector<GCellHandle>& gCells,
                        std::vector<FloatPoint>& out) override
  {
    const auto space
        = hostExecutionSpace(static_cast<int>(nb_->getNbc()->getNumThreads()));
    Kokkos::parallel_for("gpl::densityGradients",
                         HostRange(space, 0, gCells.size()),
                         [&](std::size_t i) {
                           const GCell* c = gCells[i];
                           out[i] = nb_->getDensityGradient(c);
                         });
  }

  FloatPoint getCellGradient(const GCell* gCell) override
  {
    return nb_->getDensityGradient(gCell);
  }

  const char* name() const override { return "CPU"; }

 private:
  NesterovBase* nb_;
};

}  // namespace

std::unique_ptr<DensityGradientBackend> makeDensityGradientBackend(
    const BackendContext& ctx)
{
#ifdef ENABLE_GPU
  if (gpuEnabled() && ctx.device_state && ctx.region_field
      && ctx.region_field->numBins() > 0) {
    return std::make_unique<GpuDensityGradientBackend>(
        ctx.nb, ctx.device_state, ctx.region_field);
  }
#endif
  return std::make_unique<CpuDensityGradientBackend>(ctx.nb);
}

}  // namespace gpl
