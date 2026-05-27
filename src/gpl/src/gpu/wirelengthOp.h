// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// wlop — Kokkos kernel launchers for the WA wirelength gradient pipeline.
// The five kernels are 1:1 with DG-RePlAce gpl2/src/wirelengthOp.cu
// (updateNetBBox / computeAPosNeg / computeBC / computePinWAGrad /
// gatherInstGrad).
//
// Kokkos-laden header — include only from CUDA/HIP TUs.

#pragma once

namespace gpl {

struct KokkosDeviceState;

namespace wlop {

// K1: per-net bbox over CSR-listed pins.
//
// Reads:  ds.d_net_pin_off, ds.d_net_pin_idx, ds.d_pin_cx, ds.d_pin_cy
// Writes: ds.d_net_lx, ds.d_net_ly, ds.d_net_ux, ds.d_net_uy
void launchUpdateNetBBox(KokkosDeviceState& ds, int n_nets);

// K2: per-pin shift-invariant WA exponentials.
//   a_neg = fastExp((net.lb - pin) * coef)   ≡ CPU minExpSumX/Y
//   a_pos = fastExp((pin - net.ub) * coef)   ≡ CPU maxExpSumX/Y
// Clamped to 0 if exp arg ≤ minWireLengthForceBar.
//
// Reads:  ds.d_pin_cx/cy, ds.d_pin_net_id, ds.d_net_l/u_x/y
// Writes: ds.d_pin_a_pos/neg_x/y
void launchComputeAPosNeg(KokkosDeviceState& ds,
                          int n_pins,
                          float wlCoefX,
                          float wlCoefY);

// K3: per-net B,C reductions over CSR.
//   B_neg = Σ a_neg ;        B_pos = Σ a_pos
//   C_neg = Σ pin · a_neg ;  C_pos = Σ pin · a_pos
//
// Reads:  ds.d_net_pin_off, ds.d_net_pin_idx, ds.d_pin_cx/cy, ds.d_pin_a_*
// Writes: ds.d_net_b_*, ds.d_net_c_*
void launchComputeBC(KokkosDeviceState& ds, int n_nets);

// K4: per-pin WA gradient (eq. 4.13 of JingWei thesis). Net weight folded
// into the result, so K5 is a plain sum.
//
// Reads:  ds.d_pin_a_*, ds.d_net_b_*, ds.d_net_c_*, ds.d_pin_net_id,
//         ds.d_pin_cx/cy, ds.d_net_weight
// Writes: ds.d_pin_grad_x, ds.d_pin_grad_y
void launchComputePinWAGrad(KokkosDeviceState& ds,
                            int n_pins,
                            float wlCoefX,
                            float wlCoefY);

// K5: per-inst gather of pin gradients via inst→pin CSR. I/O pins (not in
// the CSR) are skipped naturally.
//
// Reads:  ds.d_inst_pin_off, ds.d_inst_pin_idx, ds.d_pin_grad_*
// Writes: ds.d_inst_wl_grad_x, ds.d_inst_wl_grad_y
void launchGatherInstGrad(KokkosDeviceState& ds, int n_insts);

}  // namespace wlop
}  // namespace gpl
