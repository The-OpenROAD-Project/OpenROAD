// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// GpuWirelengthGradientBackend — Kokkos GPU implementation of
// WirelengthGradientBackend. Compiled only when ENABLE_GPU=ON; constructed
// by makeWirelengthGradientBackend() when the GPU path is selected at run time.
//
// Header is Kokkos-free (PIMPL); the kernel pipeline lives in
// gpuWirelengthGradientBackend.cpp and wirelengthOp.cpp.

#pragma once

#include <cstddef>
#include <memory>
#include <vector>

#include "point.h"
#include "wirelengthGradientBackend.h"

namespace gpl {

class NesterovBaseCommon;
class DeviceState;
class GCell;
class GCellHandle;

class GpuWirelengthGradientBackend : public WirelengthGradientBackend
{
 public:
  // Both pointers borrowed; must outlive this backend. `device_state`
  // supplies the device pool (pin/inst coords, CSRs, net weights). `nbc` is
  // the owning common base — used only to refresh device inst coords from
  // host gCellStor_ before each updateForce (until Phase 4 moves the
  // Nesterov coord update onto the device).
  GpuWirelengthGradientBackend(NesterovBaseCommon* nbc,
                               DeviceState* device_state);
  ~GpuWirelengthGradientBackend() override;

  void updateForce(float wlCoefX, float wlCoefY) override;
  void getCellGradients(const std::vector<GCellHandle>& gCells,
                        std::vector<FloatPoint>& out) override;
  FloatPoint getCellGradient(const GCell* gCell) override;

  const char* name() const override { return "GPU (Kokkos)"; }

 private:
  void materializeHostGrad();
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace gpl
