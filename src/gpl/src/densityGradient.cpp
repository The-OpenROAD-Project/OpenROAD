// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Density gradient backends + dispatch. Mirrors wirelengthGradient.cpp.

#include <cstddef>
#include <memory>
#include <vector>

#include "densityGradientBackend.h"
#include "nesterovBase.h"
#include "point.h"

#ifdef ENABLE_GPU
#include "gpu/deviceState.h"
#include "gpu/gpuDensityGradientBackend.h"
#include "gpu/gpuRuntime.h"
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
    for (std::size_t i = 0; i < gCells.size(); ++i) {
      const GCell* c = gCells[i];
      out[i] = nb_->getDensityGradient(c);
    }
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
    NesterovBase* nb,
    DeviceState* device_state)
{
#ifdef ENABLE_GPU
  if (gpuEnabled() && device_state && device_state->numBins() > 0) {
    return std::make_unique<GpuDensityGradientBackend>(nb, device_state);
  }
#else
  (void) device_state;
#endif
  return std::make_unique<CpuDensityGradientBackend>(nb);
}

}  // namespace gpl
