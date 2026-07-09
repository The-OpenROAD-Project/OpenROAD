// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Kokkos-laden private header for RegionDensityField. Defines
// KokkosRegionDensityField — the per-region FFT field Views (density input,
// electrostatic potential, and the two field components). Only include from
// translation units compiled as CUDA/HIP TUs (gpu/regionDensityField.cpp,
// gpu/gpuFftBackend.cpp, gpu/gpuDensityGradientBackend.cpp,
// gpu/nesterovDeviceContext.cpp, gpu/densityOp.cpp), listed in
// src/gpl/CMakeLists.txt's source-language section.
//
// Including this from a plain CXX TU would pull in <Kokkos_Core.hpp>, which
// expects __CUDACC__ when KOKKOS_ENABLE_CUDA is defined.

#pragma once

#include <Kokkos_Core.hpp>

namespace gpl {

// Per-region FFT field Views (size = binCntX × binCntY, row-major
// [x * binCntY + y]). One instance per placement region (per NesterovBase),
// so concurrent regions in the Nesterov loop never clobber each other's bin
// buffers. The solver's axis convention differs from gpl's — the gather
// kernel applies the axis swap + 0.5× scale inline.
struct KokkosRegionDensityField
{
  Kokkos::View<float*> d_bin_density;  // FFT input (scatter result)
  Kokkos::View<float*> d_bin_phi;      // FFT output (electrostatic potential)
  Kokkos::View<float*> d_bin_elec_x;   // FFT output (solver X = gpl Y)
  Kokkos::View<float*> d_bin_elec_y;   // FFT output (solver Y = gpl X)
  Kokkos::View<float*>::HostMirror h_bin_density;
  Kokkos::View<float*>::HostMirror h_bin_phi;
  Kokkos::View<float*>::HostMirror h_bin_elec_x;
  Kokkos::View<float*>::HostMirror h_bin_elec_y;
};

}  // namespace gpl
