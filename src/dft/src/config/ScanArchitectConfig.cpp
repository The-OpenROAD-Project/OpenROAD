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

#include "ScanArchitectConfig.hh"

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
