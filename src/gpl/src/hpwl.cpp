// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// HPWL (half-perimeter wirelength) backends and dispatch.
// makeHpwlBackend() chooses the backend: GpuHpwlBackend (Kokkos) on an
// ENABLE_GPU build with gpuEnabled(), else the always-compiled CpuHpwlBackend
// (Kokkos reduction over nets). NesterovBaseCommon::getHpwl() just delegates.

#include <Kokkos_Core.hpp>
#include <cstdint>
#include <memory>
#include <vector>

#include "backendContext.h"
#include "hpwlBackend.h"
#include "kokkosRuntime.h"
#include "nesterovBase.h"

#ifdef ENABLE_GPU
#include "gpu/deviceState.h"
#include "gpu/gpuHpwlBackend.h"
#include "gpu/gpuRuntime.h"
#endif

namespace gpl {

namespace {

// CPU HPWL backend: the Kokkos reduction over nets. The loop body is
// equivalent to the pre-GPU NesterovBaseCommon::getHpwl().
class CpuHpwlBackend : public HpwlBackend
{
 public:
  explicit CpuHpwlBackend(int num_threads) : num_threads_(num_threads) {}

  int64_t computeHpwl(std::vector<GNet>& nets) override
  {
    int64_t hpwl = 0;
    const auto space = hostExecutionSpace(num_threads_);
    Kokkos::parallel_reduce(
        "gpl::hpwl",
        HostRange(space, 0, nets.size()),
        [&](std::size_t index, int64_t& sum) {
          auto& gNet = nets[index];
          gNet.updateBox();
          sum += gNet.getHpwl();
        },
        Kokkos::Sum<int64_t>(hpwl));
    return hpwl;
  }

  const char* name() const override { return "CPU (Kokkos)"; }

 private:
  int num_threads_;
};

}  // namespace

std::unique_ptr<HpwlBackend> makeHpwlBackend(const BackendContext& ctx)
{
#ifdef ENABLE_GPU
  // Concurrent IO placement: IO pin GCells are not modeled by device pipelines.
  // Force CPU backends to ensure IO pins receive valid gradients
  if (gpuEnabled() && !ctx.place_ios_mode) {
    ensureKokkosInitialized();
    return std::make_unique<GpuHpwlBackend>(ctx.device_state, ctx.num_threads);
  }
#endif
  return std::make_unique<CpuHpwlBackend>(ctx.num_threads);
}

#ifdef ENABLE_GPU
// Host-side mirror of the device-computed per-net boxes, declared in
// gpu/gpuHpwlBackend.h. This uses the same host execution policy as HPWL.
void applyNetBoxesParallel(std::vector<GNet>& gNetStor,
                           const int* lx,
                           const int* ly,
                           const int* ux,
                           const int* uy,
                           const int num_threads)
{
  const int n_nets = static_cast<int>(gNetStor.size());
  const auto space = hostExecutionSpace(num_threads);
  Kokkos::parallel_for(
      "gpl::applyNetBoxes", HostRange(space, 0, n_nets), [&](int i) {
        gNetStor[i].setBox(lx[i], ly[i], ux[i], uy[i]);
      });
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
