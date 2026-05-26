// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// NesterovBase-level device arrays (Phase 4). Parallel to nb_gcells_
// (inst + filler cells). Owned by NesterovBase; distinct from the
// NesterovBaseCommon-level DeviceState which holds inst-only data
// (pin/net CSRs, WA gradient Views, etc.).
//
// Kokkos-laden — include only from CUDA/HIP TUs.

#pragma once

#include <Kokkos_Core.hpp>

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
};

}  // namespace gpl
