///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2023, Google LLC
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
#pragma once

#include <cstdint>
#include <optional>

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
