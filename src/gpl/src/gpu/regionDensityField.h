// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// RegionDensityField — owns the per-region FFT field Views (density input,
// electrostatic potential, and the two field components) plus the bin-grid
// geometry they are sized to. One instance per placement region, owned by the
// region's NesterovBase.
//
// Why per-region: gpl builds one NesterovBase per placement region (top-level
// core + one per power-domain / odb group) but a single shared
// NesterovBaseCommon / DeviceState. Each region has its own BinGrid, so its
// FFT field buffers differ in size and content. Keeping them here — rather
// than in the shared DeviceState — lets concurrent regions in the Nesterov
// loop solve and gather against their own bins without clobbering each other
// (regions advance one step each per outer iteration; back-tracking solves
// all regions then gathers all regions).
//
// Used by BOTH GPU density paths:
//   - device-resident (NesterovDeviceContext): writes/reads these Views via
//     the launchNb* kernels; all bin geometry comes from KokkosNesterovState,
//     these Views are the only shared field storage it needs.
//   - host-staged (FFT / GpuFftBackend + GpuDensityGradientBackend), used in
//     timing-driven / routability mode where the device context is disabled.
// Because the host-staged path runs when the device context is off, this
// field state must live on NesterovBase (not in the device context).
//
// PIMPL: Kokkos types are hidden in gpu/regionDensityField_kokkos.h, included
// only by Kokkos-aware translation units. This header is plain C++, so
// consumer TUs (nesterovBase.cpp in particular) need not be compiled by nvcc.
//
// Compiled only when ENABLE_GPU=ON.

#pragma once

#include <memory>
#include <type_traits>

namespace gpl {

class BinGrid;

struct KokkosRegionDensityField;  // gpu/regionDensityField_kokkos.h

class RegionDensityField
{
 public:
  // Allocate the four field Views (+ host mirrors) sized to the region's bin
  // grid, and capture the grid geometry. Constructed once in the region's
  // NesterovBase constructor, right after bg_.initBins(). The bin grid is
  // assumed fixed for this object's lifetime — there is no rebuild(); if the
  // grid ever changes, the owner must replace the unique_ptr. Must precede any
  // density solve / gather kernel or GpuFftBackend solve for this region.
  explicit RegionDensityField(const BinGrid& binGrid);
  RegionDensityField() = delete;
  // Default destructor — the function-pointer deleter on kokkos_ lets this
  // stay inline without requiring KokkosRegionDensityField to be complete
  // here (mirrors DeviceState).
  ~RegionDensityField() = default;

  // Non-copyable, non-movable: an implicit move would inherit a possibly null
  // deleter, masking the "must construct via the GPU ctor" invariant.
  RegionDensityField(const RegionDensityField&) = delete;
  RegionDensityField& operator=(const RegionDensityField&) = delete;
  RegionDensityField(RegionDensityField&&) = delete;
  RegionDensityField& operator=(RegionDensityField&&) = delete;

  int numBins() const { return num_bins_; }
  int binCntX() const { return bin_cnt_x_; }
  int binCntY() const { return bin_cnt_y_; }
  float binSizeX() const { return bin_size_x_; }
  float binSizeY() const { return bin_size_y_; }
  int gridLx() const { return grid_lx_; }
  int gridLy() const { return grid_ly_; }

  // Accessor for Kokkos-aware backend translation units. Consumers must also
  // #include "regionDensityField_kokkos.h" to use the returned reference.
  KokkosRegionDensityField& kokkos() { return *kokkos_; }
  const KokkosRegionDensityField& kokkos() const { return *kokkos_; }

 private:
  using KokkosDeleter = void (*)(KokkosRegionDensityField*);
  std::unique_ptr<KokkosRegionDensityField, KokkosDeleter> kokkos_{nullptr,
                                                                   nullptr};

  int num_bins_ = 0;
  int bin_cnt_x_ = 0;
  int bin_cnt_y_ = 0;
  float bin_size_x_ = 0;
  float bin_size_y_ = 0;
  int grid_lx_ = 0;
  int grid_ly_ = 0;
};

// Lock the "must construct via the GPU ctor" invariant at compile time (see
// the same guard on DeviceState).
static_assert(!std::is_default_constructible_v<RegionDensityField>);
static_assert(!std::is_copy_constructible_v<RegionDensityField>);
static_assert(!std::is_move_constructible_v<RegionDensityField>);

}  // namespace gpl
