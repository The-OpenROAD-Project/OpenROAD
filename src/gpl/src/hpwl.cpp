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

#include <atomic>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <cstdio>
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

// TEMP BENCH: per-process HPWL backend timing for the Phase-1 perf cycle.
// Remove before merge. Splits backend-time from device-state sync time so we
// can see where the Phase 1 host pin pack savings actually land.
struct HpwlBenchTimer
{
  std::atomic<int64_t> calls{0};
  std::atomic<int64_t> backend_us{0};
  std::atomic<int64_t> sync_us{0};
  ~HpwlBenchTimer()
  {
    const int64_t c = calls.load();
    if (c > 0) {
      const int64_t bu = backend_us.load();
      const int64_t su = sync_us.load();
      std::fprintf(stderr,
                   "[bench] HPWL: %ld calls   backend %.3fs (%.1f us/call)"
                   "   sync %.3fs (%.1f us/call)\n",
                   c,
                   bu / 1e6,
                   static_cast<double>(bu) / c,
                   su / 1e6,
                   static_cast<double>(su) / c);
    }
  }
};
HpwlBenchTimer hpwl_bench_timer;

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
  // The GPU backend reads pin coords from device_state_; refresh them from
  // the current host instance positions before invoking the backend. After
  // Phase 4 (Nesterov coord update on device) this sync moves to a one-time
  // init load and disappears from the hot path.
  if (device_state_) {
    const auto ts0 = std::chrono::steady_clock::now();
    device_state_->syncInstCoordsFromHost(gCellStor_);
    device_state_->updatePinLocations();
    const auto ts1 = std::chrono::steady_clock::now();
    hpwl_bench_timer.sync_us.fetch_add(
        std::chrono::duration_cast<std::chrono::microseconds>(ts1 - ts0)
            .count());
  }
#endif
  const auto t0 = std::chrono::steady_clock::now();
  const int64_t result = hpwl_backend_->computeHpwl(gNetStor_);
  const auto t1 = std::chrono::steady_clock::now();
  hpwl_bench_timer.backend_us.fetch_add(
      std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count());
  hpwl_bench_timer.calls.fetch_add(1);
  return result;
}

}  // namespace gpl
