// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// nesterovOp — Kokkos kernel launchers for the Nesterov loop.

#pragma once

#include "nesterovDeviceContext.h"  // for VecSlot

namespace gpl {

struct KokkosNesterovState;
struct KokkosDeviceState;

namespace nestop {

// K_gradCombine: updateGradients loop body replacement.
// Reads d_wl_grad, d_density_grad. Writes d_cur_sum_grads (or d_prev/next
// depending on which variant is called). Returns wireLengthGradSum and
// densityGradSum via parallel_reduce.
// `target` must be one of VecSlot::{Cur,Prev,Next}SumGrads.
void launchGradCombine(KokkosNesterovState& ns,
                       int n_cells,
                       float density_penalty,
                       float min_preconditioner,
                       VecSlot target,
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
                        VecSlot vec_a,
                        VecSlot vec_b);

// K_scatterToDeviceState: copy inst coords from NB arrays to DeviceState's
// d_inst_cx/cy using nbc_index mapping. Fillers (nbc_index == -1) skipped.
void launchScatterToDeviceState(const KokkosNesterovState& ns,
                                KokkosDeviceState& ds,
                                int n_cells,
                                VecSlot source);

// K_scatterGradsToNB: copy inst WL/density grads from DeviceState's
// d_inst_wl_grad/d_inst_density_grad to NB arrays. Fillers get 0 for WL.
void launchScatterGradsToNB(KokkosNesterovState& ns,
                            const KokkosDeviceState& ds,
                            int n_cells);

// K_updateInitialPrevSLPCoordi: initial prev SLP coord setup.
void launchUpdateInitialPrevSLPCoordi(KokkosNesterovState& ns,
                                      int n_cells,
                                      float initial_prev_coordi_update_coef);

}  // namespace nestop
}  // namespace gpl
