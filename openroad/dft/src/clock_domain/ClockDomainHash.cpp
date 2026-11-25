// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024-2025, The OpenROAD Authors

#include "ClockDomainHash.hh"

#include <cstddef>
#include <functional>

#include "ClockDomain.hh"
#include "ScanArchitectConfig.hh"
#include "utl/Logger.h"

namespace dft {

std::function<size_t(const ClockDomain&)> GetClockDomainHashFn(
    const ScanArchitectConfig& config,
    utl::Logger* logger)
{
  switch (config.getClockMixing()) {
    // For NoMix, every clock domain is different
    case ScanArchitectConfig::ClockMixing::NoMix:
      return [](const ClockDomain& clock_domain) {
        return clock_domain.getClockDomainId();
      };
    case ScanArchitectConfig::ClockMixing::ClockMix:
      return [](const ClockDomain& clock_domain) { return 1; };
    default:
      // Not implemented
      logger->error(utl::DFT, 4, "Clock mix config requested is not supported");
  }
}

}  // namespace dft
