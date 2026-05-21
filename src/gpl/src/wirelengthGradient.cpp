// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// WA wirelength gradient backends + dispatch. Mirrors hpwl.cpp.
//
// CpuWirelengthGradientBackend wraps the existing OMP loops in
// NesterovBaseCommon. GpuWirelengthGradientBackend (a 5-kernel Kokkos
// pipeline) is added on ENABLE_GPU. makeWirelengthGradientBackend() picks
// per-process at run time (gpl::gpuEnabled()).

#include <atomic>
#include <cassert>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <memory>
#include <vector>

#include "nesterovBase.h"
#include "point.h"
#include "wirelengthGradientBackend.h"

#ifdef ENABLE_GPU
#include "gpu/deviceState.h"
#include "gpu/gpuRuntime.h"
#include "gpu/gpuWirelengthGradientBackend.h"
#endif

namespace gpl {

namespace {

// TEMP BENCH: per-process WA gradient timing for the Phase-2 perf cycle.
// Remove before merge (Phase 5). Same shape as HpwlBenchTimer in hpwl.cpp.
struct WlGradBenchTimer
{
  std::atomic<int64_t> force_calls{0};
  std::atomic<int64_t> force_us{0};
  std::atomic<int64_t> sync_us{0};
  std::atomic<int64_t> gather_calls{0};
  std::atomic<int64_t> gather_us{0};
  std::atomic<int64_t> single_calls{0};
  ~WlGradBenchTimer()
  {
    const int64_t fc = force_calls.load();
    const int64_t gc = gather_calls.load();
    if (fc > 0 || gc > 0) {
      const int64_t fu = force_us.load();
      const int64_t gu = gather_us.load();
      const int64_t su = sync_us.load();
      std::fprintf(stderr,
                   "[bench] WLgrad: force %ld calls %.3fs (%.1f us/call)"
                   "   sync %.3fs (%.1f us/call)"
                   "   gather %ld calls %.3fs (%.1f us/call)"
                   "   single %ld calls\n",
                   fc,
                   fu / 1e6,
                   fc > 0 ? static_cast<double>(fu) / fc : 0.0,
                   su / 1e6,
                   fc > 0 ? static_cast<double>(su) / fc : 0.0,
                   gc,
                   gu / 1e6,
                   gc > 0 ? static_cast<double>(gu) / gc : 0.0,
                   single_calls.load());
    }
  }
};
WlGradBenchTimer wl_grad_bench_timer;

// CPU backend: thin wrapper around the existing nbc methods. The OMP loops
// live in NesterovBaseCommon::updateWireLengthForceWA_native — same body as
// before the Phase-2 split, just renamed.
class CpuWirelengthGradientBackend : public WirelengthGradientBackend
{
 public:
  explicit CpuWirelengthGradientBackend(NesterovBaseCommon* nbc) : nbc_(nbc) {}

  void updateForce(float wlCoefX, float wlCoefY) override
  {
    last_wl_coef_x_ = wlCoefX;
    last_wl_coef_y_ = wlCoefY;
    nbc_->updateWireLengthForceWA_native(wlCoefX, wlCoefY);
  }

  void getCellGradients(const std::vector<GCellHandle>& gCells,
                        std::vector<FloatPoint>& out) override
  {
    assert(out.size() == gCells.size());
    // Sequential loop — matches NesterovBase::updateGradients (it disables
    // OMP for determinism, see nesterovBase.cpp:2802).
    for (std::size_t i = 0; i < gCells.size(); ++i) {
      const GCell* gCell = gCells[i];  // GCellHandle → GCell*
      out[i] = nbc_->getWireLengthGradientWA(
          gCell, last_wl_coef_x_, last_wl_coef_y_);
    }
  }

  FloatPoint getCellGradient(const GCell* gCell) override
  {
    return nbc_->getWireLengthGradientWA(
        gCell, last_wl_coef_x_, last_wl_coef_y_);
  }

  const char* name() const override { return "CPU (OpenMP)"; }

 private:
  NesterovBaseCommon* nbc_;
  // Backend contract: updateForce() must precede getCellGradient(s); the
  // CPU helper takes (coefX, coefY) per call so we replay the last values.
  float last_wl_coef_x_ = 0;
  float last_wl_coef_y_ = 0;
};

}  // namespace

std::unique_ptr<WirelengthGradientBackend> makeWirelengthGradientBackend(
    int num_threads,
    NesterovBaseCommon* nbc,
    DeviceState* device_state)
{
#ifdef ENABLE_GPU
  if (gpuEnabled()) {
    ensureKokkosInitialized();
    return std::make_unique<GpuWirelengthGradientBackend>(nbc, device_state);
  }
#else
  (void) device_state;
#endif
  (void) num_threads;
  return std::make_unique<CpuWirelengthGradientBackend>(nbc);
}

//
// NesterovBaseCommon hooks. Defined out-of-line here so this TU owns the
// backend dispatch + bench timing in one place. The native CPU body
// (updateWireLengthForceWA_native) and per-cell helpers stay in
// nesterovBase.cpp.
//
void NesterovBaseCommon::updateWireLengthForceWA(float wlCoeffX, float wlCoeffY)
{
#ifdef ENABLE_GPU
  // GPU backend reads pin coords from device_state_; refresh from host
  // gCellStor_ before dispatching. Mirrors hpwl.cpp pattern. After Phase 4
  // (Nesterov coord update on device) this disappears.
  if (device_state_) {
    const auto ts0 = std::chrono::steady_clock::now();
    device_state_->syncInstCoordsFromHost(gCellStor_);
    device_state_->updatePinLocations();
    const auto ts1 = std::chrono::steady_clock::now();
    wl_grad_bench_timer.sync_us.fetch_add(
        std::chrono::duration_cast<std::chrono::microseconds>(ts1 - ts0)
            .count());
  }
#endif
  const auto t0 = std::chrono::steady_clock::now();
  wl_grad_backend_->updateForce(wlCoeffX, wlCoeffY);
  const auto t1 = std::chrono::steady_clock::now();
  wl_grad_bench_timer.force_us.fetch_add(
      std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count());
  wl_grad_bench_timer.force_calls.fetch_add(1);
}

void NesterovBaseCommon::getAllWireLengthGradientsWA(
    const std::vector<GCellHandle>& gCells,
    std::vector<FloatPoint>& out)
{
  const auto t0 = std::chrono::steady_clock::now();
  wl_grad_backend_->getCellGradients(gCells, out);
  const auto t1 = std::chrono::steady_clock::now();
  wl_grad_bench_timer.gather_us.fetch_add(
      std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count());
  wl_grad_bench_timer.gather_calls.fetch_add(1);
}

FloatPoint NesterovBaseCommon::getSingleWireLengthGradientWA(const GCell* gCell)
{
  wl_grad_bench_timer.single_calls.fetch_add(1);
  return wl_grad_backend_->getCellGradient(gCell);
}

}  // namespace gpl
