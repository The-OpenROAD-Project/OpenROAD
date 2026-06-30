// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// HPWL (half-perimeter wirelength) backends and dispatch.
// makeHpwlBackend() chooses the backend: GpuHpwlBackend (Kokkos) on an
// ENABLE_GPU build with gpuEnabled(), else the always-compiled CpuHpwlBackend
// (OpenMP reduction over nets). NesterovBaseCommon::getHpwl() just delegates.

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
    for (auto& gNet : nets) {
      gNet.updateBox();
      hpwl += gNet.getHpwl();
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
    return std::make_unique<GpuHpwlBackend>(ctx.device_state, ctx.num_threads);
  }
#endif
  return std::make_unique<CpuHpwlBackend>(ctx.num_threads);
}

#ifdef ENABLE_GPU
// Host-side mirror of the device-computed per-net bboxes; declared in
// gpu/gpuHpwlBackend.h. Lives here rather than in gpuHpwlBackend.cpp because
// that TU is compiled as CUDA and does not get the OpenMP flags this loop
// needs.
void applyNetBoxesParallel(std::vector<GNet>& gNetStor,
                           const int* lx,
                           const int* ly,
                           const int* ux,
                           const int* uy,
                           const int num_threads)
{
  const int n_nets = static_cast<int>(gNetStor.size());
#pragma omp parallel for num_threads(num_threads)
  for (int i = 0; i < n_nets; ++i) {
    gNetStor[i].setBox(lx[i], ly[i], ux[i], uy[i]);
  }
}
#endif

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

void NesterovBaseCommon::mirrorNetBoxesToHost()
{
  hpwl_backend_->mirrorNetBoxesToHost(gNetStor_);
}

}  // namespace gpl
