// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

namespace gpl {

// Initialize Kokkos once and finalize it at exit only if GPL initialized it.
void ensureKokkosInitialized();

}  // namespace gpl
