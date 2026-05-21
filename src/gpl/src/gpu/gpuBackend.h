// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Shared GPU-backend helpers for the gpl GPU kernel series (HPWL, FFT, ...).
//
// This header is intentionally Kokkos-free: it declares only two free
// functions and is safe to include from plain-C++ translation units (e.g.
// the CPU dispatch site in hpwl.cpp). The Kokkos-dependent definitions live
// in gpuBackend.cpp, which is compiled only when ENABLE_GPU=ON.

#pragma once

namespace gpl {

// Reads the ENABLE_GPU environment variable once (magic-static cached) and
// returns whether the GPU kernels should run in this process. When the GPU
// path is compiled in it is the default backend: the env var being unset
// returns true. The values "0", "off", "false", "no" and the empty string
// (case-insensitive) return false — the CPU opt-out for A/B testing and the
// golden suite. Any other value returns true.
bool gpuEnabled();

// Lazily initializes Kokkos on first call (std::call_once) and registers a
// std::atexit handler that finalizes it once at process shutdown. Safe to
// call from every GPU kernel entry point.
void ensureKokkosInitialized();

}  // namespace gpl
