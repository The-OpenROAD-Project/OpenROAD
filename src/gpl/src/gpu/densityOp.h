// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// densityOp — Kokkos kernel launchers for the density pipeline.
// K_density_gather: per-inst overlap-weighted sum of bin electric field.
// NB-level scatter/gather: device-resident replacements for the host
// updateBinsGCellDensityArea / getDensityGradient loops (all nb cells,
// fillers included).
// Kokkos-laden header — include only from CUDA/HIP TUs.

#pragma once

#include "nesterovDeviceContext.h"  // SlpSlot (plain C++ header)

namespace gpl {

struct KokkosDeviceState;
struct KokkosNesterovState;

namespace densop {

// Per-inst density gradient gather: reads d_bin_elec_x/y (solver convention),
// applies axis swap + 0.5× scale, accumulates overlap × field per overlapping
// bin. Writes d_inst_density_grad_x/y.
void launchDensityGather(KokkosDeviceState& ds,
                         int n_insts,
                         int bin_cnt_x,
                         int bin_cnt_y,
                         float bin_size_x,
                         float bin_size_y,
                         int grid_lx,
                         int grid_ly);

// NB-level density scatter: replicates BinGrid::updateBinsGCellDensityArea +
// the density/overflow bin pass on device, for all nb cells (inst + filler).
// Reads the float coords of `src`, truncates to int density centers (same
// conversion as GCell::setDensityCenterLocation), accumulates per-bin areas
// with int64 per-contribution truncation (bit-equal to the serial CPU sums),
// writes ds.d_bin_density (FFT layout), and reduces the two overflow sums.
void launchNbDensityScatter(KokkosNesterovState& ns,
                            KokkosDeviceState& ds,
                            int n_cells,
                            SlpSlot src,
                            float& overflow_area,
                            float& overflow_area_unscaled);

// NB-level density gradient gather for all nb cells (fillers included):
// overlap-weighted sum of the bin electric field at the `src` coords.
// Writes ns.d_density_grad_x/y directly (no host round-trip). Bin bounds
// come from the exact per-bin ints (ns.d_bin_lx etc.), matching the CPU's
// lround()-derived Bin geometry.
void launchNbDensityGather(KokkosNesterovState& ns,
                           const KokkosDeviceState& ds,
                           int n_cells,
                           SlpSlot src);

// Σ phi[bin] × float(nonPlace + instPlaced + filler) over all bins — the
// sumPhi_ debug metric. Only call after launchNbDensityScatter + solve.
float launchNbSumPhi(const KokkosNesterovState& ns,
                     const KokkosDeviceState& ds);

}  // namespace densop
}  // namespace gpl
