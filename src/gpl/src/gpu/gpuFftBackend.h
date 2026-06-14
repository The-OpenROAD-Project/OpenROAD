// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// GpuFftBackend — the Kokkos GPU implementation of FftBackend (see
// ../fftBackend.h). Owns a persistent Kokkos Poisson solver and device
// staging Views via PIMPL so this header stays plain C++ — matches the
// pattern used by GpuHpwlBackend / GpuWirelengthGradientBackend /
// GpuDensityGradientBackend, and lets fft.cpp include it without pulling
// in Kokkos transitively.

#pragma once

#include <memory>

#include "fftBackend.h"

namespace gpl {

class DeviceState;

class GpuFftBackend : public FftBackend
{
 public:
  GpuFftBackend(int bin_cnt_x,
                int bin_cnt_y,
                float bin_size_x,
                float bin_size_y,
                DeviceState* device_state);
  ~GpuFftBackend() override;

  // Packs the host density grid into the device View, runs the Poisson
  // solve, and unpacks potential + electric field back into the host
  // grids. All four BinGridSpans share the bin_cnt_x / bin_cnt_y this
  // backend was constructed with and reference flat row-major buffers
  // owned by the FFT context — the same staging layout as the CPU Ooura
  // backend.
  void solve(BinGridSpan density,
             BinGridSpan phi,
             BinGridSpan field_x,
             BinGridSpan field_y) override;

  const char* name() const override { return "GPU (Kokkos Poisson)"; }

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace gpl
