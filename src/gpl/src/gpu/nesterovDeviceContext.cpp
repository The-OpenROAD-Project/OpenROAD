// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "nesterovDeviceContext.h"

#include <Kokkos_Core.hpp>
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <memory>
#include <vector>

#include "densityOp.h"
#include "deviceState.h"
#include "deviceState_kokkos.h"
#include "gpuRuntime.h"
#include "nesterovBase.h"
#include "nesterovDeviceState.h"
#include "nesterovOp.h"
#include "poissonSolver.h"

namespace gpl {

namespace {

using HostUM = Kokkos::View<float*, Kokkos::HostSpace, Kokkos::MemoryUnmanaged>;

// Copy a host vector<FloatPoint> into a pair of device float Views, staging
// through caller-owned scratch buffers (NesterovDeviceContext members).
// Scratch vectors must already be sized to src.size().
void pushVecPairToDevice(const std::vector<FloatPoint>& src,
                         std::vector<float>& scratch_x,
                         std::vector<float>& scratch_y,
                         Kokkos::View<float*>& dx,
                         Kokkos::View<float*>& dy)
{
  const int n = static_cast<int>(src.size());
  for (int i = 0; i < n; ++i) {
    scratch_x[i] = src[i].x;
    scratch_y[i] = src[i].y;
  }
  Kokkos::deep_copy(dx, HostUM(scratch_x.data(), n));
  Kokkos::deep_copy(dy, HostUM(scratch_y.data(), n));
}

// Pull a pair of device float Views back into a host vector<FloatPoint>,
// staging through caller-owned scratch buffers. `dst` must be pre-sized.
void pullVecPairToHost(const Kokkos::View<float*>& dx,
                       const Kokkos::View<float*>& dy,
                       std::vector<float>& scratch_x,
                       std::vector<float>& scratch_y,
                       std::vector<FloatPoint>& dst)
{
  const int n = static_cast<int>(dst.size());
  Kokkos::deep_copy(HostUM(scratch_x.data(), n), dx);
  Kokkos::deep_copy(HostUM(scratch_y.data(), n), dy);
  for (int i = 0; i < n; ++i) {
    dst[i].x = scratch_x[i];
    dst[i].y = scratch_y[i];
  }
}

// Deleter passed to the type-erased unique_ptr in nesterovDeviceContext.h.
// Defined here where KokkosNesterovState is complete.
void deleteKokkosNesterovState(KokkosNesterovState* p)
{
  delete p;
}

}  // namespace

NesterovDeviceContext::NesterovDeviceContext(
    const std::vector<GCellHandle>& nb_gcells,
    const BinGrid& bg)
    : kokkos_(new KokkosNesterovState(), &deleteKokkosNesterovState)
{
  ensureKokkosInitialized();

  num_cells_ = static_cast<int>(nb_gcells.size());
  scratch_x_.resize(num_cells_);
  scratch_y_.resize(num_cells_);
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

  // Device-resident density pipeline: per-cell params + per-bin statics +
  // NB-owned Poisson solver.
  s.d_half_ddx = Kokkos::View<int*>("nb_half_ddx", n);
  s.d_half_ddy = Kokkos::View<int*>("nb_half_ddy", n);
  s.d_density_scale = Kokkos::View<float*>("nb_density_scale", n);
  s.d_cell_kind = Kokkos::View<int*>("nb_cell_kind", n);
  refreshCellDensityParams(nb_gcells);
  initDensityDeviceState(bg);
}

void NesterovDeviceContext::refreshCellDensityParams(
    const std::vector<GCellHandle>& nb_gcells)
{
  assert(static_cast<int>(nb_gcells.size()) == num_cells_);
  auto& s = *kokkos_;
  const size_t n = static_cast<size_t>(num_cells_);

  std::vector<int> h_half_ddx(num_cells_);
  std::vector<int> h_half_ddy(num_cells_);
  std::vector<float> h_scale(num_cells_);
  std::vector<int> h_kind(num_cells_);
  for (int i = 0; i < num_cells_; ++i) {
    const GCell* gc = nb_gcells[i];
    h_half_ddx[i] = gc->dDx() / 2;
    h_half_ddy[i] = gc->dDy() / 2;
    h_scale[i] = gc->getDensityScale();
    // Same dispatch as the BinGrid scatter loop: macro and std instances
    // accumulate inst area (macro additionally target-density scaled),
    // fillers accumulate filler area, anything else is skipped.
    int kind = 0;
    if (gc->isInstance()) {
      if (gc->isMacroInstance()) {
        kind = 2;
      } else if (gc->isStdInstance()) {
        kind = 1;
      }
    } else if (gc->isFiller()) {
      kind = 3;
    }
    h_kind[i] = kind;
  }

  using HostIntUM
      = Kokkos::View<int*, Kokkos::HostSpace, Kokkos::MemoryUnmanaged>;
  using HostFloatUM
      = Kokkos::View<float*, Kokkos::HostSpace, Kokkos::MemoryUnmanaged>;
  Kokkos::deep_copy(s.d_half_ddx, HostIntUM(h_half_ddx.data(), n));
  Kokkos::deep_copy(s.d_half_ddy, HostIntUM(h_half_ddy.data(), n));
  Kokkos::deep_copy(s.d_density_scale, HostFloatUM(h_scale.data(), n));
  Kokkos::deep_copy(s.d_cell_kind, HostIntUM(h_kind.data(), n));
}

void NesterovDeviceContext::initDensityDeviceState(const BinGrid& bg)
{
  auto& s = *kokkos_;
  s.bin_cnt_x = bg.getBinCntX();
  s.bin_cnt_y = bg.getBinCntY();
  s.bin_size_x = bg.getBinSizeX();
  s.bin_size_y = bg.getBinSizeY();
  s.grid_lx = bg.lx();
  s.grid_ly = bg.ly();
  const size_t nbins
      = static_cast<size_t>(s.bin_cnt_x) * static_cast<size_t>(s.bin_cnt_y);

  s.d_bin_lx = Kokkos::View<int*>("nb_bin_lx", nbins);
  s.d_bin_ly = Kokkos::View<int*>("nb_bin_ly", nbins);
  s.d_bin_ux = Kokkos::View<int*>("nb_bin_ux", nbins);
  s.d_bin_uy = Kokkos::View<int*>("nb_bin_uy", nbins);
  s.d_bin_target_density = Kokkos::View<float*>("nb_bin_target", nbins);
  s.d_bin_scaled_area = Kokkos::View<float*>("nb_bin_scaled_area", nbins);
  s.d_bin_nonplace = Kokkos::View<long long*>("nb_bin_nonplace", nbins);
  s.d_bin_nonplace_unscaled
      = Kokkos::View<long long*>("nb_bin_nonplace_u", nbins);
  s.d_bin_inst_area = Kokkos::View<double*>("nb_bin_inst_area", nbins);
  s.d_bin_filler_area = Kokkos::View<double*>("nb_bin_filler_area", nbins);

  std::vector<int> h_lx(nbins), h_ly(nbins), h_ux(nbins), h_uy(nbins);
  std::vector<float> h_target(nbins), h_scaled(nbins);
  std::vector<long long> h_np(nbins), h_np_u(nbins);
  const std::vector<Bin>& bins = bg.getBinsConst();
  for (int x = 0; x < s.bin_cnt_x; ++x) {
    for (int y = 0; y < s.bin_cnt_y; ++y) {
      // BinGrid stores [y * binCntX + x]; the device views use the FFT
      // layout [x * binCntY + y] shared with d_bin_density / d_bin_elec_*.
      const Bin& b = bins[static_cast<size_t>(y) * s.bin_cnt_x + x];
      const size_t k = static_cast<size_t>(x) * s.bin_cnt_y + y;
      h_lx[k] = b.lx();
      h_ly[k] = b.ly();
      h_ux[k] = b.ux();
      h_uy[k] = b.uy();
      h_target[k] = b.getTargetDensity();
      // Same expression (and float rounding) as the CPU overflow loop.
      h_scaled[k] = static_cast<float>(b.getBinArea() * b.getTargetDensity());
      h_np[k] = b.getNonPlaceArea();
      h_np_u[k] = b.getNonPlaceAreaUnscaled();
    }
  }

  using HostIntUM
      = Kokkos::View<int*, Kokkos::HostSpace, Kokkos::MemoryUnmanaged>;
  using HostFloatUM
      = Kokkos::View<float*, Kokkos::HostSpace, Kokkos::MemoryUnmanaged>;
  using HostLLUM
      = Kokkos::View<long long*, Kokkos::HostSpace, Kokkos::MemoryUnmanaged>;
  Kokkos::deep_copy(s.d_bin_lx, HostIntUM(h_lx.data(), nbins));
  Kokkos::deep_copy(s.d_bin_ly, HostIntUM(h_ly.data(), nbins));
  Kokkos::deep_copy(s.d_bin_ux, HostIntUM(h_ux.data(), nbins));
  Kokkos::deep_copy(s.d_bin_uy, HostIntUM(h_uy.data(), nbins));
  Kokkos::deep_copy(s.d_bin_target_density,
                    HostFloatUM(h_target.data(), nbins));
  Kokkos::deep_copy(s.d_bin_scaled_area, HostFloatUM(h_scaled.data(), nbins));
  Kokkos::deep_copy(s.d_bin_nonplace, HostLLUM(h_np.data(), nbins));
  Kokkos::deep_copy(s.d_bin_nonplace_unscaled, HostLLUM(h_np_u.data(), nbins));

  // The Poisson solver's binCntX axis is gpl's fast (y) axis — same swap as
  // GpuFftBackend's solver member.
  s.solver = std::make_unique<PoissonSolver>(s.bin_cnt_y,
                                             s.bin_cnt_x,
                                             static_cast<float>(s.bin_size_y),
                                             static_cast<float>(s.bin_size_x));
}

NesterovDeviceContext::DensityIterResult
NesterovDeviceContext::densitySolveIteration(DeviceState* device_state,
                                             SlpSlot slot,
                                             bool want_sum_phi)
{
  auto& s = *kokkos_;
  KokkosDeviceState& ds = device_state->kokkos();
  assert(device_state->numBins() == s.bin_cnt_x * s.bin_cnt_y);

  DensityIterResult r;
  densop::launchNbDensityScatter(
      s, ds, num_cells_, slot, r.overflow_area, r.overflow_area_unscaled);
  if (want_sum_phi) {
    s.solver->solvePoisson(
        ds.d_bin_density, ds.d_bin_phi, ds.d_bin_elec_x, ds.d_bin_elec_y);
    r.sum_phi = densop::launchNbSumPhi(s, ds);
  } else {
    // sumPhi (and thus the potential) is a debug-only metric — skip the
    // potential IDCT, one of the solve's four transforms.
    s.solver->solvePoissonField(
        ds.d_bin_density, ds.d_bin_elec_x, ds.d_bin_elec_y);
  }
  last_density_slot_ = slot;
  return r;
}

void NesterovDeviceContext::densityGatherToNB(DeviceState* device_state,
                                              SlpSlot slot)
{
  densop::launchNbDensityGather(
      *kokkos_, device_state->kokkos(), num_cells_, slot);
}

// ~NesterovDeviceContext() is inline-defaulted in nesterovDeviceContext.h
// thanks to the function-pointer deleter on kokkos_.

void NesterovDeviceContext::syncCoordsToDevice(
    const std::vector<FloatPoint>& curSLP,
    const std::vector<FloatPoint>& prevSLP,
    const std::vector<FloatPoint>& cur,
    const std::vector<FloatPoint>& curSumGrads,
    const std::vector<FloatPoint>& prevSumGrads)
{
  // Inputs must match the device-side allocation; size drift would silently
  // shred the gradient state via Kokkos::deep_copy on mismatched extents.
  // The cutFillerCells/restoreRemovedFillers path now rebuilds *this so the
  // assertion stays satisfied, but catch any future caller that forgets.
  assert(static_cast<int>(curSLP.size()) == num_cells_);
  assert(static_cast<int>(prevSLP.size()) == num_cells_);
  assert(static_cast<int>(cur.size()) == num_cells_);
  assert(static_cast<int>(curSumGrads.size()) == num_cells_);
  assert(static_cast<int>(prevSumGrads.size()) == num_cells_);
  auto& s = *kokkos_;
  pushVecPairToDevice(
      curSLP, scratch_x_, scratch_y_, s.d_cur_slp_x, s.d_cur_slp_y);
  pushVecPairToDevice(
      prevSLP, scratch_x_, scratch_y_, s.d_prev_slp_x, s.d_prev_slp_y);
  pushVecPairToDevice(cur, scratch_x_, scratch_y_, s.d_cur_x, s.d_cur_y);
  pushVecPairToDevice(curSumGrads,
                      scratch_x_,
                      scratch_y_,
                      s.d_cur_sum_grads_x,
                      s.d_cur_sum_grads_y);
  pushVecPairToDevice(prevSumGrads,
                      scratch_x_,
                      scratch_y_,
                      s.d_prev_sum_grads_x,
                      s.d_prev_sum_grads_y);
}

void NesterovDeviceContext::syncCoordsToHost(std::vector<FloatPoint>& nextSLP,
                                             std::vector<FloatPoint>& next)
{
  assert(static_cast<int>(nextSLP.size()) == num_cells_);
  assert(static_cast<int>(next.size()) == num_cells_);
  auto& s = *kokkos_;
  pullVecPairToHost(
      s.d_next_slp_x, s.d_next_slp_y, scratch_x_, scratch_y_, nextSLP);
  pullVecPairToHost(s.d_next_x, s.d_next_y, scratch_x_, scratch_y_, next);
}

void NesterovDeviceContext::gradCombine(float density_penalty,
                                        float min_preconditioner,
                                        SumGradSlot target,
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

float NesterovDeviceContext::getDistance(SlpSlot vec_a, SlpSlot vec_b)
{
  return nestop::launchGetDistance(*kokkos_, num_cells_, vec_a, vec_b);
}

float NesterovDeviceContext::getDistance(SumGradSlot vec_a, SumGradSlot vec_b)
{
  return nestop::launchGetDistance(*kokkos_, num_cells_, vec_a, vec_b);
}

void NesterovDeviceContext::scatterToDeviceState(DeviceState* device_state,
                                                 SlpSlot source)
{
  nestop::launchScatterToDeviceState(
      *kokkos_, device_state->kokkos(), num_cells_, source);
}

void NesterovDeviceContext::scatterWLGradsToNB(DeviceState* device_state)
{
  nestop::launchScatterGradsToNB(*kokkos_, device_state->kokkos(), num_cells_);
}

void NesterovDeviceContext::syncCurCoordsToHost(std::vector<FloatPoint>& curSLP,
                                                std::vector<FloatPoint>& cur)
{
  assert(static_cast<int>(curSLP.size()) == num_cells_);
  assert(static_cast<int>(cur.size()) == num_cells_);
  auto& s = *kokkos_;
  pullVecPairToHost(
      s.d_cur_slp_x, s.d_cur_slp_y, scratch_x_, scratch_y_, curSLP);
  pullVecPairToHost(s.d_cur_x, s.d_cur_y, scratch_x_, scratch_y_, cur);
}

void NesterovDeviceContext::syncPrevSLPToHost(std::vector<FloatPoint>& prevSLP)
{
  assert(static_cast<int>(prevSLP.size()) == num_cells_);
  pullVecPairToHost(kokkos_->d_prev_slp_x,
                    kokkos_->d_prev_slp_y,
                    scratch_x_,
                    scratch_y_,
                    prevSLP);
}

void NesterovDeviceContext::syncCurSumGradsToHost(
    std::vector<FloatPoint>& curSumGrads)
{
  assert(static_cast<int>(curSumGrads.size()) == num_cells_);
  pullVecPairToHost(kokkos_->d_cur_sum_grads_x,
                    kokkos_->d_cur_sum_grads_y,
                    scratch_x_,
                    scratch_y_,
                    curSumGrads);
}

void NesterovDeviceContext::syncPrevSumGradsToHost(
    std::vector<FloatPoint>& prevSumGrads)
{
  assert(static_cast<int>(prevSumGrads.size()) == num_cells_);
  pullVecPairToHost(kokkos_->d_prev_sum_grads_x,
                    kokkos_->d_prev_sum_grads_y,
                    scratch_x_,
                    scratch_y_,
                    prevSumGrads);
}

void NesterovDeviceContext::pushDensityGradsFromHost(
    const std::vector<FloatPoint>& densityGrads)
{
  assert(static_cast<int>(densityGrads.size()) == num_cells_);
  pushVecPairToDevice(densityGrads,
                      scratch_x_,
                      scratch_y_,
                      kokkos_->d_density_grad_x,
                      kokkos_->d_density_grad_y);
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
