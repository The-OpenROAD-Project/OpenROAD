// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// GpuFftBackend — the Kokkos / KokkosFFT implementation of FftBackend,
// compiled only when ENABLE_GPU=ON. It owns a persistent Kokkos Poisson solver
// and device staging Views; solve() packs the host density grid to the device,
// runs the solve, and unpacks potential + electric field back. makeFftBackend()
// (in ../fft.cpp) constructs it when the GPU path is selected at run time.

#include "gpuFftBackend.h"

#include <Kokkos_Core.hpp>
#include <cstddef>

#include "gpuRuntime.h"
#include "poissonSolver.h"

namespace gpl {

// The solver's DCT-derived electric field is 2x what the legacy CPU Ooura
// backend produces (the gpl convention); halve it on unpack so consumers see
// the same magnitudes regardless of backend. Pinned by GpuFFTTest in
// src/gpl/test/fft_gpu_test.cc.
namespace {
constexpr float kSolverToGplFieldScale = 0.5f;
}  // namespace

GpuFftBackend::GpuFftBackend(int bin_cnt_x,
                             int bin_cnt_y,
                             float bin_size_x,
                             float bin_size_y)
    : bin_cnt_x_(bin_cnt_x),
      bin_cnt_y_(bin_cnt_y),
      // The Poisson solver's binCntX axis is gpl's fast (y) axis, so the flat
      // layout [h*binCntX + w] equals gpl's [x][y] when binCntX = bin_cnt_y.
      // The bin-size axes swap with the count axes (only the ratio is used).
      solver_(bin_cnt_y, bin_cnt_x, bin_size_y, bin_size_x),
      d_density_("fft_gpu_density", static_cast<size_t>(bin_cnt_x) * bin_cnt_y),
      d_phi_("fft_gpu_phi", static_cast<size_t>(bin_cnt_x) * bin_cnt_y),
      d_elec_x_("fft_gpu_elec_x", static_cast<size_t>(bin_cnt_x) * bin_cnt_y),
      d_elec_y_("fft_gpu_elec_y", static_cast<size_t>(bin_cnt_x) * bin_cnt_y),
      h_density_(Kokkos::create_mirror_view(d_density_)),
      h_phi_(Kokkos::create_mirror_view(d_phi_)),
      h_elec_x_(Kokkos::create_mirror_view(d_elec_x_)),
      h_elec_y_(Kokkos::create_mirror_view(d_elec_y_))
{
  // Kokkos must be live before any View above is touched; the ctor body runs
  // after the member init list, so ensureKokkosInitialized() here would be too
  // late for the Views — initialization is therefore driven from
  // makeFftBackend() before GpuFftBackend is constructed.
}

void GpuFftBackend::solve(float** density,
                          float** phi,
                          float** field_x,
                          float** field_y)
{
  ensureKokkosInitialized();

  // Pack density into the flat row-major View the Poisson solver expects: it
  // indexes binDensity[h*binCntX + w] with binCntX = bin_cnt_y_, so the flat
  // index x*bin_cnt_y_ + y matches gpl's own [x][y] grid.
  for (int x = 0; x < bin_cnt_x_; x++) {
    for (int y = 0; y < bin_cnt_y_; y++) {
      h_density_(static_cast<size_t>(x) * bin_cnt_y_ + y) = density[x][y];
    }
  }
  Kokkos::deep_copy(d_density_, h_density_);

  solver_.solvePoisson(d_density_, d_phi_, d_elec_x_, d_elec_y_);
  Kokkos::fence();

  Kokkos::deep_copy(h_phi_, d_phi_);
  Kokkos::deep_copy(h_elec_x_, d_elec_x_);
  Kokkos::deep_copy(h_elec_y_, d_elec_y_);

  // Unpack. Two reconciliations vs the legacy CPU Ooura FFT:
  //   (1) axis swap — the solver's electroForceX is the force along gpl's
  //       fast (y) axis and electroForceY along the slow (x) axis;
  //   (2) field scale — kSolverToGplFieldScale (see top of file).
  // phi matches gpl 1:1, copied as-is.
  for (int x = 0; x < bin_cnt_x_; x++) {
    for (int y = 0; y < bin_cnt_y_; y++) {
      const size_t k = static_cast<size_t>(x) * bin_cnt_y_ + y;
      phi[x][y] = h_phi_(k);
      field_x[x][y] = kSolverToGplFieldScale * h_elec_y_(k);
      field_y[x][y] = kSolverToGplFieldScale * h_elec_x_(k);
    }
  }
}

}  // namespace gpl
