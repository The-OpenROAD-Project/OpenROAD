// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// GpuHpwlBackend — the Kokkos GPU implementation of HpwlBackend (see
// ../hpwlBackend.h). Compiled only when ENABLE_GPU=ON; constructed by
// makeHpwlBackend() when the GPU path is selected at run time.
//
// This header carries no Kokkos types — the device kernel lives entirely in
// gpuHpwlBackend.cpp — so the HPWL factory in ../hpwl.cpp can construct a
// GpuHpwlBackend while staying a plain (non-CUDA) translation unit.

#pragma once

#include <cstdint>
#include <memory>
#include <vector>

#include "hpwlBackend.h"

namespace gpl {

class DeviceState;

// PIMPL: the persistent device-side Kokkos state lives in Impl, hidden in
// gpuHpwlBackend.cpp. This header stays Kokkos-free so it can be included by
// the plain-CXX makeHpwlBackend() factory in ../hpwl.cpp without forcing
// that TU to be compiled by nvcc (see src/gpl/CMakeLists.txt — hpwl.cpp is
// intentionally left as a CXX TU).
//
// The backend reads pin coordinates from a DeviceState shared with the
// owning NesterovBaseCommon: pin coords are computed on the device from the
// inst coords + per-pin offsets that DeviceState pre-loaded once. This
// eliminates the per-iteration host pin pack + 3 deep_copy that the earlier
// implementation paid; only the per-net bbox/reduction buffers below are
// backend-private.
class GpuHpwlBackend : public HpwlBackend
{
 public:
  // `device_state` is borrowed; must outlive this backend. Provided by the
  // factory in ../hpwl.cpp, owned by NesterovBaseCommon.
  explicit GpuHpwlBackend(DeviceState* device_state);
  ~GpuHpwlBackend() override;

  // Total HPWL over the nets; writes each net's bbox back via GNet::setBox.
  // Bit-identical to the CPU loop (integer arithmetic, deterministic across
  // Kokkos backends).
  //
  // Caller invariant: device_state's inst coords must reflect current host
  // GCell positions and pin coords must be up-to-date. NesterovBaseCommon::
  // getHpwl() calls DeviceState::syncInstCoordsFromHost() and
  // updatePinLocations() right before invoking this backend.
  int64_t computeHpwl(std::vector<GNet>& nets) override;

  const char* name() const override { return "GPU (Kokkos)"; }

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace gpl
