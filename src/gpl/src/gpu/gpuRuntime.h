// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include "kokkosRuntime.h"

namespace gpl {

// Reads the ENABLE_GPU environment variable once (magic-static cached) and
// returns whether the GPU kernels should run in this process. When the GPU
// path is compiled in it is the default backend: the env var being unset
// returns true. The values "0", "off", "false", "no" and the empty string
// (case-insensitive) return false — the CPU opt-out for A/B testing and the
// golden suite. Any other value returns true.
bool gpuEnabled();

}  // namespace gpl
