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

namespace {

// Copy a host vector<FloatPoint> into a pair of device float Views.
void pushVecPairToDevice(const std::vector<FloatPoint>& src,
                         Kokkos::View<float*>& dx,
                         Kokkos::View<float*>& dy)
{
  const int n = static_cast<int>(src.size());
  std::vector<float> hx(n), hy(n);
  for (int i = 0; i < n; ++i) {
    hx[i] = src[i].x;
    hy[i] = src[i].y;
  }
  using HostUM
      = Kokkos::View<float*, Kokkos::HostSpace, Kokkos::MemoryUnmanaged>;
  Kokkos::deep_copy(dx, HostUM(hx.data(), n));
  Kokkos::deep_copy(dy, HostUM(hy.data(), n));
}

// Pull a pair of device float Views back into a host vector<FloatPoint>.
// `dst` must be pre-sized; only its element values are written.
void pullVecPairToHost(const Kokkos::View<float*>& dx,
                       const Kokkos::View<float*>& dy,
                       std::vector<FloatPoint>& dst)
{
  const int n = static_cast<int>(dst.size());
  std::vector<float> hx(n), hy(n);
  using HostUM
      = Kokkos::View<float*, Kokkos::HostSpace, Kokkos::MemoryUnmanaged>;
  Kokkos::deep_copy(HostUM(hx.data(), n), dx);
  Kokkos::deep_copy(HostUM(hy.data(), n), dy);
  for (int i = 0; i < n; ++i) {
    dst[i].x = hx[i];
    dst[i].y = hy[i];
  }
}

}  // namespace

NesterovDeviceContext::NesterovDeviceContext(
    const std::vector<GCellHandle>& nb_gcells,
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

    // Coord clamp bounds — must match NesterovBase::getDensityCoordiLayout-
    // InsideX/Y exactly. The CPU path clamps the cell *center* into
    // [bg.lx()+dDx/2, bg.ux()-dDx/2] (and Y mirror). Half the cell width,
    // NOT a bin width.
    const float half_ddx = 0.5f * static_cast<float>(gc->dDx());
    const float half_ddy = 0.5f * static_cast<float>(gc->dDy());
    h_clamp_lx[i] = grid_lx + half_ddx;
    h_clamp_ly[i] = grid_ly + half_ddy;
    h_clamp_ux[i] = grid_ux - half_ddx;
    h_clamp_uy[i] = grid_uy - half_ddy;
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
  pushVecPairToDevice(curSLP, s.d_cur_slp_x, s.d_cur_slp_y);
  pushVecPairToDevice(prevSLP, s.d_prev_slp_x, s.d_prev_slp_y);
  pushVecPairToDevice(cur, s.d_cur_x, s.d_cur_y);
  pushVecPairToDevice(curSumGrads, s.d_cur_sum_grads_x, s.d_cur_sum_grads_y);
  pushVecPairToDevice(prevSumGrads, s.d_prev_sum_grads_x, s.d_prev_sum_grads_y);
}

void NesterovDeviceContext::syncCoordsToHost(std::vector<FloatPoint>& nextSLP,
                                             std::vector<FloatPoint>& next)
{
  auto& s = *kokkos_;
  pullVecPairToHost(s.d_next_slp_x, s.d_next_slp_y, nextSLP);
  pullVecPairToHost(s.d_next_x, s.d_next_y, next);
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

void NesterovDeviceContext::syncPrevSLPToHost(std::vector<FloatPoint>& prevSLP)
{
  pullVecPairToHost(kokkos_->d_prev_slp_x, kokkos_->d_prev_slp_y, prevSLP);
}

void NesterovDeviceContext::syncCurSumGradsToHost(
    std::vector<FloatPoint>& curSumGrads)
{
  pullVecPairToHost(
      kokkos_->d_cur_sum_grads_x, kokkos_->d_cur_sum_grads_y, curSumGrads);
}

void NesterovDeviceContext::pushDensityGradsFromHost(
    const std::vector<FloatPoint>& densityGrads)
{
  pushVecPairToDevice(
      densityGrads, kokkos_->d_density_grad_x, kokkos_->d_density_grad_y);
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
