// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// HPWL (half-perimeter wirelength) backends and dispatch.
//
// CpuHpwlBackend — the OpenMP reduction over nets — is always compiled.
// makeHpwlBackend() is the single place the runtime backend choice is made: on
// an ENABLE_GPU build with the GPU path selected (gpl::gpuEnabled()) it returns
// the Kokkos GpuHpwlBackend, otherwise CpuHpwlBackend. NesterovBaseCommon::
// getHpwl() just delegates to the backend it was given at construction — no
// preprocessor branch, no backend knowledge.

#include <cassert>
#include <cstdint>
#include <memory>
#include <vector>

#include "hpwlBackend.h"
#include "nesterovBase.h"
#include "omp.h"

#ifdef ENABLE_GPU
#include "gpu/deviceState.h"
#include "gpu/gpuHpwlBackend.h"
#include "gpu/gpuRuntime.h"
#endif

namespace gpl {

namespace {

// CPU HPWL backend: the OpenMP reduction over nets. The loop body is
// byte-identical to the pre-GPU NesterovBaseCommon::getHpwl().
class CpuHpwlBackend : public HpwlBackend
{
 public:
  explicit CpuHpwlBackend(int num_threads) : num_threads_(num_threads) {}

  int64_t computeHpwl(std::vector<GNet>& nets) override
  {
    assert(omp_get_thread_num() == 0);
    int64_t hpwl = 0;
#pragma omp parallel for num_threads(num_threads_) reduction(+ : hpwl)
    for (auto gNet = nets.begin(); gNet < nets.end(); ++gNet) {
      // old-style loop for old OpenMP
      gNet->updateBox();
      hpwl += gNet->getHpwl();
    }
    return hpwl;
  }

  const char* name() const override { return "CPU (OpenMP)"; }

 private:
  int num_threads_;
};

}  // namespace

std::unique_ptr<HpwlBackend> makeHpwlBackend(int num_threads,
                                             DeviceState* device_state)
{
#ifdef ENABLE_GPU
  if (gpuEnabled()) {
    ensureKokkosInitialized();
    return std::make_unique<GpuHpwlBackend>(device_state);
  }
#else
  (void) device_state;
#endif
  return std::make_unique<CpuHpwlBackend>(num_threads);
}

int64_t NesterovBaseCommon::getHpwl()
{
#ifdef ENABLE_GPU
  // Phase 4+: when NesterovBase has already scattered fresh inst coords
  // from the device-resident Nesterov vectors, skip the host→device
  // round-trip — host gCellStor_::dCx/dCy is int-truncated and would lose
  // sub-integer precision the GPU coord-update kernel produced.
  if (device_state_ && !device_state_->consumeCoordsFresh()) {
    device_state_->syncInstCoordsFromHost(gCellStor_);
    device_state_->updatePinLocations();
  }
#endif
  return hpwl_backend_->computeHpwl(gNetStor_);
}

}  // namespace gpl
