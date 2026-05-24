// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// FftBackend — the Strategy interface for the FFT / Poisson density solve.
// CpuFftBackend (the Ooura DCT) is always available; GpuFftBackend (a Kokkos
// Poisson solver) is added on an ENABLE_GPU build. makeFftBackend() picks one
// per process at run time (gpl::gpuEnabled()).
//
// This header is plain C++ — no Kokkos, no preprocessor branches — so fft.h
// can hold a std::unique_ptr<FftBackend> member without learning anything
// about the GPU build.

#pragma once

#include <memory>

namespace gpl {

// Strategy: solves the Poisson equation on a density grid. The grids are owned
// by the FFT context and passed in by pointer — the backends share gpl's data
// and duplicate no storage. All four arguments are float[bin_cnt_x][bin_cnt_y]
// arrays; solve() reads `density` and writes `phi`, `field_x`, `field_y`.
class FftBackend
{
 public:
  virtual ~FftBackend() = default;

  virtual void solve(float** density,
                     float** phi,
                     float** field_x,
                     float** field_y)
      = 0;

  // Short label for diagnostic logging; constructed-once factory choice.
  virtual const char* name() const = 0;
};

class DeviceState;

// Factory: returns GpuFftBackend on an ENABLE_GPU build with the GPU path
// selected at run time, otherwise CpuFftBackend. `device_state` is the
// device-resident pool (may be null for CPU path; GpuFftBackend borrows
// its bin Views when available, falling back to self-owned Views).
std::unique_ptr<FftBackend> makeFftBackend(int bin_cnt_x,
                                           int bin_cnt_y,
                                           float bin_size_x,
                                           float bin_size_y,
                                           DeviceState* device_state);

}  // namespace gpl
