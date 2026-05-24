// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// DensityGradientBackend — Strategy interface for the density gradient
// (per-cell electric field gather). CpuDensityGradientBackend wraps the
// existing getDensityGradient per-cell loop; GpuDensityGradientBackend runs a
// Kokkos kernel reading device-resident field Views from the FFT solve.
//
// NB-level (NesterovBase), not NBC-level — the BinGrid and FFT are per-NB.
// Plain C++ header (no Kokkos).

#pragma once

#include <memory>
#include <vector>

#include "point.h"

namespace gpl {

class DeviceState;
class GCell;
class GCellHandle;
class NesterovBase;

class DensityGradientBackend
{
 public:
  virtual ~DensityGradientBackend() = default;

  virtual void getCellGradients(const std::vector<GCellHandle>& gCells,
                                std::vector<FloatPoint>& out)
      = 0;

  virtual FloatPoint getCellGradient(const GCell* gCell) = 0;

  virtual const char* name() const = 0;
};

std::unique_ptr<DensityGradientBackend> makeDensityGradientBackend(
    NesterovBase* nb,
    DeviceState* device_state);

}  // namespace gpl
