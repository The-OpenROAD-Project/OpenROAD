// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// GpuFftBackend — the Kokkos / KokkosFFT implementation of FftBackend,
// compiled only when ENABLE_GPU=ON. It owns a persistent Kokkos Poisson
// solver and device staging Views; solve() packs the host density grid to
// the device, runs the solve, and unpacks potential + electric field back.
// makeFftBackend() (in ../fft.cpp) constructs it when the GPU path is
// selected at run time.

#include "gpuFftBackend.h"

#include <Kokkos_Core.hpp>
#include <cstddef>
#include <memory>

#include "gpuRuntime.h"
#include "poissonSolver.h"
#include "regionDensityField.h"
#include "regionDensityField_kokkos.h"

namespace gpl {

// The solver→gpl axis swap + 0.5× field scale go through
// poissonSolver.h::solverToGplField (shared with the device density gather
// in densityOp.cpp) — single source of truth. Pinned by GpuFFTTest in
// src/gpl/test/fft_gpu_test.cc.

struct GpuFftBackend::Impl
{
  Impl(int bin_cnt_x,
       int bin_cnt_y,
       float bin_size_x,
       float bin_size_y,
       RegionDensityField* region_field)
      : bin_cnt_x(bin_cnt_x),
        bin_cnt_y(bin_cnt_y),
        // The Poisson solver's binCntX axis is gpl's fast (y) axis, so the
        // flat layout [h*binCntX + w] equals gpl's [x][y] when binCntX =
        // bin_cnt_y. The bin-size axes swap with the count axes (only the
        // ratio is used).
        solver(bin_cnt_y, bin_cnt_x, bin_size_y, bin_size_x),
        region_field(region_field),
        d_density("fft_gpu_density",
                  static_cast<size_t>(bin_cnt_x) * bin_cnt_y),
        d_phi("fft_gpu_phi", static_cast<size_t>(bin_cnt_x) * bin_cnt_y),
        d_elec_x("fft_gpu_elec_x", static_cast<size_t>(bin_cnt_x) * bin_cnt_y),
        d_elec_y("fft_gpu_elec_y", static_cast<size_t>(bin_cnt_x) * bin_cnt_y),
        h_density(Kokkos::create_mirror_view(d_density)),
        h_phi(Kokkos::create_mirror_view(d_phi)),
        h_elec_x(Kokkos::create_mirror_view(d_elec_x)),
        h_elec_y(Kokkos::create_mirror_view(d_elec_y))
  {
  }

  int bin_cnt_x;
  int bin_cnt_y;

  PoissonSolver solver;
  RegionDensityField* region_field;  // borrowed per-region; may be null when
                                     // there is no region field (e.g. the
                                     // standalone FFT unit test)

  // Self-owned staging Views — used when no region field is attached (the
  // fft_gpu_test path constructs an FFT with no region field). When a region
  // field is present, solve() routes to its Views so the density gather
  // kernel can read them directly on device.
  Kokkos::View<float*> d_density;
  Kokkos::View<float*> d_phi;
  Kokkos::View<float*> d_elec_x;
  Kokkos::View<float*> d_elec_y;
  Kokkos::View<float*>::HostMirror h_density;
  Kokkos::View<float*>::HostMirror h_phi;
  Kokkos::View<float*>::HostMirror h_elec_x;
  Kokkos::View<float*>::HostMirror h_elec_y;
};

GpuFftBackend::GpuFftBackend(int bin_cnt_x,
                             int bin_cnt_y,
                             float bin_size_x,
                             float bin_size_y,
                             RegionDensityField* region_field)
    : impl_(std::make_unique<Impl>(bin_cnt_x,
                                   bin_cnt_y,
                                   bin_size_x,
                                   bin_size_y,
                                   region_field))
{
}

GpuFftBackend::~GpuFftBackend() = default;

void GpuFftBackend::solve(BinGridSpan density,
                          BinGridSpan phi,
                          BinGridSpan field_x,
                          BinGridSpan field_y)
{
  ensureKokkosInitialized();
  auto& impl = *impl_;

  // Pack density into the flat row-major View the Poisson solver expects:
  // it indexes binDensity[h*binCntX + w] with binCntX = bin_cnt_y, so the
  // flat index x*bin_cnt_y + y matches gpl's own [x][y] grid.
  for (int x = 0; x < impl.bin_cnt_x; x++) {
    for (int y = 0; y < impl.bin_cnt_y; y++) {
      impl.h_density(static_cast<size_t>(x) * impl.bin_cnt_y + y)
          = density(x, y);
    }
  }

  // If a region field is attached, solve into its bin Views so the density
  // gather kernel can read them directly on device. The host unpack below
  // reads from the region field's host mirrors.
  const bool use_field = impl.region_field && impl.region_field->numBins() > 0;
  if (use_field) {
    KokkosRegionDensityField& rdf = impl.region_field->kokkos();
    Kokkos::deep_copy(rdf.d_bin_density, impl.h_density);
    impl.solver.solvePoisson(
        rdf.d_bin_density, rdf.d_bin_phi, rdf.d_bin_elec_x, rdf.d_bin_elec_y);
    Kokkos::fence();
    Kokkos::deep_copy(rdf.h_bin_phi, rdf.d_bin_phi);
    Kokkos::deep_copy(rdf.h_bin_elec_x, rdf.d_bin_elec_x);
    Kokkos::deep_copy(rdf.h_bin_elec_y, rdf.d_bin_elec_y);

    for (int x = 0; x < impl.bin_cnt_x; x++) {
      for (int y = 0; y < impl.bin_cnt_y; y++) {
        const size_t k = static_cast<size_t>(x) * impl.bin_cnt_y + y;
        phi(x, y) = rdf.h_bin_phi(k);
        const GplField f
            = solverToGplField(rdf.h_bin_elec_x(k), rdf.h_bin_elec_y(k));
        field_x(x, y) = f.x;
        field_y(x, y) = f.y;
      }
    }
  } else {
    Kokkos::deep_copy(impl.d_density, impl.h_density);
    impl.solver.solvePoisson(
        impl.d_density, impl.d_phi, impl.d_elec_x, impl.d_elec_y);
    Kokkos::fence();
    Kokkos::deep_copy(impl.h_phi, impl.d_phi);
    Kokkos::deep_copy(impl.h_elec_x, impl.d_elec_x);
    Kokkos::deep_copy(impl.h_elec_y, impl.d_elec_y);

    for (int x = 0; x < impl.bin_cnt_x; x++) {
      for (int y = 0; y < impl.bin_cnt_y; y++) {
        const size_t k = static_cast<size_t>(x) * impl.bin_cnt_y + y;
        phi(x, y) = impl.h_phi(k);
        const GplField f = solverToGplField(impl.h_elec_x(k), impl.h_elec_y(k));
        field_x(x, y) = f.x;
        field_y(x, y) = f.y;
      }
    }
  }
}

}  // namespace gpl
