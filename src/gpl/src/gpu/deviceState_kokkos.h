// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Kokkos-laden private header for DeviceState. Defines KokkosDeviceState —
// the struct of device Views holding the gpl device-resident pool. Only
// include from translation units that are compiled as CUDA/HIP TUs
// (gpu/deviceState.cpp, gpu/gpuHpwlBackend.cpp, and future GPU backends),
// listed in src/gpl/CMakeLists.txt's source-language section.
//
// Including this from a plain CXX TU would pull in <Kokkos_Core.hpp>, which
// expects __CUDACC__ when KOKKOS_ENABLE_CUDA is defined.

#pragma once

#include <Kokkos_Core.hpp>

namespace gpl {

struct KokkosDeviceState
{
  // Inst-level (size = num_insts):
  Kokkos::View<int*> d_inst_cx;
  Kokkos::View<int*> d_inst_cy;
  // Host mirrors for staging Nesterov-update output (until Phase 4).
  Kokkos::View<int*>::HostMirror h_inst_cx;
  Kokkos::View<int*>::HostMirror h_inst_cy;

  // Pin-level (size = num_pins):
  Kokkos::View<int*> d_pin_offset_cx;  // const, set once
  Kokkos::View<int*> d_pin_offset_cy;  // const, set once
  Kokkos::View<int*> d_pin_inst_id;    // const, set once (index into d_inst_*)
  Kokkos::View<int*> d_pin_net_id;     // const, set once (index into d_net_*)
  Kokkos::View<int*> d_pin_cx;         // updated by updatePinLocations()
  Kokkos::View<int*> d_pin_cy;         // updated by updatePinLocations()

  // Net→pin CSR (size = num_nets + 1):
  Kokkos::View<int*> d_net_pin_off;
  // Per-net pin indices (size = total_pins, CSR data).
  Kokkos::View<int*> d_net_pin_idx;

  // ---- Phase 2: WA wirelength gradient ----
  //
  // Per-pin WA exponentials (K2 computeAPosNeg output, K3/K4 input).
  // a_pos = fastExp((pin - net.ub) * coef), a_neg = fastExp((net.lb - pin) *
  // coef). Threshold-clamped to 0 for pins where exp arg <
  // minWireLengthForceBar.
  Kokkos::View<float*> d_pin_a_pos_x;
  Kokkos::View<float*> d_pin_a_neg_x;
  Kokkos::View<float*> d_pin_a_pos_y;
  Kokkos::View<float*> d_pin_a_neg_y;

  // Per-pin gradient (K4 output, K5 input). Already net-weight-multiplied.
  Kokkos::View<float*> d_pin_grad_x;
  Kokkos::View<float*> d_pin_grad_y;

  // Per-net WA bounding box (K1 output, K2 input).
  Kokkos::View<int*> d_net_lx;
  Kokkos::View<int*> d_net_ly;
  Kokkos::View<int*> d_net_ux;
  Kokkos::View<int*> d_net_uy;

  // Per-net B = Σ a_pos / Σ a_neg ; C = Σ pin * a_pos / Σ pin * a_neg.
  // Naming convention matches CPU: pos ≡ waExpMaxSum, neg ≡ waExpMinSum.
  Kokkos::View<float*> d_net_b_pos_x;
  Kokkos::View<float*> d_net_b_neg_x;
  Kokkos::View<float*> d_net_b_pos_y;
  Kokkos::View<float*> d_net_b_neg_y;
  Kokkos::View<float*> d_net_c_pos_x;
  Kokkos::View<float*> d_net_c_neg_x;
  Kokkos::View<float*> d_net_c_pos_y;
  Kokkos::View<float*> d_net_c_neg_y;

  // Per-net total weight (timing/custom-net weight). Static for Phase 2 — see
  // DeviceState::refreshNetWeights() TODO.
  Kokkos::View<float*> d_net_weight;

  // Inst→pin CSR (offsets size = num_insts + 1). I/O pins (inst_id == -1)
  // are not in this CSR.
  Kokkos::View<int*> d_inst_pin_off;
  Kokkos::View<int*> d_inst_pin_idx;

  // Per-inst WA wirelength gradient (K5 output, host-readable mirror).
  Kokkos::View<float*> d_inst_wl_grad_x;
  Kokkos::View<float*> d_inst_wl_grad_y;
  Kokkos::View<float*>::HostMirror h_inst_wl_grad_x;
  Kokkos::View<float*>::HostMirror h_inst_wl_grad_y;

  // ---- Phase 3: density gradient (FFT field Views + per-inst gather) ----
  //
  // Bin grid Views (size = binCntX × binCntY, row-major [x * binCntY + y]).
  // Owned here; GpuFftBackend borrows them (same pattern as Phase 1 pin
  // coords). The solver's axis convention differs from gpl's — the gather
  // kernel applies the axis swap + 0.5× scale inline.
  Kokkos::View<float*> d_bin_density;  // FFT input (scatter result)
  Kokkos::View<float*> d_bin_phi;      // FFT output (electrostatic potential)
  Kokkos::View<float*> d_bin_elec_x;   // FFT output (solver X = gpl Y)
  Kokkos::View<float*> d_bin_elec_y;   // FFT output (solver Y = gpl X)
  Kokkos::View<float*>::HostMirror h_bin_density;
  Kokkos::View<float*>::HostMirror h_bin_phi;
  Kokkos::View<float*>::HostMirror h_bin_elec_x;
  Kokkos::View<float*>::HostMirror h_bin_elec_y;

  // Per-inst density params (static for main loop, set once from initDensity1).
  // Half-sizes of the density bounding box: dLx = dCx - half_dx, etc.
  Kokkos::View<int*> d_inst_density_half_dx;
  Kokkos::View<int*> d_inst_density_half_dy;
  Kokkos::View<float*> d_inst_density_scale;

  // Per-inst density gradient (gather output, host-readable mirror).
  Kokkos::View<float*> d_inst_density_grad_x;
  Kokkos::View<float*> d_inst_density_grad_y;
  Kokkos::View<float*>::HostMirror h_inst_density_grad_x;
  Kokkos::View<float*>::HostMirror h_inst_density_grad_y;
};

}  // namespace gpl
