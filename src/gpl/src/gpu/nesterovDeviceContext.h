// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// NesterovDeviceContext — PIMPL wrapper for KokkosNesterovState. Owns the
// NB-level device arrays for the Nesterov loop (Phase 4). Plain C++ header
// so NesterovBase can hold a unique_ptr without pulling in Kokkos.

#pragma once

#include <cstddef>
#include <memory>
#include <vector>

#include "point.h"

namespace gpl {

class GCell;
class GCellHandle;
class BinGrid;
class DeviceState;
struct KokkosNesterovState;
struct KokkosDeviceState;

class NesterovDeviceContext
{
 public:
  static constexpr int kVecCurSLP = 0;
  static constexpr int kVecPrevSLP = 1;
  static constexpr int kVecNextSLP = 2;
  static constexpr int kVecCurSumGrads = 3;
  static constexpr int kVecPrevSumGrads = 4;
  static constexpr int kVecNextSumGrads = 5;

  NesterovDeviceContext(const std::vector<GCellHandle>& nb_gcells,
                        const BinGrid& bg);
  ~NesterovDeviceContext();

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

  // GPU kernel: updateGradients loop body.
  void gradCombine(float density_penalty,
                   float min_preconditioner,
                   int target,
                   float& wl_grad_sum,
                   float& density_grad_sum);

  // GPU kernel: Nesterov coordinate update.
  void nesterovCoordUpdate(float step_length, float coeff);

  // GPU kernel: update initial prevSLP coords.
  void updateInitialPrevSLPCoordi(float coef);

  // GPU kernel: step length via distance reduction.
  float getDistance(int vec_a, int vec_b);

  // Scatter NB inst coords to DeviceState d_inst_cx/cy (for HPWL/WLgrad).
  void scatterToDeviceState(DeviceState* device_state, int source);

  // Scatter DeviceState WL grads to NB arrays.
  void scatterWLGradsToNB(DeviceState* device_state);

  // Push complete density gradient vector (inst + filler) from host to device.
  // Required because GPU density backend only computes inst grads on device;
  // filler grads are CPU-computed and must be explicitly pushed.
  void pushDensityGradsFromHost(const std::vector<FloatPoint>& densityGrads);

  // Device-side pointer rotation matching NesterovBase::updateNextIter swaps.
  void rotateForNextIter();

  // Accessor for Kokkos-aware TUs.
  KokkosNesterovState& kokkos() { return *kokkos_; }

 private:
  std::unique_ptr<KokkosNesterovState> kokkos_;
  int num_cells_ = 0;
};

}  // namespace gpl
