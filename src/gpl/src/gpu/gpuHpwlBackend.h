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
  // factory in ../hpwl.cpp, owned by NesterovBaseCommon. `num_threads` fans
  // out the host-side mirror of per-net boxes (applyNetBoxesParallel).
  GpuHpwlBackend(DeviceState* device_state, int num_threads);
  ~GpuHpwlBackend() override;

  // Total HPWL over the nets. Bit-identical to the CPU loop (integer
  // arithmetic, deterministic across Kokkos backends). Unlike the CPU
  // backend this does NOT update host GNet boxes — they stay device-resident
  // until mirrorNetBoxesToHost() (see the contract in ../hpwlBackend.h).
  //
  // Caller invariant: device_state's inst coords must reflect current host
  // GCell positions and pin coords must be up-to-date. NesterovBaseCommon::
  // getHpwl() calls DeviceState::syncInstCoordsFromHost() and
  // updatePinLocations() right before invoking this backend.
  int64_t computeHpwl(std::vector<GNet>& nets) override;

  // Copies the device-resident per-net boxes of the last computeHpwl back to
  // the host GNet objects (OpenMP setBox fan-out). No-op before the first
  // computeHpwl, and also when the device buffers were sized for a different
  // net count (i.e. computeHpwl has not rerun since the netlist changed).
  void mirrorNetBoxesToHost(std::vector<GNet>& nets) override;

  const char* name() const override { return "GPU (Kokkos)"; }

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

// Mirrors the device-computed per-net bounding boxes back onto the host GNet
// objects (GNet::setBox) with an OpenMP fan-out — each net writes only its
// own slot, so the loop is embarrassingly parallel. On a 290k-net design the
// serial version of this loop cost ~2.8 ms per computeHpwl call (~3.1 s over
// a large01 run). Defined in ../hpwl.cpp, a plain C++ TU: gpuHpwlBackend.cpp
// is compiled as a CUDA TU, which does not get the OpenMP compile flags.
void applyNetBoxesParallel(std::vector<GNet>& gNetStor,
                           const int* lx,
                           const int* ly,
                           const int* ux,
                           const int* uy,
                           int num_threads);

}  // namespace gpl
