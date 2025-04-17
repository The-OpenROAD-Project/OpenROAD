// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2025, The OpenROAD Authors

#include "ScanArchitectConfig.hh"

#include <optional>
#include <string>

#include "Formatting.hh"

namespace dft {

ScanArchitectConfig::ClockMixing ScanArchitectConfig::getClockMixing() const
{
  return clock_mixing_;
}

const std::optional<uint64_t>& ScanArchitectConfig::getMaxLength() const
{
  return max_length_;
}

const std::optional<uint64_t>& ScanArchitectConfig::getMaxChains() const
{
  return max_chains_;
}

void ScanArchitectConfig::setClockMixing(ClockMixing clock_mixing)
{
  clock_mixing_ = clock_mixing;
}

void ScanArchitectConfig::setMaxChains(uint64_t max_chains)
{
  max_chains_ = max_chains;
}

void ScanArchitectConfig::setMaxLength(uint64_t max_length)
{
  max_length_ = max_length;
}

void ScanArchitectConfig::report(utl::Logger* logger) const
{
  logger->report("Scan Architect Config:");
  logger->report("- Max Length: {}", utils::FormatForReport(max_length_));
  logger->report("- Max Chains: {}", utils::FormatForReport(max_chains_));
  logger->report("- Clock Mixing: {}", ClockMixingName(clock_mixing_));
}

std::string ScanArchitectConfig::ClockMixingName(
    ScanArchitectConfig::ClockMixing clock_mixing)
{
  switch (clock_mixing) {
    case ScanArchitectConfig::ClockMixing::NoMix:
      return "No Mix";
    case ScanArchitectConfig::ClockMixing::ClockMix:
      return "Clock Mix";
    default:
      return "Missing case in ClockMixingName";
  }
}

}  // namespace dft
