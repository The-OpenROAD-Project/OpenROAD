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
    settings.set_disable_warnings(true);
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
  ensureKokkosInitialized(num_threads);
#ifdef KOKKOS_ENABLE_OPENMP
  return Kokkos::DefaultHostExecutionSpace(num_threads);
#else
  return Kokkos::DefaultHostExecutionSpace();
#endif
}

}  // namespace gpl
