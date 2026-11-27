// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// NesterovDeviceContext — PIMPL wrapper for KokkosNesterovState. Owns the
// NB-level device arrays for the Nesterov loop. Plain C++ header so
// NesterovBase can hold a unique_ptr without pulling in Kokkos.

#pragma once

#include <cstddef>
#include <memory>
#include <type_traits>
#include <vector>

#include "point.h"

namespace gpl {

class GCell;
class GCellHandle;
class BinGrid;
class DeviceState;
class RegionDensityField;
struct KokkosNesterovState;
struct KokkosDeviceState;
struct KokkosRegionDensityField;

// Per-cell vector slot identifiers — split by purpose so the launchers can
// not be passed an unrelated slot. Used by NesterovDeviceContext callers
// (NesterovBase) and the kernel launchers (nestop).
enum class SlpSlot : int
{
  Cur = 0,
  Prev = 1,
  Next = 2,
};

enum class SumGradSlot : int
{
  Cur = 0,
  Prev = 1,
  Next = 2,
};

class NesterovDeviceContext
{
 public:
  NesterovDeviceContext(const std::vector<GCellHandle>& nb_gcells,
                        const BinGrid& bg);
  NesterovDeviceContext() = delete;
  // Default destructor — see deviceState.h for the function-pointer
  // deleter rationale. Keeps unique_ptr<KokkosNesterovState> destruction
  // synthesizable in CPU-only TUs without exposing the Kokkos struct.
  ~NesterovDeviceContext() = default;

  // Non-copyable, non-movable — same reasoning as DeviceState.
  NesterovDeviceContext(const NesterovDeviceContext&) = delete;
  NesterovDeviceContext& operator=(const NesterovDeviceContext&) = delete;
  NesterovDeviceContext(NesterovDeviceContext&&) = delete;
  NesterovDeviceContext& operator=(NesterovDeviceContext&&) = delete;

  int numCells() const { return num_cells_; }

  // Push host Nesterov vectors to device.
  void syncCoordsToDevice(const std::vector<FloatPoint>& curSLP,
                          const std::vector<FloatPoint>& prevSLP,
                          const std::vector<FloatPoint>& cur,
                          const std::vector<FloatPoint>& curSumGrads,
                          const std::vector<FloatPoint>& prevSumGrads);

  // Pull device coords to host (reverse sync for density scatter).
  void syncCoordsToHost(std::vector<FloatPoint>& nextSLP,
                        std::vector<FloatPoint>& next);

  // Pull prevSLP coords to host (for density center update after
  // updateInitialPrevSLPCoordi).
  void syncPrevSLPToHost(std::vector<FloatPoint>& prevSLP);

  // Pull curSLP sum-grads from device to host. Needed before saveSnapshot:
  // on the GPU path, updateGradients writes sum-grads only to device, so
  // the host vector stays at zero unless explicitly synced.
  void syncCurSumGradsToHost(std::vector<FloatPoint>& curSumGrads);

  // Pull prevSLP sum-grads from device to host. Parallel to
  // syncCurSumGradsToHost; saveSnapshot uses both so revertToSnapshot can
  // push real values back instead of zombie host data.
  void syncPrevSumGradsToHost(std::vector<FloatPoint>& prevSumGrads);

  // GPU kernel: updateGradients loop body.
  void gradCombine(float density_penalty,
                   float min_preconditioner,
                   SumGradSlot target,
                   float& wl_grad_sum,
                   float& density_grad_sum);

  // GPU kernel: Nesterov coordinate update.
  void nesterovCoordUpdate(float step_length, float coeff);

  // GPU kernel: update initial prevSLP coords.
  void updateInitialPrevSLPCoordi(float coef);

  // GPU kernel: step length via distance reduction. Two overloads — the
  // step-length numerator iterates SLP coords, the denominator iterates
  // sum-grads, and the two are never crossed.
  float getDistance(SlpSlot vec_a, SlpSlot vec_b);
  float getDistance(SumGradSlot vec_a, SumGradSlot vec_b);

  // Scatter NB inst coords to DeviceState d_inst_cx/cy (for HPWL/WLgrad).
  void scatterToDeviceState(DeviceState* device_state, SlpSlot source);

  // Scatter DeviceState WL grads to NB arrays.
  void scatterWLGradsToNB(DeviceState* device_state);

  // Push complete density gradient vector (inst + filler) from host to device.
  // Required because GPU density backend only computes inst grads on device;
  // filler grads are CPU-computed and must be explicitly pushed.
  void pushDensityGradsFromHost(const std::vector<FloatPoint>& densityGrads);

  // ---- Device-resident density pipeline ----
  //
  // Result of one scatter + Poisson solve. sum_phi is only computed when
  // densitySolveIteration is called with want_sum_phi (it is a debug-only
  // metric); otherwise it is 0.
  struct DensityIterResult
  {
    float overflow_area = 0;
    float overflow_area_unscaled = 0;
    float sum_phi = 0;
  };

  // Run the per-iteration density pipeline fully on device: scatter the
  // `slot` coords into the region's bin grid (writes region_field's
  // d_bin_density), solve Poisson into region_field's d_bin_phi/elec_x/elec_y,
  // and reduce the overflow sums. Replaces the host
  // updateGCellDensityCenterLocation + updateDensityFieldBin pair in the
  // Nesterov hot loop.
  DensityIterResult densitySolveIteration(RegionDensityField* region_field,
                                          SlpSlot slot,
                                          bool want_sum_phi);

  // Gather the density gradient for ALL nb cells (fillers included) from the
  // region bin field at the `slot` coords, writing d_density_grad_x/y
  // directly — no host round-trip.
  void densityGatherToNB(RegionDensityField* region_field, SlpSlot slot);

  // Slot used by the most recent densitySolveIteration (the field on device
  // corresponds to these coords). updateGradients gathers from it.
  SlpSlot lastDensitySlot() const { return last_density_slot_; }

  // Re-upload per-cell density params (dDx/2, dDy/2, densityScale, kind)
  // after updateDensitySize / routability inflation changed them.
  void refreshCellDensityParams(const std::vector<GCellHandle>& nb_gcells);

  // Pull current-slot coords to host (curSLP + cur). Used to refresh the
  // host vectors lazily when the hot loop no longer syncs every iteration.
  void syncCurCoordsToHost(std::vector<FloatPoint>& curSLP,
                           std::vector<FloatPoint>& cur);

  // Device-side pointer rotation matching NesterovBase::updateNextIter swaps.
  void rotateForNextIter();

  // Accessor for Kokkos-aware TUs.
  KokkosNesterovState& kokkos() { return *kokkos_; }

 private:
  // Upload the per-bin static arrays (bounds, target density, non-place
  // areas) and construct the NB-owned Poisson solver. Called from the ctor.
  void initDensityDeviceState(const BinGrid& bg);

  // Type-erased deleter — see deviceState.h for rationale.
  using KokkosDeleter = void (*)(KokkosNesterovState*);
  std::unique_ptr<KokkosNesterovState, KokkosDeleter> kokkos_{nullptr, nullptr};
  int num_cells_ = 0;
  SlpSlot last_density_slot_ = SlpSlot::Cur;

  // Host scratch buffers reused by every push/pull sync call. Sized once
  // in the ctor to num_cells_ — avoids the per-call heap allocation that a
  // local std::vector<float> would incur (~5-10 syncs per Nesterov iter).
  std::vector<float> scratch_x_;
  std::vector<float> scratch_y_;
};

static_assert(!std::is_default_constructible_v<NesterovDeviceContext>);
static_assert(!std::is_copy_constructible_v<NesterovDeviceContext>);
static_assert(!std::is_move_constructible_v<NesterovDeviceContext>);

}  // namespace gpl
