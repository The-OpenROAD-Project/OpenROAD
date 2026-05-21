// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// WA wirelength gradient — Kokkos kernel implementations (Phase 2).
//
// Five kernels mirroring DG-RePlAce gpl2/src/wirelengthOp.cu:
//   K1 updateNetBBox    — per-net bbox over CSR-listed pins
//   K2 computeAPosNeg   — per-pin shift-invariant exponentials
//   K3 computeBC        — per-net Σ A, Σ pin·A (no atomics — serial inner)
//   K4 computePinWAGrad — per-pin gradient (eq. 4.13), folds in net weight
//   K5 gatherInstGrad   — per-inst Σ pin-grad via inst→pin CSR
//
// Determinism: no atomics; per-net/per-inst outer parallelism with serial
// CSR inner loops matches the CPU summation order. Float results may differ
// from CPU by a few ULP (fastExp / division ordering) — acceptable per plan
// §I "결정성".

#include "wirelengthOp.h"

#include <Kokkos_Core.hpp>
#include <climits>

#include "deviceState_kokkos.h"

namespace gpl {
namespace wlop {

namespace {

// Match CPU NesterovBaseCommon::nbVars_.minWireLengthForceBar. Pinning here
// is fine — this is a static threshold for exp argument clamping and has
// been the same value across releases. If it ever becomes runtime-tunable
// in NesterovBaseVars, we'll need to plumb it through.
constexpr float kMinWireLengthForceBar = -300.0f;

// fastExp — same approximation as nesterovBase.cpp:4407 (10× squaring,
// linearization at 0). KOKKOS_INLINE_FUNCTION makes it device-callable.
// Reproducing the CPU body exactly (not std::exp) keeps GPU close enough to
// CPU for convergence-trajectory parity.
KOKKOS_INLINE_FUNCTION float fastExp(float exp)
{
  exp = 1.0f + exp / 1024.0f;
  for (int i = 0; i < 10; ++i) {
    exp *= exp;
  }
  return exp;
}

using ExecSpace = Kokkos::DefaultExecutionSpace;

}  // namespace

void launchUpdateNetBBox(KokkosDeviceState& ds, int n_nets)
{
  if (n_nets == 0) {
    return;
  }
  // Local refs so the lambda captures by value (no implicit `this`).
  auto d_net_pin_off = ds.d_net_pin_off;
  auto d_net_pin_idx = ds.d_net_pin_idx;
  auto d_pin_cx = ds.d_pin_cx;
  auto d_pin_cy = ds.d_pin_cy;
  auto d_net_lx = ds.d_net_lx;
  auto d_net_ly = ds.d_net_ly;
  auto d_net_ux = ds.d_net_ux;
  auto d_net_uy = ds.d_net_uy;

  Kokkos::parallel_for(
      "wlop_K1_net_bbox",
      Kokkos::RangePolicy<ExecSpace>(0, n_nets),
      KOKKOS_LAMBDA(const int i) {
        int lx = INT_MAX;
        int ly = INT_MAX;
        int ux = INT_MIN;
        int uy = INT_MIN;
        const int begin = d_net_pin_off(i);
        const int end = d_net_pin_off(i + 1);
        for (int j = begin; j < end; ++j) {
          const int p = d_net_pin_idx(j);
          const int x = d_pin_cx(p);
          const int y = d_pin_cy(p);
          if (x < lx) {
            lx = x;
          }
          if (y < ly) {
            ly = y;
          }
          if (x > ux) {
            ux = x;
          }
          if (y > uy) {
            uy = y;
          }
        }
        d_net_lx(i) = lx;
        d_net_ly(i) = ly;
        d_net_ux(i) = ux;
        d_net_uy(i) = uy;
      });
}

void launchComputeAPosNeg(KokkosDeviceState& ds,
                          int n_pins,
                          float wlCoefX,
                          float wlCoefY)
{
  if (n_pins == 0) {
    return;
  }
  auto d_pin_cx = ds.d_pin_cx;
  auto d_pin_cy = ds.d_pin_cy;
  auto d_pin_net_id = ds.d_pin_net_id;
  auto d_net_lx = ds.d_net_lx;
  auto d_net_ly = ds.d_net_ly;
  auto d_net_ux = ds.d_net_ux;
  auto d_net_uy = ds.d_net_uy;
  auto d_pin_a_pos_x = ds.d_pin_a_pos_x;
  auto d_pin_a_neg_x = ds.d_pin_a_neg_x;
  auto d_pin_a_pos_y = ds.d_pin_a_pos_y;
  auto d_pin_a_neg_y = ds.d_pin_a_neg_y;

  Kokkos::parallel_for(
      "wlop_K2_a_pos_neg",
      Kokkos::RangePolicy<ExecSpace>(0, n_pins),
      KOKKOS_LAMBDA(const int p) {
        const int n = d_pin_net_id(p);
        if (n < 0) {
          // Pin not attached to any net (defensive — shouldn't happen in
          // practice). Zero out so K3 / K4 produce no contribution.
          d_pin_a_pos_x(p) = 0.0f;
          d_pin_a_neg_x(p) = 0.0f;
          d_pin_a_pos_y(p) = 0.0f;
          d_pin_a_neg_y(p) = 0.0f;
          return;
        }
        const float px = static_cast<float>(d_pin_cx(p));
        const float py = static_cast<float>(d_pin_cy(p));
        // CPU computes: expMinX = (net.lx - pin.cx) * coef, then if larger
        // than minWireLengthForceBar, sets minExpSumX = fastExp(expMinX).
        const float exp_min_x
            = (static_cast<float>(d_net_lx(n)) - px) * wlCoefX;
        const float exp_max_x
            = (px - static_cast<float>(d_net_ux(n))) * wlCoefX;
        const float exp_min_y
            = (static_cast<float>(d_net_ly(n)) - py) * wlCoefY;
        const float exp_max_y
            = (py - static_cast<float>(d_net_uy(n))) * wlCoefY;
        d_pin_a_neg_x(p)
            = exp_min_x > kMinWireLengthForceBar ? fastExp(exp_min_x) : 0.0f;
        d_pin_a_pos_x(p)
            = exp_max_x > kMinWireLengthForceBar ? fastExp(exp_max_x) : 0.0f;
        d_pin_a_neg_y(p)
            = exp_min_y > kMinWireLengthForceBar ? fastExp(exp_min_y) : 0.0f;
        d_pin_a_pos_y(p)
            = exp_max_y > kMinWireLengthForceBar ? fastExp(exp_max_y) : 0.0f;
      });
}

void launchComputeBC(KokkosDeviceState& ds, int n_nets)
{
  if (n_nets == 0) {
    return;
  }
  auto d_net_pin_off = ds.d_net_pin_off;
  auto d_net_pin_idx = ds.d_net_pin_idx;
  auto d_pin_cx = ds.d_pin_cx;
  auto d_pin_cy = ds.d_pin_cy;
  auto d_pin_a_pos_x = ds.d_pin_a_pos_x;
  auto d_pin_a_neg_x = ds.d_pin_a_neg_x;
  auto d_pin_a_pos_y = ds.d_pin_a_pos_y;
  auto d_pin_a_neg_y = ds.d_pin_a_neg_y;
  auto d_net_b_pos_x = ds.d_net_b_pos_x;
  auto d_net_b_neg_x = ds.d_net_b_neg_x;
  auto d_net_b_pos_y = ds.d_net_b_pos_y;
  auto d_net_b_neg_y = ds.d_net_b_neg_y;
  auto d_net_c_pos_x = ds.d_net_c_pos_x;
  auto d_net_c_neg_x = ds.d_net_c_neg_x;
  auto d_net_c_pos_y = ds.d_net_c_pos_y;
  auto d_net_c_neg_y = ds.d_net_c_neg_y;

  Kokkos::parallel_for(
      "wlop_K3_bc",
      Kokkos::RangePolicy<ExecSpace>(0, n_nets),
      KOKKOS_LAMBDA(const int n) {
        float bpx = 0, bnx = 0, bpy = 0, bny = 0;
        float cpx = 0, cnx = 0, cpy = 0, cny = 0;
        const int begin = d_net_pin_off(n);
        const int end = d_net_pin_off(n + 1);
        // Serial CSR inner — same order as CPU's `for (gPin :
        // gNet->getGPins())` loop in updateWireLengthForceWA. Keeps float
        // summation matching.
        for (int j = begin; j < end; ++j) {
          const int p = d_net_pin_idx(j);
          const float px = static_cast<float>(d_pin_cx(p));
          const float py = static_cast<float>(d_pin_cy(p));
          const float apx = d_pin_a_pos_x(p);
          const float anx = d_pin_a_neg_x(p);
          const float apy = d_pin_a_pos_y(p);
          const float any = d_pin_a_neg_y(p);
          bpx += apx;
          bnx += anx;
          bpy += apy;
          bny += any;
          cpx += px * apx;
          cnx += px * anx;
          cpy += py * apy;
          cny += py * any;
        }
        d_net_b_pos_x(n) = bpx;
        d_net_b_neg_x(n) = bnx;
        d_net_b_pos_y(n) = bpy;
        d_net_b_neg_y(n) = bny;
        d_net_c_pos_x(n) = cpx;
        d_net_c_neg_x(n) = cnx;
        d_net_c_pos_y(n) = cpy;
        d_net_c_neg_y(n) = cny;
      });
}

void launchComputePinWAGrad(KokkosDeviceState& ds,
                            int n_pins,
                            float wlCoefX,
                            float wlCoefY)
{
  if (n_pins == 0) {
    return;
  }
  auto d_pin_cx = ds.d_pin_cx;
  auto d_pin_cy = ds.d_pin_cy;
  auto d_pin_net_id = ds.d_pin_net_id;
  auto d_pin_a_pos_x = ds.d_pin_a_pos_x;
  auto d_pin_a_neg_x = ds.d_pin_a_neg_x;
  auto d_pin_a_pos_y = ds.d_pin_a_pos_y;
  auto d_pin_a_neg_y = ds.d_pin_a_neg_y;
  auto d_net_b_pos_x = ds.d_net_b_pos_x;
  auto d_net_b_neg_x = ds.d_net_b_neg_x;
  auto d_net_b_pos_y = ds.d_net_b_pos_y;
  auto d_net_b_neg_y = ds.d_net_b_neg_y;
  auto d_net_c_pos_x = ds.d_net_c_pos_x;
  auto d_net_c_neg_x = ds.d_net_c_neg_x;
  auto d_net_c_pos_y = ds.d_net_c_pos_y;
  auto d_net_c_neg_y = ds.d_net_c_neg_y;
  auto d_net_weight = ds.d_net_weight;
  auto d_pin_grad_x = ds.d_pin_grad_x;
  auto d_pin_grad_y = ds.d_pin_grad_y;

  Kokkos::parallel_for(
      "wlop_K4_pin_wa_grad",
      Kokkos::RangePolicy<ExecSpace>(0, n_pins),
      KOKKOS_LAMBDA(const int p) {
        const int n = d_pin_net_id(p);
        if (n < 0) {
          d_pin_grad_x(p) = 0.0f;
          d_pin_grad_y(p) = 0.0f;
          return;
        }
        const float px = static_cast<float>(d_pin_cx(p));
        const float py = static_cast<float>(d_pin_cy(p));
        const float anx = d_pin_a_neg_x(p);
        const float apx = d_pin_a_pos_x(p);
        const float any = d_pin_a_neg_y(p);
        const float apy = d_pin_a_pos_y(p);
        const float bnx = d_net_b_neg_x(n);
        const float bpx = d_net_b_pos_x(n);
        const float bny = d_net_b_neg_y(n);
        const float bpy = d_net_b_pos_y(n);
        const float cnx = d_net_c_neg_x(n);
        const float cpx = d_net_c_pos_x(n);
        const float cny = d_net_c_neg_y(n);
        const float cpy = d_net_c_pos_y(n);
        const float w = d_net_weight(n);

        // Eq 4.13 from JingWei's thesis, same as CPU
        // getWireLengthGradientPinWA. Min-X branch uses A_neg / B_neg / C_neg;
        // Max-X uses pos counterparts. CPU skips the branch when hasMinExpSumX
        // is false (i.e., the pin's exp arg fell below threshold and minExpSumX
        // was never set, so it's still 0). We mirror with `anx > 0` / `apx > 0`
        // guards — same effect.
        float grad_min_x = 0;
        if (anx > 0.0f && bnx > 0.0f) {
          grad_min_x
              = (bnx * (anx * (1.0f - wlCoefX * px)) + wlCoefX * anx * cnx)
                / (bnx * bnx);
        }
        float grad_max_x = 0;
        if (apx > 0.0f && bpx > 0.0f) {
          grad_max_x
              = (bpx * (apx * (1.0f + wlCoefX * px)) - wlCoefX * apx * cpx)
                / (bpx * bpx);
        }
        float grad_min_y = 0;
        if (any > 0.0f && bny > 0.0f) {
          grad_min_y
              = (bny * (any * (1.0f - wlCoefY * py)) + wlCoefY * any * cny)
                / (bny * bny);
        }
        float grad_max_y = 0;
        if (apy > 0.0f && bpy > 0.0f) {
          grad_max_y
              = (bpy * (apy * (1.0f + wlCoefY * py)) - wlCoefY * apy * cpy)
                / (bpy * bpy);
        }
        // Net weight folded in here so K5 is a plain sum.
        d_pin_grad_x(p) = (grad_min_x - grad_max_x) * w;
        d_pin_grad_y(p) = (grad_min_y - grad_max_y) * w;
      });
}

void launchGatherInstGrad(KokkosDeviceState& ds, int n_insts)
{
  if (n_insts == 0) {
    return;
  }
  auto d_inst_pin_off = ds.d_inst_pin_off;
  auto d_inst_pin_idx = ds.d_inst_pin_idx;
  auto d_pin_grad_x = ds.d_pin_grad_x;
  auto d_pin_grad_y = ds.d_pin_grad_y;
  auto d_inst_wl_grad_x = ds.d_inst_wl_grad_x;
  auto d_inst_wl_grad_y = ds.d_inst_wl_grad_y;

  Kokkos::parallel_for(
      "wlop_K5_gather_inst",
      Kokkos::RangePolicy<ExecSpace>(0, n_insts),
      KOKKOS_LAMBDA(const int i) {
        float gx = 0.0f;
        float gy = 0.0f;
        const int begin = d_inst_pin_off(i);
        const int end = d_inst_pin_off(i + 1);
        // Serial — matches CPU getWireLengthGradientWA(gCell) loop order.
        for (int j = begin; j < end; ++j) {
          const int p = d_inst_pin_idx(j);
          gx += d_pin_grad_x(p);
          gy += d_pin_grad_y(p);
        }
        d_inst_wl_grad_x(i) = gx;
        d_inst_wl_grad_y(i) = gy;
      });
}

}  // namespace wlop
}  // namespace gpl
