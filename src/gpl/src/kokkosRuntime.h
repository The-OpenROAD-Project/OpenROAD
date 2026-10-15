// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <Kokkos_Core.hpp>
#include <cstddef>
#include <optional>

namespace gpl {

using HostRange = Kokkos::RangePolicy<Kokkos::DefaultHostExecutionSpace,
                                      Kokkos::IndexType<std::size_t>>;

// Initialize Kokkos once and finalize it at exit only if GPL initialized it.
void ensureKokkosInitialized(std::optional<int> num_threads = {});

// OpenMP supports per-instance sizing. Threads uses its initialization-time
// pool.
Kokkos::DefaultHostExecutionSpace hostExecutionSpace(int num_threads);

// The minimal header with fwd decl of Kokkos::DefaultHostExecutionSpace
// is Kokkos_Core_fwd.hpp and even this pulls cuda headers transitively.
// Use fwd decl of KokkosContext in headers where cuda shouldn't be
// pulled in and the full definition in source files
struct KokkosContext {
    Kokkos::DefaultHostExecutionSpace space;
};

}  // namespace gpl
