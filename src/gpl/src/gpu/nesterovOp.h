// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// nesterovOp — Kokkos kernel launchers for Phase 4 Nesterov loop.
// Kokkos-laden header — include only from CUDA/HIP TUs.

#pragma once

namespace gpl {

struct KokkosNesterovState;
struct KokkosDeviceState;

namespace nestop {

// K_gradCombine: updateGradients loop body replacement.
// Reads d_wl_grad, d_density_grad. Writes d_cur_sum_grads (or d_prev/next
// depending on which variant is called). Returns wireLengthGradSum and
// densityGradSum via parallel_reduce.
// `target`: 0 = cur, 1 = prev, 2 = next (selects which sum_grads to write)
void launchGradCombine(KokkosNesterovState& ns,
                       int n_cells,
                       float density_penalty,
                       float min_preconditioner,
                       int target,
                       float& wl_grad_sum,
                       float& density_grad_sum);

// K_nesterovCoordUpdate: gradient descent + Nesterov momentum + clamp.
// Writes d_next, d_next_slp from d_cur_slp, d_cur, d_cur_sum_grads.
void launchNesterovCoordUpdate(KokkosNesterovState& ns,
                               int n_cells,
                               float step_length,
                               float coeff);

// K_getDistance: RMS norm of difference between two per-cell vectors.
// Returns sqrt(sum_of_squares / (2 * n_cells)).
float launchGetDistance(const KokkosNesterovState& ns,
                        int n_cells,
                        int vec_a,
                        int vec_b);

// K_scatterToDeviceState: copy inst coords from NB arrays to DeviceState's
// d_inst_cx/cy using nbc_index mapping. Fillers (nbc_index == -1) skipped.
void launchScatterToDeviceState(const KokkosNesterovState& ns,
                                KokkosDeviceState& ds,
                                int n_cells,
                                int source);

// K_scatterGradsToNB: copy inst WL/density grads from DeviceState's
// d_inst_wl_grad/d_inst_density_grad to NB arrays. Fillers get 0 for WL.
void launchScatterGradsToNB(KokkosNesterovState& ns,
                            const KokkosDeviceState& ds,
                            int n_cells);

// K_updateInitialPrevSLPCoordi: initial prev SLP coord setup.
void launchUpdateInitialPrevSLPCoordi(KokkosNesterovState& ns,
                                      int n_cells,
                                      float initial_prev_coordi_update_coef);

// Vector ID constants for launchGetDistance / launchScatterToDeviceState.
constexpr int kVecCurSLP = 0;
constexpr int kVecPrevSLP = 1;
constexpr int kVecNextSLP = 2;
constexpr int kVecCurSumGrads = 3;
constexpr int kVecPrevSumGrads = 4;
constexpr int kVecNextSumGrads = 5;

}  // namespace nestop
}  // namespace gpl
