// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024-2025, The OpenROAD Authors

#pragma once

#include <cstddef>
#include <functional>

#include "ClockDomain.hh"
#include "ScanArchitectConfig.hh"
#include "utl/Logger.h"

namespace dft {

// Depending on the ScanArchitectConfig's clock mixing setting, there are
// different ways to calculate the hash of the clock domain.
//
// For No Mix clock, we will generate a different hash value for all the clock
// domains.
//
// If we want to mix all the clocks, then the hash will be the same for all the
// clock doamins.
//
// We refer to the generated hash from a ClockDomain as Hash Domain.
//
std::function<size_t(const ClockDomain&)> GetClockDomainHashFn(
    const ScanArchitectConfig& config,
    utl::Logger* logger);

}  // namespace dft
