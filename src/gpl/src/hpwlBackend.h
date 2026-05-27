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

// Strategy: computes the total HPWL over a net storage. Implementations also
// write each net's bounding box back via GNet::setBox — the side effect the
// legacy CPU loop performed and that later passes (routability, timing)
// depend on.
class HpwlBackend
{
 public:
  virtual ~HpwlBackend() = default;
  HpwlBackend(const HpwlBackend&) = delete;
  HpwlBackend& operator=(const HpwlBackend&) = delete;
  HpwlBackend(HpwlBackend&&) = delete;
  HpwlBackend& operator=(HpwlBackend&&) = delete;

  virtual int64_t computeHpwl(std::vector<GNet>& nets) = 0;

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
