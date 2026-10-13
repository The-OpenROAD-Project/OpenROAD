// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "regionDensityField.h"

#include <Kokkos_Core.hpp>

#include "gpuRuntime.h"
#include "nesterovBase.h"
#include "regionDensityField_kokkos.h"

namespace gpl {

namespace {

// Deleter passed to the type-erased unique_ptr in regionDensityField.h.
// Defined here where KokkosRegionDensityField is complete.
void deleteKokkosRegionDensityField(KokkosRegionDensityField* p)
{
  delete p;
}

}  // namespace

RegionDensityField::RegionDensityField(const BinGrid& binGrid)
    : kokkos_(new KokkosRegionDensityField(), &deleteKokkosRegionDensityField)
{
  ensureKokkosInitialized();

  // Identical casts to the former DeviceState::initBinViews so single-region
  // results stay bit-for-bit unchanged.
  bin_cnt_x_ = binGrid.getBinCntX();
  bin_cnt_y_ = binGrid.getBinCntY();
  bin_size_x_ = static_cast<float>(binGrid.getBinSizeX());
  bin_size_y_ = static_cast<float>(binGrid.getBinSizeY());
  grid_lx_ = binGrid.lx();
  grid_ly_ = binGrid.ly();
  num_bins_ = bin_cnt_x_ * bin_cnt_y_;

  auto& s = *kokkos_;
  s.d_bin_density = Kokkos::View<float*>("rdf_bin_density", num_bins_);
  s.d_bin_phi = Kokkos::View<float*>("rdf_bin_phi", num_bins_);
  s.d_bin_elec_x = Kokkos::View<float*>("rdf_bin_elec_x", num_bins_);
  s.d_bin_elec_y = Kokkos::View<float*>("rdf_bin_elec_y", num_bins_);
  s.h_bin_density = Kokkos::create_mirror_view(s.d_bin_density);
  s.h_bin_phi = Kokkos::create_mirror_view(s.d_bin_phi);
  s.h_bin_elec_x = Kokkos::create_mirror_view(s.d_bin_elec_x);
  s.h_bin_elec_y = Kokkos::create_mirror_view(s.d_bin_elec_y);
}

}  // namespace gpl
