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

#include "backendContext.h"
#include "hpwlBackend.h"
#include "nesterovBase.h"
#include "omp.h"  // NOLINT(misc-include-cleaner): omp_get_thread_num used in assert below

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

std::unique_ptr<HpwlBackend> makeHpwlBackend(const BackendContext& ctx)
{
#ifdef ENABLE_GPU
  if (gpuEnabled()) {
    ensureKokkosInitialized();
    return std::make_unique<GpuHpwlBackend>(ctx.device_state);
  }
#endif
  return std::make_unique<CpuHpwlBackend>(ctx.num_threads);
}

int64_t NesterovBaseCommon::getHpwl()
{
#ifdef ENABLE_GPU
  // Sync the device-resident pin coords on the GPU path. ensureCoordsFresh
  // skips the host→device round-trip when NB has already scattered fresh
  // inst coords this iteration.
  if (device_state_) {
    device_state_->ensureCoordsFresh(gCellStor_);
  }
#endif
  return hpwl_backend_->computeHpwl(gNetStor_);
}

}  // namespace gpl
