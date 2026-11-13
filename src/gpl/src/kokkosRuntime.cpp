// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "kokkosRuntime.h"

#include <Kokkos_Core.hpp>
#include <cstdlib>
#include <mutex>

namespace gpl {

void ensureKokkosInitialized(std::optional<int> num_threads)
{
  static std::once_flag once;
  std::call_once(once, [=] {
    if (Kokkos::is_initialized()) {
      return;
    }
    Kokkos::InitializationSettings settings;
    // Keep Kokkos warnings enabled when GPU build is enabled. In particular,
    // Kokkos reports when the compiled CUDA architecture differs from the selected device.
    // Compatible architectures can still run, but the mismatch may reduce performance.
    #ifndef ENABLE_GPU
    settings.set_disable_warnings(true);
    #endif
    if (num_threads) {
      settings.set_num_threads(*num_threads);
    }
    Kokkos::initialize(settings);
    std::atexit([] {
      if (Kokkos::is_initialized() && !Kokkos::is_finalized()) {
        Kokkos::finalize();
      }
    });
  });
}

Kokkos::DefaultHostExecutionSpace hostExecutionSpace(int num_threads)
{
#ifdef KOKKOS_ENABLE_OPENMP
    num_threads = std::clamp(num_threads, 1, Kokkos::Impl::OpenMPTraits::MAX_THREAD_COUNT);
#endif
#ifdef KOKKOS_ENABLE_THREADS
  num_threads = std::clamp(
      num_threads, 1, Kokkos::Impl::ThreadsInternal::MAX_THREAD_COUNT);
#endif
  ensureKokkosInitialized(num_threads);
#ifdef KOKKOS_ENABLE_OPENMP
  return Kokkos::DefaultHostExecutionSpace(num_threads);
#else
  return Kokkos::DefaultHostExecutionSpace();
#endif
}

}  // namespace gpl
