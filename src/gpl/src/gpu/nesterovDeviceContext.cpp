// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "nesterovDeviceContext.h"

#include <Kokkos_Core.hpp>
#include <algorithm>
#include <cstddef>
#include <memory>
#include <vector>

#include "deviceState.h"
#include "deviceState_kokkos.h"
#include "gpuRuntime.h"
#include "nesterovBase.h"
#include "nesterovDeviceState.h"
#include "nesterovOp.h"

namespace gpl {

NesterovDeviceContext::NesterovDeviceContext(
    const std::vector<GCellHandle>& nb_gcells,
    NesterovBaseCommon* nbc,
    const BinGrid& bg)
    : kokkos_(std::make_unique<KokkosNesterovState>())
{
  ensureKokkosInitialized();

  num_cells_ = static_cast<int>(nb_gcells.size());
  auto& s = *kokkos_;

  // Allocate all Views.
  const size_t n = static_cast<size_t>(num_cells_);

  s.d_cur_slp_x = Kokkos::View<float*>("nb_cur_slp_x", n);
  s.d_cur_slp_y = Kokkos::View<float*>("nb_cur_slp_y", n);
  s.d_prev_slp_x = Kokkos::View<float*>("nb_prev_slp_x", n);
  s.d_prev_slp_y = Kokkos::View<float*>("nb_prev_slp_y", n);
  s.d_next_slp_x = Kokkos::View<float*>("nb_next_slp_x", n);
  s.d_next_slp_y = Kokkos::View<float*>("nb_next_slp_y", n);
  s.d_cur_x = Kokkos::View<float*>("nb_cur_x", n);
  s.d_cur_y = Kokkos::View<float*>("nb_cur_y", n);
  s.d_next_x = Kokkos::View<float*>("nb_next_x", n);
  s.d_next_y = Kokkos::View<float*>("nb_next_y", n);

  s.d_wl_grad_x = Kokkos::View<float*>("nb_wl_grad_x", n);
  s.d_wl_grad_y = Kokkos::View<float*>("nb_wl_grad_y", n);
  s.d_density_grad_x = Kokkos::View<float*>("nb_density_grad_x", n);
  s.d_density_grad_y = Kokkos::View<float*>("nb_density_grad_y", n);

  s.d_cur_sum_grads_x = Kokkos::View<float*>("nb_cur_sum_grads_x", n);
  s.d_cur_sum_grads_y = Kokkos::View<float*>("nb_cur_sum_grads_y", n);
  s.d_prev_sum_grads_x = Kokkos::View<float*>("nb_prev_sum_grads_x", n);
  s.d_prev_sum_grads_y = Kokkos::View<float*>("nb_prev_sum_grads_y", n);
  s.d_next_sum_grads_x = Kokkos::View<float*>("nb_next_sum_grads_x", n);
  s.d_next_sum_grads_y = Kokkos::View<float*>("nb_next_sum_grads_y", n);

  s.d_num_pins = Kokkos::View<int*>("nb_num_pins", n);
  s.d_area = Kokkos::View<float*>("nb_area", n);
  s.d_locked = Kokkos::View<int*>("nb_locked", n);
  s.d_nbc_index = Kokkos::View<int*>("nb_nbc_index", n);

  s.d_clamp_lx = Kokkos::View<float*>("nb_clamp_lx", n);
  s.d_clamp_ly = Kokkos::View<float*>("nb_clamp_ly", n);
  s.d_clamp_ux = Kokkos::View<float*>("nb_clamp_ux", n);
  s.d_clamp_uy = Kokkos::View<float*>("nb_clamp_uy", n);

  s.h_next_slp_x = Kokkos::create_mirror_view(s.d_next_slp_x);
  s.h_next_slp_y = Kokkos::create_mirror_view(s.d_next_slp_y);
  s.h_cur_slp_x = Kokkos::create_mirror_view(s.d_cur_slp_x);
  s.h_cur_slp_y = Kokkos::create_mirror_view(s.d_cur_slp_y);

  // Push static per-cell data.
  std::vector<int> h_num_pins(num_cells_);
  std::vector<float> h_area(num_cells_);
  std::vector<int> h_locked(num_cells_);
  std::vector<int> h_nbc_index(num_cells_);
  std::vector<float> h_clamp_lx(num_cells_);
  std::vector<float> h_clamp_ly(num_cells_);
  std::vector<float> h_clamp_ux(num_cells_);
  std::vector<float> h_clamp_uy(num_cells_);

  const float grid_lx = static_cast<float>(bg.lx());
  const float grid_ly = static_cast<float>(bg.ly());
  const float grid_ux = static_cast<float>(bg.ux());
  const float grid_uy = static_cast<float>(bg.uy());
  const float bsx = static_cast<float>(bg.getBinSizeX());
  const float bsy = static_cast<float>(bg.getBinSizeY());

  for (int i = 0; i < num_cells_; ++i) {
    const GCell* gc = nb_gcells[i];
    h_num_pins[i] = static_cast<int>(gc->gPins().size());
    h_area[i] = static_cast<float>(gc->dx()) * static_cast<float>(gc->dy());
    h_locked[i] = gc->isLocked() ? 1 : 0;

    if (nb_gcells[i].isNesterovBaseCommon()) {
      h_nbc_index[i] = static_cast<int>(nb_gcells[i].getStorageIndex());
    } else {
      h_nbc_index[i] = -1;
    }

    // Coord clamp bounds (same as getDensityCoordiLayoutInsideX/Y).
    const float ddx = static_cast<float>(gc->dDx());
    const float ddy = static_cast<float>(gc->dDy());
    h_clamp_lx[i] = grid_lx + bsx;
    h_clamp_ly[i] = grid_ly + bsy;
    h_clamp_ux[i] = grid_ux - bsx - ddx;
    h_clamp_uy[i] = grid_uy - bsy - ddy;
  }

  auto push_int = [&](Kokkos::View<int*>& d_view, std::vector<int>& h_vec) {
    Kokkos::View<int*, Kokkos::HostSpace, Kokkos::MemoryUnmanaged> hv(
        h_vec.data(), n);
    Kokkos::deep_copy(d_view, hv);
  };
  auto push_float
      = [&](Kokkos::View<float*>& d_view, std::vector<float>& h_vec) {
          Kokkos::View<float*, Kokkos::HostSpace, Kokkos::MemoryUnmanaged> hv(
              h_vec.data(), n);
          Kokkos::deep_copy(d_view, hv);
        };

  push_int(s.d_num_pins, h_num_pins);
  push_float(s.d_area, h_area);
  push_int(s.d_locked, h_locked);
  push_int(s.d_nbc_index, h_nbc_index);
  push_float(s.d_clamp_lx, h_clamp_lx);
  push_float(s.d_clamp_ly, h_clamp_ly);
  push_float(s.d_clamp_ux, h_clamp_ux);
  push_float(s.d_clamp_uy, h_clamp_uy);
}

NesterovDeviceContext::~NesterovDeviceContext() = default;

void NesterovDeviceContext::syncCoordsToDevice(
    const std::vector<FloatPoint>& curSLP,
    const std::vector<FloatPoint>& prevSLP,
    const std::vector<FloatPoint>& cur,
    const std::vector<FloatPoint>& curSumGrads,
    const std::vector<FloatPoint>& prevSumGrads)
{
  auto& s = *kokkos_;
  for (int i = 0; i < num_cells_; ++i) {
    s.h_cur_slp_x(i) = curSLP[i].x;
    s.h_cur_slp_y(i) = curSLP[i].y;
  }
  Kokkos::deep_copy(s.d_cur_slp_x, s.h_cur_slp_x);
  Kokkos::deep_copy(s.d_cur_slp_y, s.h_cur_slp_y);

  // prevSLP
  std::vector<float> hpx(num_cells_), hpy(num_cells_);
  for (int i = 0; i < num_cells_; ++i) {
    hpx[i] = prevSLP[i].x;
    hpy[i] = prevSLP[i].y;
  }
  Kokkos::View<float*, Kokkos::HostSpace, Kokkos::MemoryUnmanaged> hpxv(
      hpx.data(), num_cells_);
  Kokkos::View<float*, Kokkos::HostSpace, Kokkos::MemoryUnmanaged> hpyv(
      hpy.data(), num_cells_);
  Kokkos::deep_copy(s.d_prev_slp_x, hpxv);
  Kokkos::deep_copy(s.d_prev_slp_y, hpyv);

  // cur
  std::vector<float> hcx(num_cells_), hcy(num_cells_);
  for (int i = 0; i < num_cells_; ++i) {
    hcx[i] = cur[i].x;
    hcy[i] = cur[i].y;
  }
  Kokkos::View<float*, Kokkos::HostSpace, Kokkos::MemoryUnmanaged> hcxv(
      hcx.data(), num_cells_);
  Kokkos::View<float*, Kokkos::HostSpace, Kokkos::MemoryUnmanaged> hcyv(
      hcy.data(), num_cells_);
  Kokkos::deep_copy(s.d_cur_x, hcxv);
  Kokkos::deep_copy(s.d_cur_y, hcyv);

  // curSumGrads
  std::vector<float> hsgx(num_cells_), hsgy(num_cells_);
  for (int i = 0; i < num_cells_; ++i) {
    hsgx[i] = curSumGrads[i].x;
    hsgy[i] = curSumGrads[i].y;
  }
  Kokkos::View<float*, Kokkos::HostSpace, Kokkos::MemoryUnmanaged> hsgxv(
      hsgx.data(), num_cells_);
  Kokkos::View<float*, Kokkos::HostSpace, Kokkos::MemoryUnmanaged> hsgyv(
      hsgy.data(), num_cells_);
  Kokkos::deep_copy(s.d_cur_sum_grads_x, hsgxv);
  Kokkos::deep_copy(s.d_cur_sum_grads_y, hsgyv);

  // prevSumGrads
  std::vector<float> hpsgx(num_cells_), hpsgy(num_cells_);
  for (int i = 0; i < num_cells_; ++i) {
    hpsgx[i] = prevSumGrads[i].x;
    hpsgy[i] = prevSumGrads[i].y;
  }
  Kokkos::View<float*, Kokkos::HostSpace, Kokkos::MemoryUnmanaged> hpsgxv(
      hpsgx.data(), num_cells_);
  Kokkos::View<float*, Kokkos::HostSpace, Kokkos::MemoryUnmanaged> hpsgyv(
      hpsgy.data(), num_cells_);
  Kokkos::deep_copy(s.d_prev_sum_grads_x, hpsgxv);
  Kokkos::deep_copy(s.d_prev_sum_grads_y, hpsgyv);
}

void NesterovDeviceContext::syncCoordsToHost(std::vector<FloatPoint>& nextSLP,
                                             std::vector<FloatPoint>& next)
{
  auto& s = *kokkos_;
  Kokkos::deep_copy(s.h_next_slp_x, s.d_next_slp_x);
  Kokkos::deep_copy(s.h_next_slp_y, s.d_next_slp_y);
  for (int i = 0; i < num_cells_; ++i) {
    nextSLP[i].x = s.h_next_slp_x(i);
    nextSLP[i].y = s.h_next_slp_y(i);
  }

  // Also fetch next coords.
  Kokkos::View<float*>::HostMirror h_nx
      = Kokkos::create_mirror_view(s.d_next_x);
  Kokkos::View<float*>::HostMirror h_ny
      = Kokkos::create_mirror_view(s.d_next_y);
  Kokkos::deep_copy(h_nx, s.d_next_x);
  Kokkos::deep_copy(h_ny, s.d_next_y);
  for (int i = 0; i < num_cells_; ++i) {
    next[i].x = h_nx(i);
    next[i].y = h_ny(i);
  }
}

void NesterovDeviceContext::syncCurSLPToHost(std::vector<FloatPoint>& curSLP)
{
  auto& s = *kokkos_;
  Kokkos::deep_copy(s.h_cur_slp_x, s.d_cur_slp_x);
  Kokkos::deep_copy(s.h_cur_slp_y, s.d_cur_slp_y);
  for (int i = 0; i < num_cells_; ++i) {
    curSLP[i].x = s.h_cur_slp_x(i);
    curSLP[i].y = s.h_cur_slp_y(i);
  }
}

void NesterovDeviceContext::gradCombine(float density_penalty,
                                        float min_preconditioner,
                                        int target,
                                        float& wl_grad_sum,
                                        float& density_grad_sum)
{
  nestop::launchGradCombine(*kokkos_,
                            num_cells_,
                            density_penalty,
                            min_preconditioner,
                            target,
                            wl_grad_sum,
                            density_grad_sum);
}

void NesterovDeviceContext::nesterovCoordUpdate(float step_length, float coeff)
{
  nestop::launchNesterovCoordUpdate(*kokkos_, num_cells_, step_length, coeff);
}

void NesterovDeviceContext::updateInitialPrevSLPCoordi(float coef)
{
  nestop::launchUpdateInitialPrevSLPCoordi(*kokkos_, num_cells_, coef);
}

float NesterovDeviceContext::getDistance(int vec_a, int vec_b)
{
  return nestop::launchGetDistance(*kokkos_, num_cells_, vec_a, vec_b);
}

void NesterovDeviceContext::scatterToDeviceState(DeviceState* device_state,
                                                 int source)
{
  nestop::launchScatterToDeviceState(
      *kokkos_, device_state->kokkos(), num_cells_, source);
}

void NesterovDeviceContext::scatterWLGradsToNB(DeviceState* device_state)
{
  nestop::launchScatterGradsToNB(*kokkos_, device_state->kokkos(), num_cells_);
}

void NesterovDeviceContext::scatterDensityGradsToNB(DeviceState* device_state)
{
  auto& ns = *kokkos_;
  auto& ds = device_state->kokkos();
  auto d_nbc_index = ns.d_nbc_index;
  auto d_nb_dens_x = ns.d_density_grad_x;
  auto d_nb_dens_y = ns.d_density_grad_y;
  auto d_inst_dens_x = ds.d_inst_density_grad_x;
  auto d_inst_dens_y = ds.d_inst_density_grad_y;
  const int n = num_cells_;

  using ExecSpace = Kokkos::DefaultExecutionSpace;
  Kokkos::parallel_for(
      "nestop_scatter_dens_nb",
      Kokkos::RangePolicy<ExecSpace>(0, n),
      KOKKOS_LAMBDA(const int i) {
        const int nbc_idx = d_nbc_index(i);
        if (nbc_idx >= 0) {
          d_nb_dens_x(i) = d_inst_dens_x(nbc_idx);
          d_nb_dens_y(i) = d_inst_dens_y(nbc_idx);
        }
        // Fillers: density grad stays from previous K_density_gather
        // which now runs over all nb cells (Phase 4 filler support).
      });
}

void NesterovDeviceContext::syncPrevSLPToHost(std::vector<FloatPoint>& prevSLP)
{
  auto& s = *kokkos_;
  std::vector<float> hx(num_cells_), hy(num_cells_);
  Kokkos::View<float*, Kokkos::HostSpace, Kokkos::MemoryUnmanaged> hxv(
      hx.data(), num_cells_);
  Kokkos::View<float*, Kokkos::HostSpace, Kokkos::MemoryUnmanaged> hyv(
      hy.data(), num_cells_);
  Kokkos::deep_copy(hxv, s.d_prev_slp_x);
  Kokkos::deep_copy(hyv, s.d_prev_slp_y);
  for (int i = 0; i < num_cells_; ++i) {
    prevSLP[i].x = hx[i];
    prevSLP[i].y = hy[i];
  }
}

void NesterovDeviceContext::pushDensityGradsFromHost(
    const std::vector<FloatPoint>& densityGrads)
{
  auto& s = *kokkos_;
  std::vector<float> hx(num_cells_), hy(num_cells_);
  for (int i = 0; i < num_cells_; ++i) {
    hx[i] = densityGrads[i].x;
    hy[i] = densityGrads[i].y;
  }
  Kokkos::View<float*, Kokkos::HostSpace, Kokkos::MemoryUnmanaged> hxv(
      hx.data(), num_cells_);
  Kokkos::View<float*, Kokkos::HostSpace, Kokkos::MemoryUnmanaged> hyv(
      hy.data(), num_cells_);
  Kokkos::deep_copy(s.d_density_grad_x, hxv);
  Kokkos::deep_copy(s.d_density_grad_y, hyv);
}

void NesterovDeviceContext::swapCurNext()
{
  auto& s = *kokkos_;
  std::swap(s.d_cur_slp_x, s.d_next_slp_x);
  std::swap(s.d_cur_slp_y, s.d_next_slp_y);
  std::swap(s.d_cur_x, s.d_next_x);
  std::swap(s.d_cur_y, s.d_next_y);
}

void NesterovDeviceContext::swapSumGrads(int a, int b)
{
  auto& s = *kokkos_;
  auto get_pair
      = [&](int id) -> std::pair<Kokkos::View<float*>&, Kokkos::View<float*>&> {
    if (id == 0) {
      return {s.d_cur_sum_grads_x, s.d_cur_sum_grads_y};
    }
    if (id == 1) {
      return {s.d_prev_sum_grads_x, s.d_prev_sum_grads_y};
    }
    return {s.d_next_sum_grads_x, s.d_next_sum_grads_y};
  };
  auto [ax, ay] = get_pair(a);
  auto [bx, by] = get_pair(b);
  std::swap(ax, bx);
  std::swap(ay, by);
}

void NesterovDeviceContext::rotateForNextIter()
{
  auto& s = *kokkos_;
  // Match host-side updateNextIter: swap(prev,cur) then swap(cur,next).
  // SLP coords
  std::swap(s.d_prev_slp_x, s.d_cur_slp_x);
  std::swap(s.d_prev_slp_y, s.d_cur_slp_y);
  std::swap(s.d_cur_slp_x, s.d_next_slp_x);
  std::swap(s.d_cur_slp_y, s.d_next_slp_y);
  // Sum grads
  std::swap(s.d_prev_sum_grads_x, s.d_cur_sum_grads_x);
  std::swap(s.d_prev_sum_grads_y, s.d_cur_sum_grads_y);
  std::swap(s.d_cur_sum_grads_x, s.d_next_sum_grads_x);
  std::swap(s.d_cur_sum_grads_y, s.d_next_sum_grads_y);
  // Regular coords
  std::swap(s.d_cur_x, s.d_next_x);
  std::swap(s.d_cur_y, s.d_next_y);
}

}  // namespace gpl
