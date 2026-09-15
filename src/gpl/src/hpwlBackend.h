// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// HpwlBackend — the Strategy interface for the HPWL (half-perimeter
// wirelength) computation. CpuHpwlBackend (the OpenMP loop) is always
// available; GpuHpwlBackend (a Kokkos kernel) is added on an ENABLE_GPU build.
// makeHpwlBackend() picks one per process at run time (gpl::gpuEnabled()).
//
// This header is plain C++ — no Kokkos, no preprocessor branches — so
// nesterovBase.h can hold a std::unique_ptr<HpwlBackend> member without
// learning anything about the GPU build.

#pragma once

#include <cstdint>
#include <memory>
#include <type_traits>
#include <vector>

namespace gpl {

class GNet;

// Strategy: computes the total HPWL over a net storage.
//
// Host GNet box contract: the CPU backend updates every net's box as a side
// effect of computeHpwl (GNet::updateBox, the legacy behavior). The GPU
// backend keeps the boxes device-resident — mirroring them to the host cost
// ~2-3 ms per call on a 290k-net design (D2H copies plus the setBox
// fan-out) while (as of 2026-06) nothing reads host GNet boxes on the GPU
// path: the only readers — updateWireLengthForceWA_native, which recomputes
// the boxes itself via GNet::updateBox, and GNet::getHpwl under the CPU
// HPWL backend — run only on CPU paths. Callers that do need host boxes (or
// future host-side consumers) must call mirrorNetBoxesToHost() first;
// NesterovPlace does this at the timing-driven boundary and once after the
// Nesterov loop ends.
class HpwlBackend
{
 public:
  virtual ~HpwlBackend() = default;
  HpwlBackend(const HpwlBackend&) = delete;
  HpwlBackend& operator=(const HpwlBackend&) = delete;
  HpwlBackend(HpwlBackend&&) = delete;
  HpwlBackend& operator=(HpwlBackend&&) = delete;

  virtual int64_t computeHpwl(std::vector<GNet>& nets) = 0;

  // Refreshes host GNet boxes from the backend's last computeHpwl result.
  // No-op on the CPU backend (its computeHpwl already updates them); the GPU
  // backend copies the device-resident boxes back and applies GNet::setBox.
  // No-op as well if computeHpwl has not run yet.
  virtual void mirrorNetBoxesToHost(std::vector<GNet>& nets) {}

  // Short label for diagnostic logging; constructed-once factory choice.
  virtual const char* name() const = 0;

 protected:
  HpwlBackend() = default;
};

struct BackendContext;

// Factory: returns GpuHpwlBackend on an ENABLE_GPU build with the GPU path
// selected at run time, otherwise CpuHpwlBackend. Consumes ctx.num_threads
// (CPU path) and ctx.device_state (GPU path); other fields are ignored.
std::unique_ptr<HpwlBackend> makeHpwlBackend(const BackendContext& ctx);

static_assert(!std::is_copy_constructible_v<HpwlBackend>);
static_assert(!std::is_move_constructible_v<HpwlBackend>);

}  // namespace gpl
