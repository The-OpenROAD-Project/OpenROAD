// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Nesterov loop kernels. Replaces per-cell CPU loops in
// NesterovBase::updateGradients (loop body), nesterovUpdateCoordinates,
// getDistance, and scatter/gather between NB and DeviceState indices.

#include "nesterovOp.h"

#include <Kokkos_Core.hpp>
#include <cmath>

#include "deviceState_kokkos.h"
#include "nesterovDeviceState.h"

namespace gpl {
namespace nestop {

namespace {
using ExecSpace = Kokkos::DefaultExecutionSpace;

// Helper: select x/y pair from NesterovState by vector ID.
// Returns View references for the requested vector.
struct VecPair
{
  Kokkos::View<float*> x;
  Kokkos::View<float*> y;
};

// Kokkos::View has shallow-copy semantics (the const applies to the View
// handle, not the underlying device memory), so a single const& overload
// serves both read-only and writing callers without a const_cast.
VecPair getVec(const KokkosNesterovState& ns, SlpSlot vec_id)
{
  switch (vec_id) {
    case SlpSlot::Cur:
      return {ns.d_cur_slp_x, ns.d_cur_slp_y};
    case SlpSlot::Prev:
      return {ns.d_prev_slp_x, ns.d_prev_slp_y};
    case SlpSlot::Next:
      return {ns.d_next_slp_x, ns.d_next_slp_y};
  }
  Kokkos::abort("getVec: invalid SlpSlot");
  return {ns.d_next_slp_x, ns.d_next_slp_y};
}

VecPair getVec(const KokkosNesterovState& ns, SumGradSlot vec_id)
{
  switch (vec_id) {
    case SumGradSlot::Cur:
      return {ns.d_cur_sum_grads_x, ns.d_cur_sum_grads_y};
    case SumGradSlot::Prev:
      return {ns.d_prev_sum_grads_x, ns.d_prev_sum_grads_y};
    case SumGradSlot::Next:
      return {ns.d_next_sum_grads_x, ns.d_next_sum_grads_y};
  }
  Kokkos::abort("getVec: invalid SumGradSlot");
  return {ns.d_next_sum_grads_x, ns.d_next_sum_grads_y};
}

}  // namespace

void launchGradCombine(KokkosNesterovState& ns,
                       int n_cells,
                       float density_penalty,
                       float min_preconditioner,
                       SumGradSlot target,
                       float& wl_grad_sum,
                       float& density_grad_sum)
{
  if (n_cells == 0) {
    return;
  }

  auto d_wl_x = ns.d_wl_grad_x;
  auto d_wl_y = ns.d_wl_grad_y;
  auto d_dens_x = ns.d_density_grad_x;
  auto d_dens_y = ns.d_density_grad_y;
  auto d_num_pins = ns.d_num_pins;
  auto d_area = ns.d_area;
  auto d_locked = ns.d_locked;

  VecPair out = getVec(ns, target);
  auto d_out_x = out.x;
  auto d_out_y = out.y;

  const float penalty = density_penalty;
  const float min_pre = min_preconditioner;

  // Two-pass: first parallel_for writes sumGrads, then two reductions.
  Kokkos::parallel_for(
      "nestop_grad_combine",
      Kokkos::RangePolicy<ExecSpace>(0, n_cells),
      KOKKOS_LAMBDA(const int i) {
        if (d_locked(i)) {
          d_out_x(i) = 0.0f;
          d_out_y(i) = 0.0f;
          return;
        }
        const float wx = d_wl_x(i);
        const float wy = d_wl_y(i);
        const float dx = d_dens_x(i);
        const float dy = d_dens_y(i);

        float sx = wx + penalty * dx;
        float sy = wy + penalty * dy;

        const float np = static_cast<float>(d_num_pins(i));
        const float a = d_area(i);
        float pre = np + penalty * a;
        if (pre < min_pre) {
          pre = min_pre;
        }
        d_out_x(i) = sx / pre;
        d_out_y(i) = sy / pre;
      });

  // Reduction: wl grad sum.
  float wl_sum = 0;
  Kokkos::parallel_reduce(
      "nestop_wl_sum",
      Kokkos::RangePolicy<ExecSpace>(0, n_cells),
      KOKKOS_LAMBDA(const int i, float& local) {
        local += Kokkos::fabs(d_wl_x(i)) + Kokkos::fabs(d_wl_y(i));
      },
      wl_sum);

  // Reduction: density grad sum.
  float dens_sum = 0;
  Kokkos::parallel_reduce(
      "nestop_dens_sum",
      Kokkos::RangePolicy<ExecSpace>(0, n_cells),
      KOKKOS_LAMBDA(const int i, float& local) {
        local += Kokkos::fabs(d_dens_x(i)) + Kokkos::fabs(d_dens_y(i));
      },
      dens_sum);

  wl_grad_sum = wl_sum;
  density_grad_sum = dens_sum;
}

void launchNesterovCoordUpdate(KokkosNesterovState& ns,
                               int n_cells,
                               float step_length,
                               float coeff)
{
  if (n_cells == 0) {
    return;
  }

  auto d_cur_slp_x = ns.d_cur_slp_x;
  auto d_cur_slp_y = ns.d_cur_slp_y;
  auto d_cur_x = ns.d_cur_x;
  auto d_cur_y = ns.d_cur_y;
  auto d_sum_x = ns.d_cur_sum_grads_x;
  auto d_sum_y = ns.d_cur_sum_grads_y;
  auto d_next_x = ns.d_next_x;
  auto d_next_y = ns.d_next_y;
  auto d_next_slp_x = ns.d_next_slp_x;
  auto d_next_slp_y = ns.d_next_slp_y;
  auto d_locked = ns.d_locked;
  auto d_clamp_lx = ns.d_clamp_lx;
  auto d_clamp_ly = ns.d_clamp_ly;
  auto d_clamp_ux = ns.d_clamp_ux;
  auto d_clamp_uy = ns.d_clamp_uy;

  const float step = step_length;
  const float c = coeff;

  Kokkos::parallel_for(
      "nestop_coord_update",
      Kokkos::RangePolicy<ExecSpace>(0, n_cells),
      KOKKOS_LAMBDA(const int i) {
        if (d_locked(i)) {
          d_next_x(i) = d_cur_x(i);
          d_next_y(i) = d_cur_y(i);
          d_next_slp_x(i) = d_cur_slp_x(i);
          d_next_slp_y(i) = d_cur_slp_y(i);
          return;
        }
        // Gradient descent.
        float nx = d_cur_slp_x(i) + step * d_sum_x(i);
        float ny = d_cur_slp_y(i) + step * d_sum_y(i);

        // Nesterov momentum.
        float nsx = nx + c * (nx - d_cur_x(i));
        float nsy = ny + c * (ny - d_cur_y(i));

        // Clamp to density layout bounds.
        const float lx = d_clamp_lx(i);
        const float ly = d_clamp_ly(i);
        const float ux = d_clamp_ux(i);
        const float uy = d_clamp_uy(i);
        if (nx < lx) {
          nx = lx;
        }
        if (nx > ux) {
          nx = ux;
        }
        if (ny < ly) {
          ny = ly;
        }
        if (ny > uy) {
          ny = uy;
        }
        if (nsx < lx) {
          nsx = lx;
        }
        if (nsx > ux) {
          nsx = ux;
        }
        if (nsy < ly) {
          nsy = ly;
        }
        if (nsy > uy) {
          nsy = uy;
        }

        d_next_x(i) = nx;
        d_next_y(i) = ny;
        d_next_slp_x(i) = nsx;
        d_next_slp_y(i) = nsy;
      });
}

namespace {
// Template impl shared by the two launchGetDistance overloads — the body is
// identical, only the Slot type differs (and `getVec` dispatches accordingly).
template <typename Slot>
float launchGetDistanceImpl(const KokkosNesterovState& ns,
                            int n_cells,
                            Slot vec_a,
                            Slot vec_b)
{
  if (n_cells == 0) {
    return 0.0f;
  }
  VecPair a = getVec(ns, vec_a);
  VecPair b = getVec(ns, vec_b);
  auto ax = a.x;
  auto ay = a.y;
  auto bx = b.x;
  auto by = b.y;

  float sum = 0;
  Kokkos::parallel_reduce(
      "nestop_distance",
      Kokkos::RangePolicy<ExecSpace>(0, n_cells),
      KOKKOS_LAMBDA(const int i, float& local) {
        const float dxx = ax(i) - bx(i);
        const float dyy = ay(i) - by(i);
        local += dxx * dxx + dyy * dyy;
      },
      sum);

  return std::sqrt(sum / (2.0f * n_cells));
}
}  // namespace

float launchGetDistance(const KokkosNesterovState& ns,
                        int n_cells,
                        SlpSlot vec_a,
                        SlpSlot vec_b)
{
  return launchGetDistanceImpl(ns, n_cells, vec_a, vec_b);
}

float launchGetDistance(const KokkosNesterovState& ns,
                        int n_cells,
                        SumGradSlot vec_a,
                        SumGradSlot vec_b)
{
  return launchGetDistanceImpl(ns, n_cells, vec_a, vec_b);
}

void launchScatterToDeviceState(const KokkosNesterovState& ns,
                                KokkosDeviceState& ds,
                                int n_cells,
                                SlpSlot source)
{
  if (n_cells == 0) {
    return;
  }
  VecPair src = getVec(ns, source);
  auto src_x = src.x;
  auto src_y = src.y;
  auto d_nbc_index = ns.d_nbc_index;
  auto d_inst_cx = ds.d_inst_cx;
  auto d_inst_cy = ds.d_inst_cy;

  Kokkos::parallel_for(
      "nestop_scatter_to_ds",
      Kokkos::RangePolicy<ExecSpace>(0, n_cells),
      KOKKOS_LAMBDA(const int i) {
        const int nbc_idx = d_nbc_index(i);
        if (nbc_idx >= 0) {
          d_inst_cx(nbc_idx) = static_cast<int>(src_x(i));
          d_inst_cy(nbc_idx) = static_cast<int>(src_y(i));
        }
      });
}

void launchScatterGradsToNB(KokkosNesterovState& ns,
                            const KokkosDeviceState& ds,
                            int n_cells)
{
  if (n_cells == 0) {
    return;
  }
  auto d_nbc_index = ns.d_nbc_index;
  auto d_nb_wl_x = ns.d_wl_grad_x;
  auto d_nb_wl_y = ns.d_wl_grad_y;
  auto d_inst_wl_x = ds.d_inst_wl_grad_x;
  auto d_inst_wl_y = ds.d_inst_wl_grad_y;

  Kokkos::parallel_for(
      "nestop_scatter_grads_nb",
      Kokkos::RangePolicy<ExecSpace>(0, n_cells),
      KOKKOS_LAMBDA(const int i) {
        const int nbc_idx = d_nbc_index(i);
        if (nbc_idx >= 0) {
          d_nb_wl_x(i) = d_inst_wl_x(nbc_idx);
          d_nb_wl_y(i) = d_inst_wl_y(nbc_idx);
        } else {
          d_nb_wl_x(i) = 0.0f;
          d_nb_wl_y(i) = 0.0f;
        }
      });
}

void launchUpdateInitialPrevSLPCoordi(KokkosNesterovState& ns,
                                      int n_cells,
                                      float initial_prev_coordi_update_coef)
{
  if (n_cells == 0) {
    return;
  }
  auto d_cur_slp_x = ns.d_cur_slp_x;
  auto d_cur_slp_y = ns.d_cur_slp_y;
  auto d_cur_sum_x = ns.d_cur_sum_grads_x;
  auto d_cur_sum_y = ns.d_cur_sum_grads_y;
  auto d_prev_slp_x = ns.d_prev_slp_x;
  auto d_prev_slp_y = ns.d_prev_slp_y;
  auto d_locked = ns.d_locked;
  auto d_clamp_lx = ns.d_clamp_lx;
  auto d_clamp_ly = ns.d_clamp_ly;
  auto d_clamp_ux = ns.d_clamp_ux;
  auto d_clamp_uy = ns.d_clamp_uy;

  const float coef = initial_prev_coordi_update_coef;

  Kokkos::parallel_for(
      "nestop_init_prev_slp",
      Kokkos::RangePolicy<ExecSpace>(0, n_cells),
      KOKKOS_LAMBDA(const int i) {
        if (d_locked(i)) {
          d_prev_slp_x(i) = d_cur_slp_x(i);
          d_prev_slp_y(i) = d_cur_slp_y(i);
          return;
        }
        float px = d_cur_slp_x(i) - coef * d_cur_sum_x(i);
        float py = d_cur_slp_y(i) - coef * d_cur_sum_y(i);

        const float lx = d_clamp_lx(i);
        const float ly = d_clamp_ly(i);
        const float ux = d_clamp_ux(i);
        const float uy = d_clamp_uy(i);
        if (px < lx) {
          px = lx;
        }
        if (px > ux) {
          px = ux;
        }
        if (py < ly) {
          py = ly;
        }
        if (py > uy) {
          py = uy;
        }

        d_prev_slp_x(i) = px;
        d_prev_slp_y(i) = py;
      });
}

}  // namespace nestop
}  // namespace gpl
