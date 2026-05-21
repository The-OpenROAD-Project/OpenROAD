// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// GpuFftBackend — the Kokkos GPU implementation of FftBackend (see
// ../fftBackend.h). It owns a persistent Kokkos Poisson solver and device
// staging Views, constructed once and reused for every solve().
//
// Compiled only when ENABLE_GPU=ON; constructed by makeFftBackend() when the
// GPU path is selected at run time. This header is Kokkos-dependent, so it is
// included only by CUDA/HIP translation units — gpu/gpuFftBackend.cpp and the
// FFT factory in ../fft.cpp.

#pragma once

#include <Kokkos_Core.hpp>

#include "fftBackend.h"
#include "poissonSolver.h"

namespace gpl {

class GpuFftBackend : public FftBackend
{
 public:
  GpuFftBackend(int bin_cnt_x,
                int bin_cnt_y,
                float bin_size_x,
                float bin_size_y);

  // Packs the host density grid into the device View, runs the Poisson solve,
  // and unpacks potential + electric field back into the host grids. All four
  // arguments are float[bin_cnt_x][bin_cnt_y] host arrays owned by the FFT
  // context — the same staging layout as the CPU Ooura backend.
  void solve(float** density,
             float** phi,
             float** field_x,
             float** field_y) override;

  const char* name() const override { return "GPU (Kokkos Poisson)"; }

 private:
  int bin_cnt_x_;
  int bin_cnt_y_;

  PoissonSolver solver_;
  Kokkos::View<float*> d_density_;
  Kokkos::View<float*> d_phi_;
  Kokkos::View<float*> d_elec_x_;  // PoissonSolver electroForceX → gpl fy axis
  Kokkos::View<float*> d_elec_y_;  // PoissonSolver electroForceY → gpl fx axis
  // Persistent host mirrors paired with the four device staging Views above.
  // Reused across solve() calls so each invocation skips four host-side mirror
  // allocations -- measurably significant in the placement hot path.
  Kokkos::View<float*>::HostMirror h_density_;
  Kokkos::View<float*>::HostMirror h_phi_;
  Kokkos::View<float*>::HostMirror h_elec_x_;
  Kokkos::View<float*>::HostMirror h_elec_y_;
};

}  // namespace gpl
