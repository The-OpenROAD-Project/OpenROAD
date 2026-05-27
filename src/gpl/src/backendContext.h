// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// BackendContext — a single bundle of construction parameters passed to each
// of the gpl Strategy backend factories (makeHpwlBackend,
// makeWirelengthGradientBackend, makeDensityGradientBackend, makeFftBackend).
//
// Each factory consumes the subset of fields it needs and ignores the rest;
// callers build one context per construction site and reuse it across the
// four factory calls. Plain C++ — Kokkos types are forward-declared elsewhere
// and pointers (DeviceState*, NesterovBase*, NesterovBaseCommon*) are only
// dereferenced inside backend translation units.

#pragma once

namespace gpl {

class DeviceState;
class NesterovBase;
class NesterovBaseCommon;

struct BackendContext
{
  // Owning / context pointers. nbc is required by the wirelength gradient
  // backend; nb is required by the density gradient backend; device_state is
  // borrowed by every GPU backend and ignored by the CPU backends.
  NesterovBaseCommon* nbc = nullptr;
  NesterovBase* nb = nullptr;
  DeviceState* device_state = nullptr;

  // OpenMP fan-out for the CPU backends.
  int num_threads = 1;

  // FFT-only grid geometry. Required by makeFftBackend; ignored elsewhere.
  int bin_cnt_x = 0;
  int bin_cnt_y = 0;
  float bin_size_x = 0;
  float bin_size_y = 0;
};

}  // namespace gpl
