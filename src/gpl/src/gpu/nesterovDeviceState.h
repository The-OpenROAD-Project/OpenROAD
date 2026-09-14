// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// NesterovBase-level device arrays. Parallel to nb_gcells_
// (inst + filler cells). Owned by NesterovBase; distinct from the
// NesterovBaseCommon-level DeviceState which holds inst-only data
// (pin/net CSRs, WA gradient Views, etc.).
//
// Kokkos-laden — include only from CUDA/HIP TUs.

#pragma once

#include <Kokkos_Core.hpp>
#include <memory>

#include "poissonSolver.h"

namespace gpl {

struct KokkosNesterovState
{
  // ---- Per-cell Nesterov coordinates (size = num_nb_cells) ----
  // SLP = Steepest-descent with Lipschitz-constant Prediction
  Kokkos::View<float*> d_cur_slp_x;
  Kokkos::View<float*> d_cur_slp_y;
  Kokkos::View<float*> d_prev_slp_x;
  Kokkos::View<float*> d_prev_slp_y;
  Kokkos::View<float*> d_next_slp_x;
  Kokkos::View<float*> d_next_slp_y;
  Kokkos::View<float*> d_cur_x;
  Kokkos::View<float*> d_cur_y;
  Kokkos::View<float*> d_next_x;
  Kokkos::View<float*> d_next_y;

  // ---- Per-cell gradients ----
  Kokkos::View<float*> d_wl_grad_x;
  Kokkos::View<float*> d_wl_grad_y;
  Kokkos::View<float*> d_density_grad_x;
  Kokkos::View<float*> d_density_grad_y;

  // Combined preconditioned gradient output.
  Kokkos::View<float*> d_cur_sum_grads_x;
  Kokkos::View<float*> d_cur_sum_grads_y;
  Kokkos::View<float*> d_prev_sum_grads_x;
  Kokkos::View<float*> d_prev_sum_grads_y;
  Kokkos::View<float*> d_next_sum_grads_x;
  Kokkos::View<float*> d_next_sum_grads_y;

  // ---- Per-cell static (set once at init) ----
  Kokkos::View<int*> d_num_pins;   // for WL preconditioner
  Kokkos::View<float*> d_area;     // for density preconditioner
  Kokkos::View<int*> d_locked;     // 1 if locked, 0 otherwise
  Kokkos::View<int*> d_nbc_index;  // gCellStor_ index (-1 for fillers)

  // Coord clamp bounds (density layout inside). Static for main loop.
  Kokkos::View<float*> d_clamp_lx;
  Kokkos::View<float*> d_clamp_ly;
  Kokkos::View<float*> d_clamp_ux;
  Kokkos::View<float*> d_clamp_uy;

  // ---- Device-resident density pipeline (scatter + Poisson + gather) ----
  //
  // Per-cell density params, covering ALL nb cells (inst + filler) — unlike
  // DeviceState's inst-only d_inst_density_*. Refreshed by
  // NesterovDeviceContext::refreshCellDensityParams after updateDensitySize.
  Kokkos::View<int*> d_half_ddx;         // GCell::dDx()/2
  Kokkos::View<int*> d_half_ddy;         // GCell::dDy()/2
  Kokkos::View<float*> d_density_scale;  // GCell::getDensityScale()
  // 0 = neither (skipped by the scatter, like the CPU loop), 1 = std inst,
  // 2 = macro inst (target-density scaled), 3 = filler.
  Kokkos::View<int*> d_cell_kind;

  // Per-bin statics, FFT layout [x * binCntY + y] (matches d_bin_density /
  // d_bin_elec_* in KokkosDeviceState; NOT BinGrid's [y * binCntX + x]).
  // Bin bounds are the exact lround()-derived ints from BinGrid::initBins,
  // so the device overlap math matches the CPU Bin geometry bit-for-bit.
  Kokkos::View<int*> d_bin_lx;
  Kokkos::View<int*> d_bin_ly;
  Kokkos::View<int*> d_bin_ux;
  Kokkos::View<int*> d_bin_uy;
  Kokkos::View<float*> d_bin_target_density;
  // Precomputed float(binArea * targetDensity) — same expression and float
  // rounding as the CPU overflow loop.
  Kokkos::View<float*> d_bin_scaled_area;
  Kokkos::View<long long*> d_bin_nonplace;           // Bin::nonPlaceArea_
  Kokkos::View<long long*> d_bin_nonplace_unscaled;  // unscaled variant

  // Per-iteration scatter accumulators. Each contribution is truncated to
  // int64 before accumulation — the same arithmetic as
  // Bin::addInstPlacedAreaUnscaled / addFillerArea taking int64_t — which
  // makes the parallel sum order-independent and equal to the serial CPU
  // result. Stored as double (sums stay well below 2^53, so integer-exact)
  // because atomicAdd(double) is hardware-native on every supported arch;
  // 64-bit integer atomics measured catastrophically slow on this kernel.
  Kokkos::View<double*> d_bin_inst_area;
  Kokkos::View<double*> d_bin_filler_area;

  // Density-pipeline geometry (copied from BinGrid at context build).
  int bin_cnt_x = 0;
  int bin_cnt_y = 0;
  double bin_size_x = 0;
  double bin_size_y = 0;
  int grid_lx = 0;
  int grid_ly = 0;

  // NB-owned Poisson solver for the device-resident density loop. The
  // axis convention is swapped exactly like GpuFftBackend's solver member.
  std::unique_ptr<PoissonSolver> solver;
};

}  // namespace gpl
