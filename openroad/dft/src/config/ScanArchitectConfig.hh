// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2025, The OpenROAD Authors

#pragma once

#include <cstdint>
#include <optional>
#include <string>

#include "utl/Logger.h"

namespace dft {

class ScanArchitectConfig
{
 public:
  // TODO Add suport for mix_edges, mix_clocks, mix_clocks_not_edges
  enum class ClockMixing
  {
    NoMix,    // We create different scan chains for each clock and edge
    ClockMix  // We architect the flops of different clock and edge together
  };

  void setClockMixing(ClockMixing clock_mixing);

  // The max length in bits that a scan chain can have
  void setMaxLength(uint64_t max_length);
  const std::optional<uint64_t>& getMaxLength() const;

  // The max number of scan chains (per clock in NoMixe mode) to generate
  void setMaxChains(uint64_t max_chains);
  const std::optional<uint64_t>& getMaxChains() const;

  ClockMixing getClockMixing() const;

  // Prints using logger->report the config used by Scan Architect
  void report(utl::Logger* logger) const;

  static std::string ClockMixingName(ClockMixing clock_mixing);

 private:
  // The max length in bits of the scan chain
  std::optional<uint64_t> max_length_;
  // The max number of chains to generate
  std::optional<uint64_t> max_chains_;

  // How we are going to mix the clocks of the scan cells
  ClockMixing clock_mixing_;
};

}  // namespace dft
