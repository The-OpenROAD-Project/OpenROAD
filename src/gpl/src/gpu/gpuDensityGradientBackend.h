// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// GpuDensityGradientBackend — Kokkos GPU density gradient gather.
// Kokkos-free PIMPL header.

#pragma once

#include <cstddef>
#include <memory>
#include <vector>

#include "densityGradientBackend.h"
#include "point.h"

namespace gpl {

class DeviceState;
class GCell;
class GCellHandle;
class NesterovBase;
class RegionDensityField;

class GpuDensityGradientBackend : public DensityGradientBackend
{
 public:
  GpuDensityGradientBackend(NesterovBase* nb,
                            DeviceState* device_state,
                            RegionDensityField* region_field);
  ~GpuDensityGradientBackend() override;

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
