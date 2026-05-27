// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// densityOp — Kokkos kernel launcher for density gradient gather.
// K_density_gather: per-inst overlap-weighted sum of bin electric field.
// Kokkos-laden header — include only from CUDA/HIP TUs.

#pragma once

namespace gpl {

struct KokkosDeviceState;

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

}  // namespace densop
}  // namespace gpl
