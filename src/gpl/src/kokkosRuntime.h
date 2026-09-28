// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <Kokkos_Core.hpp>
#include <cstddef>

namespace gpl {

using HostRange = Kokkos::RangePolicy<Kokkos::DefaultHostExecutionSpace,
                                      Kokkos::IndexType<std::size_t>>;

// Initialize Kokkos once and finalize it at exit only if GPL initialized it.
void ensureKokkosInitialized();

// OpenMP supports per-instance sizing. Threads uses its initialization-time
// pool.
Kokkos::DefaultHostExecutionSpace hostExecutionSpace(int num_threads);

}  // namespace gpl
